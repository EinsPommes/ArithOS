#include "display.h"
#include "board_pins.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include <string.h>
#include <stdlib.h>

#define ST7365P_NOP       0x00
#define ST7365P_SWRESET   0x01
#define ST7365P_SLPIN     0x10
#define ST7365P_SLPOUT    0x11
#define ST7365P_NORON     0x13
#define ST7365P_INVOFF    0x20
#define ST7365P_INVON     0x21
#define ST7365P_DISPOFF   0x28
#define ST7365P_DISPON    0x29
#define ST7365P_CASET     0x2A
#define ST7365P_RASET     0x2B
#define ST7365P_RAMWR     0x2C
#define ST7365P_COLMOD    0x3A
#define ST7365P_MADCTL    0x36

static const uint8_t font_5x7[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, // Space
    0x00, 0x00, 0x5F, 0x00, 0x00, // !
    0x00, 0x07, 0x00, 0x07, 0x00, // "
    0x14, 0x7F, 0x14, 0x7F, 0x14, // #
    0x24, 0x2A, 0x7F, 0x2A, 0x12, // $
    0x23, 0x13, 0x08, 0x64, 0x62, // %
    0x36, 0x49, 0x55, 0x22, 0x50, // &
    0x00, 0x05, 0x03, 0x00, 0x00, // '
    0x00, 0x1C, 0x22, 0x41, 0x00, // (
    0x00, 0x41, 0x22, 0x1C, 0x00, // )
    0x14, 0x08, 0x3E, 0x08, 0x14, // *
    0x08, 0x08, 0x3E, 0x08, 0x08, // +
    0x00, 0x50, 0x30, 0x00, 0x00, // ,
    0x08, 0x08, 0x08, 0x08, 0x08, // -
    0x00, 0x60, 0x60, 0x00, 0x00, // .
    0x20, 0x10, 0x08, 0x04, 0x02, // /
    0x3E, 0x51, 0x49, 0x45, 0x3E, // 0
    0x00, 0x42, 0x7F, 0x40, 0x00, // 1
    0x42, 0x61, 0x51, 0x49, 0x46, // 2
    0x21, 0x41, 0x45, 0x4B, 0x31, // 3
    0x18, 0x14, 0x12, 0x7F, 0x10, // 4
    0x27, 0x45, 0x45, 0x45, 0x39, // 5
    0x3C, 0x4A, 0x49, 0x49, 0x30, // 6
    0x01, 0x71, 0x09, 0x05, 0x03, // 7
    0x36, 0x49, 0x49, 0x49, 0x36, // 8
    0x06, 0x49, 0x49, 0x29, 0x1E, // 9
    0x00, 0x36, 0x36, 0x00, 0x00, // :
    0x00, 0x56, 0x36, 0x00, 0x00, // ;
    0x08, 0x14, 0x22, 0x41, 0x00, // <
    0x14, 0x14, 0x14, 0x14, 0x14, // =
    0x00, 0x41, 0x22, 0x14, 0x08, // >
    0x02, 0x01, 0x51, 0x09, 0x06  // ?
};

static bool display_initialized = false;

static void write_command(uint8_t cmd);
static void write_data(uint8_t data);
static void write_data16(uint16_t data);
static void set_address_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

