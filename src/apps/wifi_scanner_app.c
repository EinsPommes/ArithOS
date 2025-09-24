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

typedef struct {
    bool scanning;
    int network_count;
    char networks[10][33];  // Store up to 10 networks, 32 chars + null terminator
    char security[10][8];   // Security type (WPA, WEP, OPEN)
    int8_t rssi[10];        // Signal strength
    int selected_network;
    int scroll_offset;
    char status_message[64];
    bool esp_initialized;
} wifi_scanner_data_t;

static app_t wifi_scanner_app;
static wifi_scanner_data_t wifi_scanner_data;

static void wifi_scanner_init(app_t* app);
static void wifi_scanner_update(app_t* app);
static void wifi_scanner_render(app_t* app);
static void wifi_scanner_process_key(app_t* app, uint8_t key_code);
static void wifi_scanner_destroy(app_t* app);

static void init_esp_module(wifi_scanner_data_t* data);
static void start_scan(wifi_scanner_data_t* data);
static void process_scan_results(wifi_scanner_data_t* data);
static void uart_write_string(const char* str);
static bool uart_read_line(char* buffer, int max_len, uint32_t timeout_ms);

app_t* register_wifi_scanner_app(void) {
    wifi_scanner_app.name = "WiFi Scanner";
    wifi_scanner_app.icon = "W";
    wifi_scanner_app.app_data = &wifi_scanner_data;
    wifi_scanner_app.init = wifi_scanner_init;
    wifi_scanner_app.update = wifi_scanner_update;
    wifi_scanner_app.render = wifi_scanner_render;
    wifi_scanner_app.process_key = wifi_scanner_process_key;
    wifi_scanner_app.destroy = wifi_scanner_destroy;
    
    return &wifi_scanner_app;
}

static void wifi_scanner_init(app_t* app) {
    wifi_scanner_data_t* data = (wifi_scanner_data_t*)app->app_data;
    
    data->scanning = false;
    data->network_count = 0;
    data->selected_network = 0;
    data->scroll_offset = 0;
    data->esp_initialized = false;
    strcpy(data->status_message, "Initializing ESP module...");
    
    init_esp_module(data);
}

static void wifi_scanner_update(app_t* app) {
    wifi_scanner_data_t* data = (wifi_scanner_data_t*)app->app_data;
    
    if (data->scanning) {
        process_scan_results(data);
    }
}

static void wifi_scanner_render(app_t* app) {
    wifi_scanner_data_t* data = (wifi_scanner_data_t*)app->app_data;
    
    // Draw header
    display_fill_rect(0, 0, DISPLAY_WIDTH, 40, COLOR_DARKGRAY);
    display_draw_text_aligned(DISPLAY_WIDTH / 2, 15, "WiFi Scanner", FONT_LARGE, ALIGN_CENTER, COLOR_WHITE);
    
    // Draw status message
    display_draw_text_aligned(DISPLAY_WIDTH / 2, 40, data->status_message, FONT_SMALL, ALIGN_CENTER, COLOR_WHITE);
    
    // Draw network list
    if (data->network_count > 0) {
        int y = 60;
        int visible_networks = (DISPLAY_HEIGHT - 100) / 30; // How many networks can fit on screen
        
        for (int i = data->scroll_offset; i < data->scroll_offset + visible_networks && i < data->network_count; i++) {
            // Background for network item
            uint16_t bg_color = (i == data->selected_network) ? COLOR_BLUE : COLOR_DARKGRAY;
            display_fill_rect(10, y, DISPLAY_WIDTH - 20, 25, bg_color);
            display_draw_rect(10, y, DISPLAY_WIDTH - 20, 25, COLOR_WHITE);
            
            // Network name
            display_draw_text(15, y + 5, data->networks[i], FONT_SMALL, COLOR_WHITE);
            
            // Security type
            display_draw_text(15, y + 15, data->security[i], FONT_SMALL, COLOR_YELLOW);
            
            // Signal strength
            char rssi_str[10];
            sprintf(rssi_str, "%d dBm", data->rssi[i]);
            display_draw_text_aligned(DISPLAY_WIDTH - 25, y + 10, rssi_str, FONT_SMALL, ALIGN_RIGHT, COLOR_GREEN);
            
            y += 30;
        }
        
        // Draw scroll indicators if needed
        if (data->scroll_offset > 0) {
            display_draw_text_aligned(DISPLAY_WIDTH / 2, 50, "▲", FONT_MEDIUM, ALIGN_CENTER, COLOR_WHITE);
        }
        if (data->scroll_offset + visible_networks < data->network_count) {
            display_draw_text_aligned(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT - 60, "▼", FONT_MEDIUM, ALIGN_CENTER, COLOR_WHITE);
        }
    } else if (!data->scanning && data->esp_initialized) {
        display_draw_text_aligned(DISPLAY_WIDTH / 2, 100, "No networks found", FONT_MEDIUM, ALIGN_CENTER, COLOR_WHITE);
    }
    
    // Draw control buttons
    display_fill_rect(10, DISPLAY_HEIGHT - 50, 100, 40, data->scanning ? COLOR_RED : COLOR_GREEN);
    display_draw_rect(10, DISPLAY_HEIGHT - 50, 100, 40, COLOR_WHITE);
    display_draw_text_aligned(60, DISPLAY_HEIGHT - 30, data->scanning ? "Stop" : "Scan", FONT_MEDIUM, ALIGN_CENTER, COLOR_WHITE);
    
    display_fill_rect(DISPLAY_WIDTH - 110, DISPLAY_HEIGHT - 50, 100, 40, COLOR_DARKGRAY);
    display_draw_rect(DISPLAY_WIDTH - 110, DISPLAY_HEIGHT - 50, 100, 40, COLOR_WHITE);
    display_draw_text_aligned(DISPLAY_WIDTH - 60, DISPLAY_HEIGHT - 30, "Back", FONT_MEDIUM, ALIGN_CENTER, COLOR_WHITE);
}

