#include "boot_animation.h"
#include "board_pins.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

static const uint8_t logo_bitmap[] = {
    0,0,0,0,1,1,1,1,1,1,0,0,0,0,
    0,0,0,1,1,0,0,0,0,1,1,0,0,0,
    0,0,1,1,0,0,0,0,0,0,1,1,0,0,
    0,1,1,0,0,0,0,0,0,0,0,1,1,0,
    0,1,0,0,0,0,0,0,0,0,0,0,1,0,
    1,1,0,0,0,0,0,0,0,0,0,0,1,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,1,0,0,0,0,0,0,0,0,0,0,1,1,
    0,1,0,0,0,0,0,0,0,0,0,0,1,0,
    0,1,1,0,0,0,0,0,0,0,0,1,1,0,
    0,0,1,1,0,0,0,0,0,0,1,1,0,0,
    0,0,0,1,1,0,0,0,0,1,1,0,0,0,
    0,0,0,0,1,1,1,1,1,1,0,0,0,0,
};

#define LOGO_WIDTH 14
#define LOGO_HEIGHT 17
#define LOGO_SCALE 6
#define MAX_DROPS 20
#define DROP_LENGTH 10

typedef struct {
    int x;
    int y;
    int speed;
    int length;
    uint16_t color;
} drop_t;

static void draw_logo(int16_t x, int16_t y, uint16_t color) {
    for (int16_t row = 0; row < LOGO_HEIGHT; row++) {
        for (int16_t col = 0; col < LOGO_WIDTH; col++) {
            if (logo_bitmap[row * LOGO_WIDTH + col]) {
                display_fill_rect(
                    x + col * LOGO_SCALE, 
                    y + row * LOGO_SCALE, 
                    LOGO_SCALE, LOGO_SCALE, 
                    color
                );
            }
        }
    }
}

static void matrix_animation(int duration_ms) {
    drop_t drops[MAX_DROPS];
    const char* chars = "01010101";
    int char_count = strlen(chars);
    
    for (int i = 0; i < MAX_DROPS; i++) {
        drops[i].x = rand() % DISPLAY_WIDTH;
        drops[i].y = (rand() % DISPLAY_HEIGHT) - DROP_LENGTH * 8;
        drops[i].speed = 2 + (rand() % 4);
        drops[i].length = 5 + (rand() % DROP_LENGTH);
        drops[i].color = COLOR_GREEN;
    }
    
    uint32_t start_time = to_ms_since_boot(get_absolute_time());
    uint32_t current_time;
    
    do {
        display_clear(COLOR_BLACK);
        
        for (int i = 0; i < MAX_DROPS; i++) {
            for (int j = 0; j < drops[i].length; j++) {
                int y = drops[i].y - j * 8;
                if (y >= 0 && y < DISPLAY_HEIGHT) {
                    uint16_t color = drops[i].color;
                    if (j > 0) {
                        float fade = 1.0f - (float)j / drops[i].length;
                        uint8_t r = ((color >> 11) & 0x1F) * fade;
                        uint8_t g = ((color >> 5) & 0x3F) * fade;
                        uint8_t b = (color & 0x1F) * fade;
                        color = (r << 11) | (g << 5) | b;
                    }
                    
                    char c = chars[(i + j) % char_count];
                    display_draw_text(drops[i].x, y, &c, FONT_SMALL, color);
                }
            }
            
            drops[i].y += drops[i].speed;
            
            if (drops[i].y - 8 * drops[i].length > DISPLAY_HEIGHT) {
                drops[i].x = rand() % DISPLAY_WIDTH;
                drops[i].y = -DROP_LENGTH * 8;
                drops[i].speed = 2 + (rand() % 4);
            }
        }
        
        display_update();
        sleep_ms(50);
        
        current_time = to_ms_since_boot(get_absolute_time());
    } while (current_time - start_time < duration_ms);
}

static void wipe_effect(uint16_t color, int duration_ms) {
    int step_size = 4;
    int delay_per_step = duration_ms / (DISPLAY_WIDTH / step_size);
    
    for (int x = 0; x < DISPLAY_WIDTH; x += step_size) {
        display_fill_rect(x, 0, step_size, DISPLAY_HEIGHT, color);
        display_update();
        sleep_ms(delay_per_step);
    }
}