void display_init(void) {
    if (display_initialized) {
        return;
    }
    
    spi_init(spi0, 40 * 1000 * 1000);
    gpio_set_function(PIN_LCD_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_LCD_MOSI, GPIO_FUNC_SPI);
    
    gpio_init(PIN_LCD_CS);
    gpio_init(PIN_LCD_DC);
    gpio_init(PIN_LCD_RST);
    gpio_set_dir(PIN_LCD_CS, GPIO_OUT);
    gpio_set_dir(PIN_LCD_DC, GPIO_OUT);
    gpio_set_dir(PIN_LCD_RST, GPIO_OUT);
    
    gpio_put(PIN_LCD_CS, 1);
    gpio_put(PIN_LCD_RST, 1);
    sleep_ms(10);
    gpio_put(PIN_LCD_RST, 0);
    sleep_ms(10);
    gpio_put(PIN_LCD_RST, 1);
    sleep_ms(120);
    
    gpio_put(PIN_LCD_CS, 0);
    
    write_command(ST7365P_SWRESET);
    sleep_ms(150);
    
    write_command(ST7365P_SLPOUT);
    sleep_ms(120);
    
    write_command(ST7365P_COLMOD);
    write_data(0x55);
    
    write_command(ST7365P_MADCTL);
    write_data(0x08);
    
    write_command(ST7365P_INVON);
    sleep_ms(10);
    
    write_command(ST7365P_NORON);
    sleep_ms(10);
    
    write_command(ST7365P_DISPON);
    sleep_ms(10);
    
    gpio_put(PIN_LCD_CS, 1);
    
    display_clear(COLOR_BLACK);
    
    display_initialized = true;
}

void display_clear(uint16_t color) {
    set_address_window(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);
    
    gpio_put(PIN_LCD_CS, 0);
    write_command(ST7365P_RAMWR);
    
    for (int i = 0; i < DISPLAY_WIDTH * DISPLAY_HEIGHT; i++) {
        write_data16(color);
    }
    
    gpio_put(PIN_LCD_CS, 1);
}

void display_draw_pixel(int16_t x, int16_t y, uint16_t color) {
    if (x < 0 || x >= DISPLAY_WIDTH || y < 0 || y >= DISPLAY_HEIGHT) {
        return;
    }
    
    set_address_window(x, y, x, y);
    
    gpio_put(PIN_LCD_CS, 0);
    write_command(ST7365P_RAMWR);
    write_data16(color);
    gpio_put(PIN_LCD_CS, 1);
}

void display_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    int16_t dx = abs(x1 - x0);
    int16_t sx = x0 < x1 ? 1 : -1;
    int16_t dy = -abs(y1 - y0);
    int16_t sy = y0 < y1 ? 1 : -1;
    int16_t err = dx + dy;
    int16_t e2;
    
    while (true) {
        display_draw_pixel(x0, y0, color);
        
        if (x0 == x1 && y0 == y1) {
            break;
        }
        
        e2 = 2 * err;
        
        if (e2 >= dy) {
            if (x0 == x1) {
                break;
            }
            err += dy;
            x0 += sx;
        }
        
        if (e2 <= dx) {
            if (y0 == y1) {
                break;
            }
            err += dx;
            y0 += sy;
        }
    }
}

void display_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    display_draw_line(x, y, x + w - 1, y, color);
    display_draw_line(x, y + h - 1, x + w - 1, y + h - 1, color);
    display_draw_line(x, y, x, y + h - 1, color);
    display_draw_line(x + w - 1, y, x + w - 1, y + h - 1, color);
}

void display_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT || w <= 0 || h <= 0) {
        return;
    }
    
    if (x < 0) {
        w += x;
        x = 0;
    }
    if (y < 0) {
        h += y;
        y = 0;
    }
    if (x + w > DISPLAY_WIDTH) {
        w = DISPLAY_WIDTH - x;
    }
    if (y + h > DISPLAY_HEIGHT) {
        h = DISPLAY_HEIGHT - y;
    }
    
    set_address_window(x, y, x + w - 1, y + h - 1);
    
    gpio_put(PIN_LCD_CS, 0);
    write_command(ST7365P_RAMWR);
    
    for (int i = 0; i < w * h; i++) {
        write_data16(color);
    }
    
    gpio_put(PIN_LCD_CS, 1);
}

void display_draw_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    int16_t x = r;
    int16_t y = 0;
    int16_t err = 0;
    
    while (x >= y) {
        display_draw_pixel(x0 + x, y0 + y, color);
        display_draw_pixel(x0 + y, y0 + x, color);
        display_draw_pixel(x0 - y, y0 + x, color);
        display_draw_pixel(x0 - x, y0 + y, color);
        display_draw_pixel(x0 - x, y0 - y, color);
        display_draw_pixel(x0 - y, y0 - x, color);
        display_draw_pixel(x0 + y, y0 - x, color);
        display_draw_pixel(x0 + x, y0 - y, color);
        
        y++;
        err += 1 + 2 * y;
        if (2 * (err - x) + 1 > 0) {
            x--;
            err += 1 - 2 * x;
        }
    }
}

