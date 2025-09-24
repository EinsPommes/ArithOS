#include "app.h"
#include "../display.h"
#include "../keys.h"
#include "../board_pins.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    int selected_app;
    int scroll_offset;
} launcher_data_t;

static app_t launcher_app;
static launcher_data_t launcher_data;

#define ICON_SIZE 64
#define ICON_SPACING 20
#define ICONS_PER_ROW 3
#define ICONS_PER_COL 2
#define GRID_TOP 60
#define GRID_LEFT 10

static void launcher_init(app_t* app);
static void launcher_update(app_t* app);
static void launcher_render(app_t* app);
static void launcher_process_key(app_t* app, uint8_t key_code);
static void launcher_destroy(app_t* app);

app_t* register_launcher_app(void) {
    launcher_app.name = "Launcher";
    launcher_app.icon = "L";
    launcher_app.app_data = &launcher_data;
    launcher_app.init = launcher_init;
    launcher_app.update = launcher_update;
    launcher_app.render = launcher_render;
    launcher_app.process_key = launcher_process_key;
    launcher_app.destroy = launcher_destroy;
    
    return &launcher_app;
}

static void launcher_init(app_t* app) {
    launcher_data_t* data = (launcher_data_t*)app->app_data;
    data->selected_app = 0;
    data->scroll_offset = 0;
}

static void launcher_update(app_t* app) {
}

static void launcher_render(app_t* app) {
    launcher_data_t* data = (launcher_data_t*)app->app_data;
    
    display_fill_rect(0, 0, DISPLAY_WIDTH, 40, COLOR_DARKGRAY);
    display_draw_text_aligned(DISPLAY_WIDTH / 2, 15, "CalcOS", FONT_LARGE, ALIGN_CENTER, COLOR_WHITE);
    
    int app_count = app_manager_get_app_count();
    
    for (int i = 1; i < app_count; i++) {
        app_t* current_app = app_manager_get_app(i);
        if (!current_app) continue;
        
        int grid_index = i - 1;
        int row = grid_index / ICONS_PER_ROW;
        int col = grid_index % ICONS_PER_ROW;
        
        int x = GRID_LEFT + col * (ICON_SIZE + ICON_SPACING);
        int y = GRID_TOP + row * (ICON_SIZE + ICON_SPACING);
        
        uint16_t bg_color = (data->selected_app == i) ? COLOR_BLUE : COLOR_GRAY;
        display_fill_rect(x, y, ICON_SIZE, ICON_SIZE, bg_color);
        display_draw_rect(x, y, ICON_SIZE, ICON_SIZE, COLOR_WHITE);
        
        char icon[2] = {current_app->icon[0], '\0'};
        display_draw_text_aligned(x + ICON_SIZE / 2, y + ICON_SIZE / 2 - 5, 
                                icon, FONT_LARGE, ALIGN_CENTER, COLOR_WHITE);
        
        display_draw_text_aligned(x + ICON_SIZE / 2, y + ICON_SIZE - 10, 
                                current_app->name, FONT_SMALL, ALIGN_CENTER, COLOR_WHITE);
    }
    
    display_fill_rect(0, DISPLAY_HEIGHT - 30, DISPLAY_WIDTH, 30, COLOR_DARKGRAY);
    display_draw_text_aligned(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT - 20, 
                            "Press = to launch app", FONT_SMALL, ALIGN_CENTER, COLOR_WHITE);
}

static void launcher_process_key(app_t* app, uint8_t key_code) {
    launcher_data_t* data = (launcher_data_t*)app->app_data;
    int app_count = app_manager_get_app_count();
    
    switch (key_code) {
        case KEY_2: // Up
            if (data->selected_app > ICONS_PER_ROW) {
                data->selected_app -= ICONS_PER_ROW;
            }
            break;
            
        case KEY_8: // Down
            if (data->selected_app + ICONS_PER_ROW < app_count) {
                data->selected_app += ICONS_PER_ROW;
            }
            break;
            
        case KEY_4: // Left
            if (data->selected_app > 1) {
                data->selected_app--;
            }
            break;
            
        case KEY_6: // Right
            if (data->selected_app < app_count - 1) {
                data->selected_app++;
            }
            break;
            
        case KEY_EQUAL: // Launch selected app
            if (data->selected_app > 0 && data->selected_app < app_count) {
                app_manager_switch_to_app(data->selected_app);
            }
            break;
            
        default:
            break;
    }
}

static void launcher_destroy(app_t* app) {
}