static void radar_scan(int16_t center_x, int16_t center_y, int duration_ms) {
    int max_radius = DISPLAY_WIDTH > DISPLAY_HEIGHT ? DISPLAY_WIDTH : DISPLAY_HEIGHT;
    max_radius = (int)(max_radius * 1.5f);
    
    uint32_t start_time = to_ms_since_boot(get_absolute_time());
    uint32_t current_time;
    float angle = 0.0f;
    
    do {
        display_clear(COLOR_BLACK);
        
        display_draw_circle(center_x, center_y, 30, COLOR_DARKGRAY);
        display_draw_circle(center_x, center_y, 60, COLOR_DARKGRAY);
        display_draw_circle(center_x, center_y, 90, COLOR_DARKGRAY);
        display_draw_circle(center_x, center_y, 120, COLOR_DARKGRAY);
        
        float rad = angle * (3.14159f / 180.0f);
        int line_x = center_x + (int)(cos(rad) * max_radius);
        int line_y = center_y + (int)(sin(rad) * max_radius);
        display_draw_line(center_x, center_y, line_x, line_y, COLOR_GREEN);
        
        for (int i = 0; i < 30; i++) {
            float echo_angle = angle - i * 3.0f;
            if (echo_angle < 0) echo_angle += 360.0f;
            
            float echo_rad = echo_angle * (3.14159f / 180.0f);
            float fade = 1.0f - (float)i / 30.0f;
            
            uint16_t color = (uint16_t)((0 << 11) | ((uint8_t)(31 * fade) << 5) | 0);
            
            int echo_x = center_x + (int)(cos(echo_rad) * max_radius * 0.8f);
            int echo_y = center_y + (int)(sin(echo_rad) * max_radius * 0.8f);
            display_draw_line(center_x, center_y, echo_x, echo_y, color);
        }
        
        angle += 3.0f;
        if (angle >= 360.0f) angle -= 360.0f;
        
        display_update();
        sleep_ms(50);
        
        current_time = to_ms_since_boot(get_absolute_time());
    } while (current_time - start_time < duration_ms);
}

void show_boot_animation(void) {
    int16_t center_x = DISPLAY_WIDTH / 2;
    int16_t center_y = DISPLAY_HEIGHT / 2;
    int16_t logo_x = center_x - (LOGO_WIDTH * LOGO_SCALE) / 2;
    int16_t logo_y = center_y - (LOGO_HEIGHT * LOGO_SCALE) / 2;
    
    display_clear(COLOR_BLACK);
    display_update();
    sleep_ms(300);
    
    matrix_animation(2000);
    wipe_effect(COLOR_BLACK, 500);
    radar_scan(center_x, center_y, 3000);
    wipe_effect(COLOR_BLACK, 300);
    
    draw_logo(logo_x, logo_y, COLOR_RED);
    display_update();
    sleep_ms(300);
    
    const char* text = "ArithOS";
    int text_y = center_y + (LOGO_HEIGHT * LOGO_SCALE) / 2 + 20;
    
    for (int alpha = 0; alpha <= 15; alpha++) {
        uint16_t text_color = ((alpha << 11) | (alpha << 7) | (alpha << 1));
        display_draw_text_aligned(center_x, text_y, text, FONT_LARGE, ALIGN_CENTER, text_color);
        display_update();
        sleep_ms(50);
    }
    
    display_draw_text_aligned(center_x, text_y + 30, "v1.0", FONT_MEDIUM, ALIGN_CENTER, COLOR_LIGHTGRAY);
    display_update();
    
    for (int i = 0; i < 3; i++) {
        sleep_ms(300);
        display_draw_text_aligned(center_x, text_y, text, FONT_LARGE, ALIGN_CENTER, COLOR_YELLOW);
        display_update();
        sleep_ms(300);
        display_draw_text_aligned(center_x, text_y, text, FONT_LARGE, ALIGN_CENTER, COLOR_WHITE);
        display_update();
    }
    
    sleep_ms(500);
}