void display_fill_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    int16_t x = r;
    int16_t y = 0;
    int16_t err = 0;
    
    while (x >= y) {
        display_draw_line(x0 - x, y0 + y, x0 + x, y0 + y, color);
        display_draw_line(x0 - y, y0 + x, x0 + y, y0 + x, color);
        display_draw_line(x0 - x, y0 - y, x0 + x, y0 - y, color);
        display_draw_line(x0 - y, y0 - x, x0 + y, y0 - x, color);
        
        y++;
        err += 1 + 2 * y;
        if (2 * (err - x) + 1 > 0) {
            x--;
            err += 1 - 2 * x;
        }
    }
}

void display_draw_text(int16_t x, int16_t y, const char* text, font_size_t size, uint16_t color) {
    display_draw_text_aligned(x, y, text, size, ALIGN_LEFT, color);
}

void display_draw_text_aligned(int16_t x, int16_t y, const char* text, font_size_t size, text_align_t align, uint16_t color) {
    if (!text || *text == '\0') {
        return;
    }
    
    uint8_t scale = 1;
    switch (size) {
        case FONT_SMALL:  scale = 1; break;
        case FONT_MEDIUM: scale = 2; break;
        case FONT_LARGE:  scale = 3; break;
    }
    
    int16_t text_width = display_get_text_width(text, size);
    
    switch (align) {
        case ALIGN_LEFT:   break;
        case ALIGN_CENTER: x -= text_width / 2; break;
        case ALIGN_RIGHT:  x -= text_width; break;
    }
    
    const uint8_t char_width = 5;
    const uint8_t char_height = 7;
    const uint8_t char_spacing = 1;
    
    while (*text) {
        char c = *text++;
        
        if (c < 32 || c > 126) {
            c = '?';
        }
        
        const uint8_t* char_data = &font_5x7[(c - 32) * char_width];
        
        for (uint8_t col = 0; col < char_width; col++) {
            uint8_t line = char_data[col];
            for (uint8_t row = 0; row < char_height; row++) {
                if (line & (1 << row)) {
                    if (scale == 1) {
                        display_draw_pixel(x + col, y + row, color);
                    } else {
                        display_fill_rect(x + col * scale, y + row * scale, scale, scale, color);
                    }
                }
            }
        }
        
        x += (char_width + char_spacing) * scale;
    }
}

int16_t display_get_text_width(const char* text, font_size_t size) {
    if (!text) {
        return 0;
    }
    
    uint8_t scale = 1;
    switch (size) {
        case FONT_SMALL:  scale = 1; break;
        case FONT_MEDIUM: scale = 2; break;
        case FONT_LARGE:  scale = 3; break;
    }
    
    const uint8_t char_width = 5;
    const uint8_t char_spacing = 1;
    
    return strlen(text) * (char_width + char_spacing) * scale - char_spacing * scale;
}

void display_update(void) {
}

static void write_command(uint8_t cmd) {
    gpio_put(PIN_LCD_DC, 0);
    spi_write_blocking(spi0, &cmd, 1);
}

static void write_data(uint8_t data) {
    gpio_put(PIN_LCD_DC, 1);
    spi_write_blocking(spi0, &data, 1);
}

static void write_data16(uint16_t data) {
    uint8_t data_bytes[2] = {(uint8_t)(data >> 8), (uint8_t)data};
    gpio_put(PIN_LCD_DC, 1);
    spi_write_blocking(spi0, data_bytes, 2);
}

static void set_address_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    gpio_put(PIN_LCD_CS, 0);
    
    write_command(ST7365P_CASET);
    write_data16(x0);
    write_data16(x1);
    
    write_command(ST7365P_RASET);
    write_data16(y0);
    write_data16(y1);
    
    gpio_put(PIN_LCD_CS, 1);
}