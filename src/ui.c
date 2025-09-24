#include "ui.h"
#include "display.h"
#include "keys.h"
#include "calc.h"
#include "board_pins.h"
#include "hardware/pwm.h"
#include <string.h>
#include <stdio.h>

#define NOTIFICATION_TIMEOUT_MS 2000

#define HEADER_HEIGHT 30
#define DISPLAY_AREA_HEIGHT 50
#define KEYPAD_TOP (HEADER_HEIGHT + DISPLAY_AREA_HEIGHT + 10)
#define KEY_SIZE 40
#define KEY_SPACING 10
#define KEY_RADIUS 5

static ui_state_t ui_state;
static char notification_message[64];
static absolute_time_t notification_end_time;
static bool notification_active = false;

static const char* key_labels[4][4] = {
    {"7", "8", "9", "/"},
    {"4", "5", "6", "*"},
    {"1", "2", "3", "-"},
    {"0", "C", "=", "+"}
};

static const uint8_t key_mappings[4][4] = {
    {KEY_7, KEY_8, KEY_9, KEY_DIV},
    {KEY_4, KEY_5, KEY_6, KEY_MULT},
    {KEY_1, KEY_2, KEY_3, KEY_MINUS},
    {KEY_0, KEY_CLEAR, KEY_EQUAL, KEY_PLUS}
};

static void render_calculator_screen(void);
static void render_settings_screen(void);
static void render_about_screen(void);
static void render_notification(void);
static void render_keypad(void);
static void process_key_event(key_event_t event);
static void init_buzzer(void);

void ui_init(void) {
    ui_state.current_screen = UI_SCREEN_CALCULATOR;
    ui_state.needs_redraw = true;
    
    calc_init();
    init_buzzer();
    
    display_clear(COLOR_BLACK);
    ui_show_notification("ArithOS v1.0", 1500);
}

void ui_update(void) {
    key_event_t event;
    
    while (keys_get_event(&event)) {
        process_key_event(event);
    }
    
    if (notification_active && absolute_time_diff_us(get_absolute_time(), notification_end_time) >= 0) {
        notification_active = false;
        ui_state.needs_redraw = true;
    }
}

void ui_render(void) {
    if (!ui_state.needs_redraw) {
        return;
    }
    
    display_clear(COLOR_BLACK);
    
    switch (ui_state.current_screen) {
        case UI_SCREEN_CALCULATOR:
            render_calculator_screen();
            break;
            
        case UI_SCREEN_SETTINGS:
            render_settings_screen();
            break;
            
        case UI_SCREEN_ABOUT:
            render_about_screen();
            break;
    }
    
    if (notification_active) {
        render_notification();
    }
    
    display_update();
    ui_state.needs_redraw = false;
}

void ui_set_screen(ui_screen_t screen) {
    if (ui_state.current_screen != screen) {
        ui_state.current_screen = screen;
        ui_state.needs_redraw = true;
    }
}

ui_screen_t ui_get_current_screen(void) {
    return ui_state.current_screen;
}

void ui_show_notification(const char* message, uint32_t duration_ms) {
    strncpy(notification_message, message, sizeof(notification_message) - 1);
    notification_message[sizeof(notification_message) - 1] = '\0';
    notification_end_time = make_timeout_time_ms(duration_ms);
    notification_active = true;
    ui_state.needs_redraw = true;
}

void ui_play_sound(uint16_t frequency, uint16_t duration_ms) {
    uint slice_num = pwm_gpio_to_slice_num(PIN_BUZZER);
    uint chan = pwm_gpio_to_channel(PIN_BUZZER);
    
    uint32_t clock = 125000000;
    uint32_t divider = 125;
    uint32_t wrap = clock / (divider * frequency) - 1;
    pwm_set_clkdiv(slice_num, divider);
    pwm_set_wrap(slice_num, wrap);
    pwm_set_chan_level(slice_num, chan, wrap / 2);
    
    pwm_set_enabled(slice_num, true);
    add_alarm_in_ms(duration_ms, (alarm_callback_t)pwm_set_enabled, (void*)((slice_num << 1) | 0), false);
}

static void render_calculator_screen(void) {
    display_fill_rect(0, 0, DISPLAY_WIDTH, HEADER_HEIGHT, COLOR_DARKGRAY);
    display_draw_text_aligned(DISPLAY_WIDTH / 2, 10, "ArithOS Calculator", FONT_MEDIUM, ALIGN_CENTER, COLOR_WHITE);
    
    display_fill_rect(10, HEADER_HEIGHT + 5, DISPLAY_WIDTH - 20, DISPLAY_AREA_HEIGHT, COLOR_LIGHTGRAY);
    display_draw_rect(10, HEADER_HEIGHT + 5, DISPLAY_WIDTH - 20, DISPLAY_AREA_HEIGHT, COLOR_WHITE);
    
    const char* display_text = calc_get_display_string();
    display_draw_text_aligned(DISPLAY_WIDTH - 20, HEADER_HEIGHT + DISPLAY_AREA_HEIGHT / 2 + 5, 
                             display_text, FONT_LARGE, ALIGN_RIGHT, COLOR_BLACK);
    
    render_keypad();
}

