#include "app.h"
#include "../display.h"
#include "../keys.h"
#include "../board_pins.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

typedef struct {
    char display[32];
    double current_value;
    double stored_value;
    char operation;
    bool new_input;
    bool has_decimal;
    bool error;
} calculator_data_t;

static app_t calculator_app;
static calculator_data_t calculator_data;

static void calculator_init(app_t* app);
static void calculator_update(app_t* app);
static void calculator_render(app_t* app);
static void calculator_process_key(app_t* app, uint8_t key_code);
static void calculator_destroy(app_t* app);
static void clear_calculator(calculator_data_t* data);
static void update_display(calculator_data_t* data);
static void perform_calculation(calculator_data_t* data);

app_t* register_calculator_app(void) {
    calculator_app.name = "Calculator";
    calculator_app.icon = "C";
    calculator_app.app_data = &calculator_data;
    calculator_app.init = calculator_init;
    calculator_app.update = calculator_update;
    calculator_app.render = calculator_render;
    calculator_app.process_key = calculator_process_key;
    calculator_app.destroy = calculator_destroy;
    
    return &calculator_app;
}

static void calculator_init(app_t* app) {
    calculator_data_t* data = (calculator_data_t*)app->app_data;
    clear_calculator(data);
}

static void calculator_update(app_t* app) {
}

static void calculator_render(app_t* app) {
    calculator_data_t* data = (calculator_data_t*)app->app_data;
    
    display_fill_rect(0, 0, DISPLAY_WIDTH, 40, COLOR_DARKGRAY);
    display_draw_text_aligned(DISPLAY_WIDTH / 2, 15, "Calculator", FONT_LARGE, ALIGN_CENTER, COLOR_WHITE);
    
    display_fill_rect(10, 50, DISPLAY_WIDTH - 20, 40, COLOR_LIGHTGRAY);
    display_draw_rect(10, 50, DISPLAY_WIDTH - 20, 40, COLOR_BLACK);
    
    display_draw_text_aligned(DISPLAY_WIDTH - 20, 70, data->display, FONT_LARGE, ALIGN_RIGHT, COLOR_BLACK);
    
    int key_size = 40;
    int key_spacing = 10;
    int keypad_top = 100;
    int keypad_left = (DISPLAY_WIDTH - (4 * key_size + 3 * key_spacing)) / 2;
    
    const char* key_labels[4][4] = {
        {"7", "8", "9", "/"},
        {"4", "5", "6", "*"},
        {"1", "2", "3", "-"},
        {"0", "C", "=", "+"}
    };
    
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            int x = keypad_left + col * (key_size + key_spacing);
            int y = keypad_top + row * (key_size + key_spacing);
            
            uint16_t color;
            if (col == 3) {
                color = COLOR_ORANGE;
            } else if (row == 3 && col == 1) {
                color = COLOR_RED;
            } else if (row == 3 && col == 2) {
                color = COLOR_GREEN;
            } else {
                color = COLOR_GRAY;
            }
            
            display_fill_rect(x, y, key_size, key_size, color);
            display_draw_rect(x, y, key_size, key_size, COLOR_WHITE);
            display_draw_text_aligned(x + key_size / 2, y + key_size / 2, 
                                    key_labels[row][col], FONT_MEDIUM, ALIGN_CENTER, COLOR_WHITE);
        }
    }
    
    display_fill_rect(10, DISPLAY_HEIGHT - 40, 80, 30, COLOR_DARKGRAY);
    display_draw_rect(10, DISPLAY_HEIGHT - 40, 80, 30, COLOR_WHITE);
    display_draw_text_aligned(50, DISPLAY_HEIGHT - 25, "Back", FONT_MEDIUM, ALIGN_CENTER, COLOR_WHITE);
}

static void calculator_process_key(app_t* app, uint8_t key_code) {
    calculator_data_t* data = (calculator_data_t*)app->app_data;
    
    if (key_code >= KEY_0 && key_code <= KEY_9) {
        char digit = '0' + key_code;
        
        if (data->new_input) {
            strcpy(data->display, "");
            data->new_input = false;
            data->has_decimal = false;
        }
        
        if (strlen(data->display) < 15) {
            char temp[32];
            sprintf(temp, "%s%c", data->display, digit);
            strcpy(data->display, temp);
        }
    }
    else if (key_code == KEY_PLUS || key_code == KEY_MINUS || 
             key_code == KEY_MULT || key_code == KEY_DIV) {
        
        if (data->operation != '\0') {
            perform_calculation(data);
        } else {
            data->stored_value = atof(data->display);
        }
        
        switch (key_code) {
            case KEY_PLUS:  data->operation = '+'; break;
            case KEY_MINUS: data->operation = '-'; break;
            case KEY_MULT:  data->operation = '*'; break;
            case KEY_DIV:   data->operation = '/'; break;
        }
        
        data->new_input = true;
    }
    else if (key_code == KEY_EQUAL) {
        perform_calculation(data);
        data->operation = '\0';
        data->new_input = true;
    }
    else if (key_code == KEY_CLEAR) {
        clear_calculator(data);
    }
    else if (key_code == KEY_0 && keys_is_pressed(KEY_CLEAR)) {
        app_manager_return_to_launcher();
        return;
    }
    
    update_display(data);
}

static void calculator_destroy(app_t* app) {
}

static void clear_calculator(calculator_data_t* data) {
    strcpy(data->display, "0");
    data->current_value = 0;
    data->stored_value = 0;
    data->operation = '\0';
    data->new_input = true;
    data->has_decimal = false;
    data->error = false;
}

static void update_display(calculator_data_t* data) {
    data->display[31] = '\0';
}

static void perform_calculation(calculator_data_t* data) {
    if (data->error) {
        return;
    }
    
    double current = atof(data->display);
    double result = 0;
    
    switch (data->operation) {
        case '+':
            result = data->stored_value + current;
            break;
        case '-':
            result = data->stored_value - current;
            break;
        case '*':
            result = data->stored_value * current;
            break;
        case '/':
            if (fabs(current) < 1e-10) {
                strcpy(data->display, "Error");
                data->error = true;
                return;
            }
            result = data->stored_value / current;
            break;
        default:
            result = current;
            break;
    }
    
    if (fabs(result) > 1e10 || (fabs(result) < 1e-10 && result != 0)) {
        sprintf(data->display, "%e", result);
    } else {
        sprintf(data->display, "%.10g", result);
    }
    
    data->stored_value = result;
}