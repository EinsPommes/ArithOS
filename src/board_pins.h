#pragma once

// Display - ST7365P SPI
#define PIN_LCD_SCK   10
#define PIN_LCD_MOSI  11
#define PIN_LCD_DC    12
#define PIN_LCD_RST   13
#define PIN_LCD_CS    14

// Keymatrix (4x4)
#define PIN_ROW0      2
#define PIN_ROW1      3
#define PIN_ROW2      4
#define PIN_ROW3      5
#define PIN_COL0      6
#define PIN_COL1      7
#define PIN_COL2      8
#define PIN_COL3      9

// Buzzer (PWM)
#define PIN_BUZZER    15

// ESP8266/ESP32 UART
#define ESP_UART_ID   uart1
#define ESP_UART_TX   16
#define ESP_UART_RX   17

// Display dimensions
#define DISPLAY_WIDTH  240
#define DISPLAY_HEIGHT 240

// Key definitions
#define KEY_0       0x00
#define KEY_1       0x01
#define KEY_2       0x02
#define KEY_3       0x03
#define KEY_4       0x04
#define KEY_5       0x05
#define KEY_6       0x06
#define KEY_7       0x07
#define KEY_8       0x08
#define KEY_9       0x09
#define KEY_PLUS    0x0A
#define KEY_MINUS   0x0B
#define KEY_MULT    0x0C
#define KEY_DIV     0x0D
#define KEY_EQUAL   0x0E
#define KEY_CLEAR   0x0F