#include "app.h"
#include "../display.h"
#include "../keys.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    bool running;
    uint32_t start_time;
    uint32_t elapsed_time;
    uint32_t laps[10];
    int lap_count;
    int selected_lap;
    int scroll_offset;
} stopwatch_data_t;

static app_t stopwatch_app;
static stopwatch_data_t stopwatch_data;

static void stopwatch_init(app_t* app);
static void stopwatch_update(app_t* app);
static void stopwatch_render(app_t* app);
static void stopwatch_process_key(app_t* app, uint8_t key_code);
static void stopwatch_destroy(app_t* app);

app_t* register_stopwatch_app(void) {
    stopwatch_app.name = "Stopwatch";
    stopwatch_app.icon = "S";
    stopwatch_app.app_data = &stopwatch_data;
    stopwatch_app.init = stopwatch_init;
    stopwatch_app.update = stopwatch_update;
    stopwatch_app.render = stopwatch_render;
    stopwatch_app.process_key = stopwatch_process_key;
    stopwatch_app.destroy = stopwatch_destroy;
    
    return &stopwatch_app;
}

static void stopwatch_init(app_t* app) {
    stopwatch_data_t* data = (stopwatch_data_t*)app->app_data;
    
    data->running = false;
    data->start_time = 0;
    data->elapsed_time = 0;
    data->lap_count = 0;
    data->selected_lap = 0;
    data->scroll_offset = 0;
    
    memset(data->laps, 0, sizeof(data->laps));
}

static void stopwatch_update(app_t* app) {
    stopwatch_data_t* data = (stopwatch_data_t*)app->app_data;
    
    if (data->running) {
        data->elapsed_time = to_ms_since_boot(get_absolute_time()) - data->start_time;
    }
}

static void format_time(uint32_t time_ms, char* buffer) {
    uint32_t minutes = time_ms / 60000;
    uint32_t seconds = (time_ms / 1000) % 60;
    uint32_t ms = time_ms % 1000;
    
    sprintf(buffer, "%02lu:%02lu.%03lu", minutes, seconds, ms);
}

static void stopwatch_render(app_t* app) {
    stopwatch_data_t* data = (stopwatch_data_t*)app->app_data;
    char time_str[16];
    
    display_fill_rect(0, 0, DISPLAY_WIDTH, 40, COLOR_DARKGRAY);
    display_draw_text_aligned(DISPLAY_WIDTH / 2, 15, "Stopwatch", FONT_LARGE, ALIGN_CENTER, COLOR_WHITE);
    
    format_time(data->elapsed_time, time_str);
    display_draw_text_aligned(DISPLAY_WIDTH / 2, 60, time_str, FONT_LARGE, ALIGN_CENTER, COLOR_WHITE);
    
    if (data->lap_count > 0) {
        int y = 100;
        int visible_laps = (DISPLAY_HEIGHT - 140) / 20;
        
        for (int i = data->scroll_offset; i < data->scroll_offset + visible_laps && i < data->lap_count; i++) {
            char lap_str[32];
            format_time(data->laps[i], time_str);
            sprintf(lap_str, "Lap %d: %s", i + 1, time_str);
            
            uint16_t color = (i == data->selected_lap) ? COLOR_YELLOW : COLOR_WHITE;
            display_draw_text_aligned(DISPLAY_WIDTH / 2, y, lap_str, FONT_SMALL, ALIGN_CENTER, color);
            y += 20;
        }
        
        if (data->scroll_offset > 0) {
            display_draw_text_aligned(DISPLAY_WIDTH / 2, 90, "▲", FONT_SMALL, ALIGN_CENTER, COLOR_WHITE);
        }
        
        if (data->scroll_offset + visible_laps < data->lap_count) {
            display_draw_text_aligned(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT - 60, "▼", FONT_SMALL, ALIGN_CENTER, COLOR_WHITE);
        }
    }
    
    display_fill_rect(10, DISPLAY_HEIGHT - 50, 100, 40, data->running ? COLOR_RED : COLOR_GREEN);
    display_draw_rect(10, DISPLAY_HEIGHT - 50, 100, 40, COLOR_WHITE);
    display_draw_text_aligned(60, DISPLAY_HEIGHT - 30, data->running ? "Stop" : "Start", FONT_MEDIUM, ALIGN_CENTER, COLOR_WHITE);
    
    display_fill_rect(DISPLAY_WIDTH - 110, DISPLAY_HEIGHT - 50, 100, 40, COLOR_BLUE);
    display_draw_rect(DISPLAY_WIDTH - 110, DISPLAY_HEIGHT - 50, 100, 40, COLOR_WHITE);
    display_draw_text_aligned(DISPLAY_WIDTH - 60, DISPLAY_HEIGHT - 30, "Lap", FONT_MEDIUM, ALIGN_CENTER, COLOR_WHITE);
    
    display_draw_text_aligned(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT - 10, "0: Back", FONT_SMALL, ALIGN_CENTER, COLOR_WHITE);
}

static void stopwatch_process_key(app_t* app, uint8_t key_code) {
    stopwatch_data_t* data = (stopwatch_data_t*)app->app_data;
    
    switch (key_code) {
        case KEY_5: // Start/Stop
            if (!data->running) {
                data->running = true;
                data->start_time = to_ms_since_boot(get_absolute_time()) - data->elapsed_time;
            } else {
                data->running = false;
            }
            break;
            
        case KEY_EQUAL: // Lap
            if (data->running && data->lap_count < 10) {
                data->laps[data->lap_count++] = data->elapsed_time;
                data->selected_lap = data->lap_count - 1;
                
                int visible_laps = (DISPLAY_HEIGHT - 140) / 20;
                if (data->selected_lap >= data->scroll_offset + visible_laps) {
                    data->scroll_offset = data->selected_lap - visible_laps + 1;
                }
            } else if (!data->running) {
                data->elapsed_time = 0;
                data->lap_count = 0;
                data->selected_lap = 0;
                data->scroll_offset = 0;
            }
            break;
            
        case KEY_2: // Up
            if (data->selected_lap > 0) {
                data->selected_lap--;
                if (data->selected_lap < data->scroll_offset) {
                    data->scroll_offset = data->selected_lap;
                }
            }
            break;
            
        case KEY_8: // Down
            if (data->selected_lap < data->lap_count - 1) {
                data->selected_lap++;
                int visible_laps = (DISPLAY_HEIGHT - 140) / 20;
                if (data->selected_lap >= data->scroll_offset + visible_laps) {
                    data->scroll_offset = data->selected_lap - visible_laps + 1;
                }
            }
            break;
            
        case KEY_0: // Back to launcher
            app_manager_return_to_launcher();
            break;
            
        default:
            break;
    }
}

static void stopwatch_destroy(app_t* app) {
}