static void render_settings_screen(void) {
    display_fill_rect(0, 0, DISPLAY_WIDTH, HEADER_HEIGHT, COLOR_DARKGRAY);
    display_draw_text_aligned(DISPLAY_WIDTH / 2, 10, "Settings", FONT_MEDIUM, ALIGN_CENTER, COLOR_WHITE);
    
    display_draw_text(20, 50, "Brightness", FONT_MEDIUM, COLOR_WHITE);
    display_draw_text(20, 80, "Sound", FONT_MEDIUM, COLOR_WHITE);
    display_draw_text(20, 110, "Theme", FONT_MEDIUM, COLOR_WHITE);
    
    display_fill_rect(10, DISPLAY_HEIGHT - 50, 100, 40, COLOR_DARKGRAY);
    display_draw_rect(10, DISPLAY_HEIGHT - 50, 100, 40, COLOR_WHITE);
    display_draw_text_aligned(60, DISPLAY_HEIGHT - 35, "Back", FONT_MEDIUM, ALIGN_CENTER, COLOR_WHITE);
}

static void render_about_screen(void) {
    display_fill_rect(0, 0, DISPLAY_WIDTH, HEADER_HEIGHT, COLOR_DARKGRAY);
    display_draw_text_aligned(DISPLAY_WIDTH / 2, 10, "About", FONT_MEDIUM, ALIGN_CENTER, COLOR_WHITE);
    
    display_draw_text_aligned(DISPLAY_WIDTH / 2, 60, "ArithOS v1.0", FONT_MEDIUM, ALIGN_CENTER, COLOR_WHITE);
    display_draw_text_aligned(DISPLAY_WIDTH / 2, 90, "A custom firmware for", FONT_SMALL, ALIGN_CENTER, COLOR_WHITE);
    display_draw_text_aligned(DISPLAY_WIDTH / 2, 110, "PicoCalc hardware", FONT_SMALL, ALIGN_CENTER, COLOR_WHITE);
    display_draw_text_aligned(DISPLAY_WIDTH / 2, 140, "Built with Pico SDK", FONT_SMALL, ALIGN_CENTER, COLOR_WHITE);
    
    display_fill_rect(10, DISPLAY_HEIGHT - 50, 100, 40, COLOR_DARKGRAY);
    display_draw_rect(10, DISPLAY_HEIGHT - 50, 100, 40, COLOR_WHITE);
    display_draw_text_aligned(60, DISPLAY_HEIGHT - 35, "Back", FONT_MEDIUM, ALIGN_CENTER, COLOR_WHITE);
}

static void render_notification(void) {
    int width = display_get_text_width(notification_message, FONT_MEDIUM) + 20;
    int height = 40;
    int x = (DISPLAY_WIDTH - width) / 2;
    int y = (DISPLAY_HEIGHT - height) / 2;
    
    display_fill_rect(x, y, width, height, COLOR_BLUE);
    display_draw_rect(x, y, width, height, COLOR_WHITE);
    
    display_draw_text_aligned(DISPLAY_WIDTH / 2, y + height / 2 - 5, 
                             notification_message, FONT_MEDIUM, ALIGN_CENTER, COLOR_WHITE);
}

static void render_keypad(void) {
    int start_x = (DISPLAY_WIDTH - (4 * KEY_SIZE + 3 * KEY_SPACING)) / 2;
    int start_y = KEYPAD_TOP;
    
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            int x = start_x + col * (KEY_SIZE + KEY_SPACING);
            int y = start_y + row * (KEY_SIZE + KEY_SPACING);
            
            uint16_t key_color;
            if (col == 3) {
                key_color = COLOR_ORANGE;
            } else if (row == 3 && col == 1) {
                key_color = COLOR_RED;
            } else if (row == 3 && col == 2) {
                key_color = COLOR_GREEN;
            } else {
                key_color = COLOR_GRAY;
            }
            
            display_fill_rect(x, y, KEY_SIZE, KEY_SIZE, key_color);
            display_draw_rect(x, y, KEY_SIZE, KEY_SIZE, COLOR_WHITE);
            
            display_draw_text_aligned(x + KEY_SIZE / 2, y + KEY_SIZE / 2 - 5, 
                                    key_labels[row][col], FONT_MEDIUM, ALIGN_CENTER, COLOR_WHITE);
            
            if (keys_is_pressed(key_mappings[row][col])) {
                display_draw_rect(x + 2, y + 2, KEY_SIZE - 4, KEY_SIZE - 4, COLOR_WHITE);
            }
        }
    }
}

static void process_key_event(key_event_t event) {
    if (event.event_type != KEY_EVENT_PRESSED) {
        return;
    }
    
    ui_play_sound(1000, 50);
    
    switch (ui_state.current_screen) {
        case UI_SCREEN_CALCULATOR:
            calc_process_key(event.key_code);
            ui_state.needs_redraw = true;
            break;
            
        case UI_SCREEN_SETTINGS:
        case UI_SCREEN_ABOUT:
            if (event.key_code == KEY_CLEAR) {
                ui_set_screen(UI_SCREEN_CALCULATOR);
            }
            break;
    }
}

static void init_buzzer(void) {
    gpio_set_function(PIN_BUZZER, GPIO_FUNC_PWM);
    
    uint slice_num = pwm_gpio_to_slice_num(PIN_BUZZER);
    
    pwm_config config = pwm_get_default_config();
    pwm_init(slice_num, &config, false);
}