static void wifi_scanner_process_key(app_t* app, uint8_t key_code) {
    wifi_scanner_data_t* data = (wifi_scanner_data_t*)app->app_data;
    
    switch (key_code) {
        case KEY_2: // Up
            if (data->selected_network > 0) {
                data->selected_network--;
                if (data->selected_network < data->scroll_offset) {
                    data->scroll_offset = data->selected_network;
                }
            }
            break;
            
        case KEY_8: // Down
            if (data->selected_network < data->network_count - 1) {
                data->selected_network++;
                int visible_networks = (DISPLAY_HEIGHT - 100) / 30;
                if (data->selected_network >= data->scroll_offset + visible_networks) {
                    data->scroll_offset = data->selected_network - visible_networks + 1;
                }
            }
            break;
            
        case KEY_5: // Scan/Stop
            if (!data->scanning && data->esp_initialized) {
                start_scan(data);
            } else if (data->scanning) {
                data->scanning = false;
                strcpy(data->status_message, "Scan stopped");
            }
            break;
            
        case KEY_0: // Back to launcher
            app_manager_return_to_launcher();
            break;
            
        default:
            break;
    }
}

static void wifi_scanner_destroy(app_t* app) {
    wifi_scanner_data_t* data = (wifi_scanner_data_t*)app->app_data;
    
    // Stop scanning if active
    if (data->scanning) {
        data->scanning = false;
    }
    
    // Put ESP module to sleep to save power
    if (data->esp_initialized) {
        uart_write_string("AT+GSLP=0\r\n"); // Deep sleep mode
    }
}

static void init_esp_module(wifi_scanner_data_t* data) {
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
    } else {
        strcpy(data->status_message, "ESP module not responding");
    }
}

static void start_scan(wifi_scanner_data_t* data) {
    if (!data->esp_initialized) {
        strcpy(data->status_message, "ESP module not initialized");
        return;
    }
    
    // Clear previous results
    data->network_count = 0;
    data->selected_network = 0;
    data->scroll_offset = 0;
    
    // Start scan
    data->scanning = true;
    strcpy(data->status_message, "Scanning...");
    
    // Send scan command
    uart_write_string("AT+CWLAP\r\n");
}

static void process_scan_results(wifi_scanner_data_t* data) {
    char buffer[128];
    
    // Check for scan results
    if (uart_read_line(buffer, sizeof(buffer), 100)) {
        // Parse network info: +CWLAP:(sec,ssid,rssi,mac)
        if (strstr(buffer, "+CWLAP:") && data->network_count < 10) {
            // Extract security type
            char* sec_start = strchr(buffer, '(');
            if (sec_start) {
                int sec_type = atoi(sec_start + 1);
                switch (sec_type) {
                    case 0: strcpy(data->security[data->network_count], "OPEN"); break;
                    case 1: strcpy(data->security[data->network_count], "WEP"); break;
                    case 2: 
                    case 3: 
                    case 4: strcpy(data->security[data->network_count], "WPA"); break;
                    default: strcpy(data->security[data->network_count], "?"); break;
                }
                
                // Extract SSID
                char* ssid_start = strchr(sec_start, ',');
                if (ssid_start) {
                    ssid_start++; // Skip comma
                    if (*ssid_start == '\"') {
                        ssid_start++; // Skip quote
                        char* ssid_end = strchr(ssid_start, '\"');
                        if (ssid_end) {
                            int ssid_len = ssid_end - ssid_start;
                            if (ssid_len > 32) ssid_len = 32;
                            strncpy(data->networks[data->network_count], ssid_start, ssid_len);
                            data->networks[data->network_count][ssid_len] = '\0';
                            
                            // Extract RSSI
                            char* rssi_start = strchr(ssid_end, ',');
                            if (rssi_start) {
                                data->rssi[data->network_count] = atoi(rssi_start + 1);
                                data->network_count++;
                                
                                // Update status
                                sprintf(data->status_message, "Found %d networks", data->network_count);
                            }
                        }
                    }
                }
            }
        }
        
        // Check if scan is complete
        if (strstr(buffer, "OK") && strstr(buffer, "CWLAP")) {
            data->scanning = false;
            if (data->network_count == 0) {
                strcpy(data->status_message, "No networks found");
            } else {
                sprintf(data->status_message, "Found %d networks", data->network_count);
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