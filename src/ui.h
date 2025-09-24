#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    UI_SCREEN_CALCULATOR,
    UI_SCREEN_SETTINGS,
    UI_SCREEN_ABOUT
} ui_screen_t;

typedef struct {
    ui_screen_t current_screen;
    bool needs_redraw;
} ui_state_t;

void ui_init(void);
void ui_update(void);
void ui_render(void);
void ui_set_screen(ui_screen_t screen);
ui_screen_t ui_get_current_screen(void);
void ui_show_notification(const char* message, uint32_t duration_ms);
void ui_play_sound(uint16_t frequency, uint16_t duration_ms);