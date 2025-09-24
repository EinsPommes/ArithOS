#include "app.h"
#include "../display.h"
#include "../keys.h"
#include "../board_pins.h"
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define UART_ID ESP_UART_ID
#define UART_TX_PIN ESP_UART_TX
#define UART_RX_PIN ESP_UART_RX
#define BAUD_RATE 115200

#define MAX_PACKETS 50
#define MAX_PACKET_LENGTH 64

typedef struct {
    bool sniffing;
    int packet_count;
    char packets[MAX_PACKETS][MAX_PACKET_LENGTH];  // Store packet info
    uint8_t packet_types[MAX_PACKETS];             // 0=data, 1=management, 2=control
    uint16_t packet_lengths[MAX_PACKETS];          // Length in bytes
    int selected_packet;
    int scroll_offset;
    char status_message[64];
    bool esp_initialized;
    int channel;                                   // WiFi channel (1-14)
    int filter_type;                               // 0=all, 1=data, 2=management, 3=control
} packet_sniffer_data_t;

static app_t packet_sniffer_app;
static packet_sniffer_data_t packet_sniffer_data;

static void packet_sniffer_init(app_t* app);
static void packet_sniffer_update(app_t* app);
static void packet_sniffer_render(app_t* app);
static void packet_sniffer_process_key(app_t* app, uint8_t key_code);
static void packet_sniffer_destroy(app_t* app);

static void init_esp_module(packet_sniffer_data_t* data);
static void start_sniffing(packet_sniffer_data_t* data);
static void stop_sniffing(packet_sniffer_data_t* data);
static void process_packet_data(packet_sniffer_data_t* data);
static void uart_write_string(const char* str);
static bool uart_read_line(char* buffer, int max_len, uint32_t timeout_ms);
static void change_channel(packet_sniffer_data_t* data, int channel);
static void toggle_filter(packet_sniffer_data_t* data);
static void render_packet_list(packet_sniffer_data_t* data);
static void render_packet_details(packet_sniffer_data_t* data);

app_t* register_packet_sniffer_app(void) {
    packet_sniffer_app.name = "Packet Sniffer";
    packet_sniffer_app.icon = "P";
    packet_sniffer_app.app_data = &packet_sniffer_data;
    packet_sniffer_app.init = packet_sniffer_init;
    packet_sniffer_app.update = packet_sniffer_update;
    packet_sniffer_app.render = packet_sniffer_render;
    packet_sniffer_app.process_key = packet_sniffer_process_key;
    packet_sniffer_app.destroy = packet_sniffer_destroy;
    
    return &packet_sniffer_app;
}

static void packet_sniffer_init(app_t* app) {
    packet_sniffer_data_t* data = (packet_sniffer_data_t*)app->app_data;
    
    data->sniffing = false;
    data->packet_count = 0;
    data->selected_packet = 0;
    data->scroll_offset = 0;
    data->esp_initialized = false;
    data->channel = 1;
    data->filter_type = 0; // All packets
    strcpy(data->status_message, "Initializing ESP module...");
    
    init_esp_module(data);
}

static void packet_sniffer_update(app_t* app) {
    packet_sniffer_data_t* data = (packet_sniffer_data_t*)app->app_data;
    
    if (data->sniffing) {
        process_packet_data(data);
    }
}

