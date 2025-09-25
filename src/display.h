#pragma once
#include <stdint.h>

// Display dimensions
#define DISPLAY_WIDTH     320
#define DISPLAY_HEIGHT    240

#define COLOR_BLACK       0x0000
#define COLOR_WHITE       0xFFFF
#define COLOR_RED         0xF800
#define COLOR_GREEN       0x07E0
#define COLOR_BLUE        0x001F
#define COLOR_CYAN        0x07FF
#define COLOR_MAGENTA     0xF81F
#define COLOR_YELLOW      0xFFE0
#define COLOR_ORANGE      0xFD20
#define COLOR_GRAY        0x8410
#define COLOR_DARKGRAY    0x4208
#define COLOR_LIGHTGRAY   0xC618

typedef enum {
    FONT_SMALL,
    FONT_MEDIUM,
    FONT_LARGE
} font_size_t;

typedef enum {
    ALIGN_LEFT,
    ALIGN_CENTER,
    ALIGN_RIGHT
} text_align_t;

void display_init(void);
void display_clear(uint16_t color);
void display_draw_pixel(int16_t x, int16_t y, uint16_t color);
void display_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void display_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void display_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void display_draw_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
void display_fill_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
void display_draw_text(int16_t x, int16_t y, const char* text, font_size_t size, uint16_t color);
void display_draw_text_aligned(int16_t x, int16_t y, const char* text, font_size_t size, text_align_t align, uint16_t color);
int16_t display_get_text_width(const char* text, font_size_t size);
void display_update(void);