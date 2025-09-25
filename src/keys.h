#pragma once
#include <stdint.h>
#include "pico/stdlib.h"

// Key codes
#define KEY_0     0
#define KEY_1     1
#define KEY_2     2
#define KEY_3     3
#define KEY_4     4
#define KEY_5     5
#define KEY_6     6
#define KEY_7     7
#define KEY_8     8
#define KEY_9     9
#define KEY_PLUS      10
#define KEY_MINUS     11
#define KEY_MULTIPLY  12
#define KEY_DIVIDE    13
#define KEY_EQUAL     14
#define KEY_DOT       15
#define KEY_ENTER     16
#define KEY_CLEAR     17
#define KEY_BACKSPACE 18
#define KEY_ESC       19

typedef enum {
    KEY_EVENT_PRESSED,
    KEY_EVENT_RELEASED,
    KEY_EVENT_REPEAT
} key_event_type_t;

typedef struct {
    uint8_t key_code;
    key_event_type_t event_type;
} key_event_t;

void keys_init(void);
void keys_poll(void);
bool keys_get_event(key_event_t *event);
bool keys_is_pressed(uint8_t key_code);
uint16_t keys_get_raw_state(void);