static void packet_sniffer_render(app_t* app) {
    packet_sniffer_data_t* data = (packet_sniffer_data_t*)app->app_data;
    
    // Draw header
    display_fill_rect(0, 0, DISPLAY_WIDTH, 40, COLOR_DARKGRAY);
    display_draw_text_aligned(DISPLAY_WIDTH / 2, 15, "Packet Sniffer", FONT_LARGE, ALIGN_CENTER, COLOR_WHITE);
    
    // Draw status bar
    char status_bar[64];
    const char* filter_types[] = {"All", "Data", "Mgmt", "Ctrl"};
    sprintf(status_bar, "CH:%d  Filter:%s  Pkts:%d", 
            data->channel, filter_types[data->filter_type], data->packet_count);
    display_draw_text_aligned(DISPLAY_WIDTH / 2, 40, status_bar, FONT_SMALL, ALIGN_CENTER, COLOR_YELLOW);
    
    // Draw packet list or details
    if (data->selected_packet < data->packet_count && data->selected_packet >= 0) {
        render_packet_details(data);
    } else {
        render_packet_list(data);
    }
    
    // Draw footer with controls
    display_fill_rect(0, DISPLAY_HEIGHT - 30, DISPLAY_WIDTH, 30, COLOR_DARKGRAY);
    
    if (data->sniffing) {
        display_draw_text(10, DISPLAY_HEIGHT - 20, "5:Stop 8:CH+ 2:CH- 0:Back", FONT_SMALL, COLOR_WHITE);
    } else {
        display_draw_text(10, DISPLAY_HEIGHT - 20, "5:Start 6:Filter 0:Back", FONT_SMALL, COLOR_WHITE);
    }
}

static void packet_sniffer_process_key(app_t* app, uint8_t key_code) {
    packet_sniffer_data_t* data = (packet_sniffer_data_t*)app->app_data;
    
    switch (key_code) {
        case KEY_2: // Up / Channel down
            if (data->sniffing) {
                // Change to previous channel
                if (data->channel > 1) {
                    change_channel(data, data->channel - 1);
                }
            } else if (data->selected_packet >= 0) {
                // Return to packet list
                data->selected_packet = -1;
            } else if (data->scroll_offset > 0) {
                // Scroll up in packet list
                data->scroll_offset--;
            }
            break;
            
        case KEY_8: // Down / Channel up
            if (data->sniffing) {
                // Change to next channel
                if (data->channel < 14) {
                    change_channel(data, data->channel + 1);
                }
            } else if (data->selected_packet < 0 && data->packet_count > 0) {
                // Select first packet for details
                data->selected_packet = data->scroll_offset;
            } else if (data->scroll_offset + 5 < data->packet_count && data->selected_packet < 0) {
                // Scroll down in packet list
                data->scroll_offset++;
            }
            break;
            
        case KEY_5: // Start/Stop sniffing
            if (!data->sniffing && data->esp_initialized) {
                start_sniffing(data);
            } else if (data->sniffing) {
                stop_sniffing(data);
            }
            break;
            
        case KEY_6: // Toggle filter
            if (!data->sniffing) {
                toggle_filter(data);
            }
            break;
            
        case KEY_0: // Back to launcher
            app_manager_return_to_launcher();
            break;
            
        default:
            break;
    }
}

static void packet_sniffer_destroy(app_t* app) {
    packet_sniffer_data_t* data = (packet_sniffer_data_t*)app->app_data;
    
    // Stop sniffing if active
    if (data->sniffing) {
        stop_sniffing(data);
    }
    
    // Put ESP module to sleep to save power
    if (data->esp_initialized) {
        uart_write_string("AT+GSLP=0\r\n"); // Deep sleep mode
    }
}

static void init_esp_module(packet_sniffer_data_t* data) {
    // Initialize UART for ESP8266/ESP32 communication
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    
    // Reset ESP module
    uart_write_string("AT+RST\r\n");
    sleep_ms(2000); // Wait for reset
    
    // Test AT command
    uart_write_string("AT\r\n");
    
    // Check for OK response
    char buffer[64];
    if (uart_read_line(buffer, sizeof(buffer), 1000) && strstr(buffer, "OK")) {
        data->esp_initialized = true;
        strcpy(data->status_message, "ESP module ready");
        
        // Set ESP to station mode
        uart_write_string("AT+CWMODE=1\r\n");
        sleep_ms(500);
        
        // Enable sniffing mode (this is a hypothetical command, actual implementation depends on ESP firmware)
        uart_write_string("AT+SNIFFMODE=1\r\n");
        sleep_ms(500);
    } else {
        strcpy(data->status_message, "ESP module not responding");
    }
}

