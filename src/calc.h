#pragma once
#include <stdbool.h>

#define CALC_MAX_INPUT_LENGTH 32

typedef enum {
    OP_NONE,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE
} calc_operation_t;

typedef struct {
    char input[CALC_MAX_INPUT_LENGTH + 1];
    int input_position;
    double current_value;
    double stored_value;
    calc_operation_t operation;
    bool has_decimal;
    bool error_state;
    bool result_displayed;
} calc_state_t;

void calc_init(void);
void calc_process_key(uint8_t key_code);
calc_state_t* calc_get_state(void);
void calc_clear(void);
void calc_clear_entry(void);
void calc_calculate(void);
const char* calc_get_display_string(void);