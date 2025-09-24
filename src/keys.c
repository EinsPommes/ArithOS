#include "keys.h"
#include "board_pins.h"
#include <string.h>

static const uint8_t rows[] = {PIN_ROW0, PIN_ROW1, PIN_ROW2, PIN_ROW3};
static const uint8_t cols[] = {PIN_COL0, PIN_COL1, PIN_COL2, PIN_COL3};
#define NUM_ROWS 4
#define NUM_COLS 4

#define DEBOUNCE_MS 20
#define KEY_REPEAT_DELAY_MS 300
#define KEY_REPEAT_RATE_MS 80

static uint16_t stable_state = 0;
static uint16_t last_state = 0;
static absolute_time_t last_change;
static absolute_time_t key_down_time[16];
static bool key_repeat_sent[16];

#define KEY_EVENT_QUEUE_SIZE 16
static key_event_t event_queue[KEY_EVENT_QUEUE_SIZE];
static uint8_t queue_head = 0;
static uint8_t queue_tail = 0;

static uint16_t read_matrix(void);
static void enqueue_event(uint8_t key_code, key_event_type_t event_type);
static bool is_queue_full(void);

void keys_init(void) {
    for (int r = 0; r < NUM_ROWS; r++) {
        gpio_init(rows[r]);
        gpio_set_dir(rows[r], true);
        gpio_put(rows[r], 1);
    }
    
    for (int c = 0; c < NUM_COLS; c++) {
        gpio_init(cols[c]);
        gpio_set_dir(cols[c], false);
        gpio_pull_up(cols[c]);
    }
    
    last_change = get_absolute_time();
    memset(key_down_time, 0, sizeof(key_down_time));
    memset(key_repeat_sent, 0, sizeof(key_repeat_sent));
}

static uint16_t read_matrix(void) {
    uint16_t bits = 0;
    
    for (int r = 0; r < NUM_ROWS; r++) {
        for (int rr = 0; rr < NUM_ROWS; rr++) {
            gpio_put(rows[rr], rr == r ? 0 : 1);
        }
        
        sleep_us(5);
        
        for (int c = 0; c < NUM_COLS; c++) {
            bool pressed = !gpio_get(cols[c]);
            if (pressed) {
                bits |= (1u << (r * NUM_COLS + c));
            }
        }
    }
    
    for (int r = 0; r < NUM_ROWS; r++) {
        gpio_put(rows[r], 1);
    }
    
    return bits;
}

void keys_poll(void) {
    uint16_t now = read_matrix();
    absolute_time_t current_time = get_absolute_time();
    
    if (now != last_state) {
        last_state = now;
        last_change = current_time;
    }
    
    if (absolute_time_diff_us(current_time, last_change) <= -DEBOUNCE_MS * 1000) {
        if (stable_state != now) {
            uint16_t changes = stable_state ^ now;
            uint16_t new_presses = changes & now;
            uint16_t new_releases = changes & stable_state;
            
            for (int i = 0; i < 16; i++) {
                if (new_presses & (1u << i)) {
                    enqueue_event(i, KEY_EVENT_PRESSED);
                    key_down_time[i] = current_time;
                    key_repeat_sent[i] = false;
                }
                else if (new_releases & (1u << i)) {
                    enqueue_event(i, KEY_EVENT_RELEASED);
                }
            }
            
            stable_state = now;
        }
    }
    
    for (int i = 0; i < 16; i++) {
        if (stable_state & (1u << i)) {
            int64_t down_time_us = absolute_time_diff_us(key_down_time[i], current_time);
            
            if (!key_repeat_sent[i] && down_time_us < -KEY_REPEAT_DELAY_MS * 1000) {
                enqueue_event(i, KEY_EVENT_REPEAT);
                key_repeat_sent[i] = true;
                key_down_time[i] = current_time;
            }
            else if (key_repeat_sent[i] && down_time_us < -KEY_REPEAT_RATE_MS * 1000) {
                enqueue_event(i, KEY_EVENT_REPEAT);
                key_down_time[i] = current_time;
            }
        }
    }
}

static void enqueue_event(uint8_t key_code, key_event_type_t event_type) {
    if (is_queue_full()) {
        return;
    }
    
    event_queue[queue_tail].key_code = key_code;
    event_queue[queue_tail].event_type = event_type;
    
    queue_tail = (queue_tail + 1) % KEY_EVENT_QUEUE_SIZE;
}

bool keys_get_event(key_event_t *event) {
    if (queue_head == queue_tail) {
        return false;
    }
    
    *event = event_queue[queue_head];
    queue_head = (queue_head + 1) % KEY_EVENT_QUEUE_SIZE;
    
    return true;
}

static bool is_queue_full(void) {
    return ((queue_tail + 1) % KEY_EVENT_QUEUE_SIZE) == queue_head;
}

bool keys_is_pressed(uint8_t key_code) {
    if (key_code >= 16) {
        return false;
    }
    return (stable_state & (1u << key_code)) != 0;
}

uint16_t keys_get_raw_state(void) {
    return stable_state;
}