static void start_sniffing(packet_sniffer_data_t* data) {
    if (!data->esp_initialized) {
        strcpy(data->status_message, "ESP module not initialized");
        return;
    }
    
    // Clear previous results
    data->packet_count = 0;
    data->selected_packet = -1;
    data->scroll_offset = 0;
    
    // Set channel
    change_channel(data, data->channel);
    
    // Start sniffing (hypothetical command)
    char cmd[32];
    sprintf(cmd, "AT+SNIFFSTART=%d,%d\r\n", data->channel, data->filter_type);
    uart_write_string(cmd);
    
    data->sniffing = true;
    strcpy(data->status_message, "Sniffing started");
}

static void stop_sniffing(packet_sniffer_data_t* data) {
    // Stop sniffing (hypothetical command)
    uart_write_string("AT+SNIFFSTOP\r\n");
    
    data->sniffing = false;
    sprintf(data->status_message, "Captured %d packets", data->packet_count);
}

static void process_packet_data(packet_sniffer_data_t* data) {
    char buffer[128];
    
    // Check for packet data
    if (uart_read_line(buffer, sizeof(buffer), 10)) {
        // Parse packet info (hypothetical format: +SNIFF:(type,length,mac1,mac2,rssi))
        if (strstr(buffer, "+SNIFF:") && data->packet_count < MAX_PACKETS) {
            // Extract packet type
            char* type_start = strchr(buffer, '(');
            if (type_start) {
                int type = atoi(type_start + 1);
                data->packet_types[data->packet_count] = type > 2 ? 0 : type;
                
                // Extract packet length
                char* len_start = strchr(type_start, ',');
                if (len_start) {
                    data->packet_lengths[data->packet_count] = atoi(len_start + 1);
                    
                    // Format packet info
                    char* mac_start = strchr(len_start + 1, ',');
                    if (mac_start) {
                        // Create simplified packet display
                        const char* type_str[] = {"DATA", "MGMT", "CTRL"};
                        sprintf(data->packets[data->packet_count], "%s %04d bytes",
                                type_str[data->packet_types[data->packet_count]],
                                data->packet_lengths[data->packet_count]);
                        
                        data->packet_count++;
                    }
                }
            }
        }
    }
}

static void uart_write_string(const char* str) {
    uart_puts(UART_ID, str);
}

static bool uart_read_line(char* buffer, int max_len, uint32_t timeout_ms) {
    int i = 0;
    absolute_time_t timeout_time = make_timeout_time_ms(timeout_ms);
    
    while (i < max_len - 1) {
        if (absolute_time_diff_us(get_absolute_time(), timeout_time) <= 0) {
            break; // Timeout
        }
        
        if (uart_is_readable(UART_ID)) {
            char c = uart_getc(UART_ID);
            if (c == '\n') {
                buffer[i] = '\0';
                return true;
            } else if (c != '\r') {
                buffer[i++] = c;
            }
        } else {
            sleep_ms(1);
        }
    }
    
    buffer[i] = '\0';
    return i > 0; // Return true if we read anything
}

static void change_channel(packet_sniffer_data_t* data, int channel) {
    if (channel < 1 || channel > 14) {
        return;
    }
    
    data->channel = channel;
    
    // Send channel change command (hypothetical)
    char cmd[32];
    sprintf(cmd, "AT+SNIFFCHANNEL=%d\r\n", channel);
    uart_write_string(cmd);
    
    sprintf(data->status_message, "Channel changed to %d", channel);
}

static void toggle_filter(packet_sniffer_data_t* data) {
    data->filter_type = (data->filter_type + 1) % 4;
    
    const char* filter_types[] = {"All packets", "Data packets", "Management packets", "Control packets"};
    sprintf(data->status_message, "Filter: %s", filter_types[data->filter_type]);
}

