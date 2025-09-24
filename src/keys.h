#pragma once
#include <stdint.h>
#include "pico/stdlib.h"

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