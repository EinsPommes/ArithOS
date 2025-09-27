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

// Display dimensions - moved to display.h to avoid conflicts
// Key definitions - moved to keys.h to avoid conflicts