static void render_packet_list(packet_sniffer_data_t* data) {
    if (data->packet_count == 0) {
        // No packets captured
        display_draw_text_aligned(DISPLAY_WIDTH / 2, 120, "No packets captured", FONT_MEDIUM, ALIGN_CENTER, COLOR_WHITE);
        return;
    }
    
    // Draw packet list
    int y = 60;
    int visible_packets = 5; // How many packets can fit on screen
    
    for (int i = data->scroll_offset; i < data->scroll_offset + visible_packets && i < data->packet_count; i++) {
        // Background for packet item
        uint16_t bg_color = COLOR_DARKGRAY;
        switch (data->packet_types[i]) {
            case 0: bg_color = COLOR_BLUE; break;    // Data
            case 1: bg_color = COLOR_GREEN; break;   // Management
            case 2: bg_color = COLOR_ORANGE; break;  // Control
        }
        
        display_fill_rect(10, y, DISPLAY_WIDTH - 20, 25, bg_color);
        display_draw_rect(10, y, DISPLAY_WIDTH - 20, 25, COLOR_WHITE);
        
        // Packet number and info
        char packet_num[8];
        sprintf(packet_num, "%03d:", i + 1);
        display_draw_text(15, y + 8, packet_num, FONT_SMALL, COLOR_WHITE);
        display_draw_text(50, y + 8, data->packets[i], FONT_SMALL, COLOR_WHITE);
        
        y += 30;
    }
    
    // Draw scroll indicators if needed
    if (data->scroll_offset > 0) {
        display_draw_text_aligned(DISPLAY_WIDTH / 2, 50, "▲", FONT_MEDIUM, ALIGN_CENTER, COLOR_WHITE);
    }
    if (data->scroll_offset + visible_packets < data->packet_count) {
        display_draw_text_aligned(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT - 40, "▼", FONT_MEDIUM, ALIGN_CENTER, COLOR_WHITE);
    }
}

static void render_packet_details(packet_sniffer_data_t* data) {
    int idx = data->selected_packet;
    
    // Draw packet details header
    display_fill_rect(5, 50, DISPLAY_WIDTH - 10, 30, COLOR_DARKGRAY);
    
    char header[32];
    sprintf(header, "Packet #%d Details", idx + 1);
    display_draw_text_aligned(DISPLAY_WIDTH / 2, 60, header, FONT_MEDIUM, ALIGN_CENTER, COLOR_WHITE);
    
    // Draw packet info
    display_fill_rect(5, 85, DISPLAY_WIDTH - 10, DISPLAY_HEIGHT - 120, COLOR_BLACK);
    display_draw_rect(5, 85, DISPLAY_WIDTH - 10, DISPLAY_HEIGHT - 120, COLOR_WHITE);
    
    int y = 95;
    
    const char* type_str[] = {"Data", "Management", "Control"};
    char type_info[32];
    sprintf(type_info, "Type: %s", type_str[data->packet_types[idx]]);
    display_draw_text(15, y, type_info, FONT_SMALL, COLOR_WHITE);
    y += 20;
    
    char len_info[32];
    sprintf(len_info, "Length: %d bytes", data->packet_lengths[idx]);
    display_draw_text(15, y, len_info, FONT_SMALL, COLOR_WHITE);
    y += 20;
    
    // Simulated packet data (in a real implementation, this would be actual packet data)
    display_draw_text(15, y, "MAC: 00:11:22:33:44:55", FONT_SMALL, COLOR_WHITE);
    y += 20;
    
    display_draw_text(15, y, "RSSI: -65 dBm", FONT_SMALL, COLOR_WHITE);
    y += 20;
    
    display_draw_text(15, y, "Channel: 1", FONT_SMALL, COLOR_WHITE);
    y += 20;
    
    display_draw_text(15, y, "Press [2] to return", FONT_SMALL, COLOR_YELLOW);
}
