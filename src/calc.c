#include "calc.h"
#include "board_pins.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

static calc_state_t calc_state;

static void update_display_string(void);
static void handle_digit(uint8_t digit);
static void handle_operation(calc_operation_t op);
static void handle_equals(void);

void calc_init(void) {
    calc_clear();
}

void calc_process_key(uint8_t key_code) {
    if (calc_state.result_displayed) {
        if (key_code >= KEY_0 && key_code <= KEY_9) {
            calc_clear();
        } else if (key_code != KEY_PLUS && key_code != KEY_MINUS && 
                   key_code != KEY_MULT && key_code != KEY_DIV) {
            calc_state.result_displayed = false;
        }
    }
    
    switch (key_code) {
        case KEY_0:
        case KEY_1:
        case KEY_2:
        case KEY_3:
        case KEY_4:
        case KEY_5:
        case KEY_6:
        case KEY_7:
        case KEY_8:
        case KEY_9:
            handle_digit(key_code);
            break;
            
        case KEY_PLUS:
            handle_operation(OP_ADD);
            break;
            
        case KEY_MINUS:
            handle_operation(OP_SUBTRACT);
            break;
            
        case KEY_MULT:
            handle_operation(OP_MULTIPLY);
            break;
            
        case KEY_DIV:
            handle_operation(OP_DIVIDE);
            break;
            
        case KEY_EQUAL:
            handle_equals();
            break;
            
        case KEY_CLEAR:
            calc_clear();
            break;
            
        default:
            break;
    }
    
    update_display_string();
}

calc_state_t* calc_get_state(void) {
    return &calc_state;
}

void calc_clear(void) {
    memset(&calc_state, 0, sizeof(calc_state));
    strcpy(calc_state.input, "0");
    calc_state.input_position = 1;
    calc_state.current_value = 0;
    calc_state.stored_value = 0;
    calc_state.operation = OP_NONE;
    calc_state.has_decimal = false;
    calc_state.error_state = false;
    calc_state.result_displayed = false;
}

void calc_clear_entry(void) {
    strcpy(calc_state.input, "0");
    calc_state.input_position = 1;
    calc_state.current_value = 0;
    calc_state.has_decimal = false;
    calc_state.result_displayed = false;
}

void calc_calculate(void) {
    if (calc_state.error_state) {
        return;
    }
    
    calc_state.current_value = atof(calc_state.input);
    
    switch (calc_state.operation) {
        case OP_ADD:
            calc_state.stored_value += calc_state.current_value;
            break;
            
        case OP_SUBTRACT:
            calc_state.stored_value -= calc_state.current_value;
            break;
            
        case OP_MULTIPLY:
            calc_state.stored_value *= calc_state.current_value;
            break;
            
        case OP_DIVIDE:
            if (fabs(calc_state.current_value) < 1e-10) {
                calc_state.error_state = true;
                strcpy(calc_state.input, "Error");
                return;
            }
            calc_state.stored_value /= calc_state.current_value;
            break;
            
        case OP_NONE:
            calc_state.stored_value = calc_state.current_value;
            break;
    }
    
    if (fabs(calc_state.stored_value) > 1e10 || 
        (fabs(calc_state.stored_value) < 1e-10 && calc_state.stored_value != 0)) {
        snprintf(calc_state.input, CALC_MAX_INPUT_LENGTH, "%e", calc_state.stored_value);
    } else {
        snprintf(calc_state.input, CALC_MAX_INPUT_LENGTH, "%.10g", calc_state.stored_value);
    }
    
    calc_state.input_position = strlen(calc_state.input);
    calc_state.current_value = calc_state.stored_value;
    calc_state.has_decimal = (strchr(calc_state.input, '.') != NULL);
    calc_state.result_displayed = true;
}

const char* calc_get_display_string(void) {
    return calc_state.input;
}

static void update_display_string(void) {
    calc_state.input[CALC_MAX_INPUT_LENGTH] = '\0';
}

static void handle_digit(uint8_t digit) {
    if (calc_state.error_state) {
        return;
    }
    
    if (calc_state.input_position == 1 && calc_state.input[0] == '0' && digit != 0) {
        calc_state.input[0] = '0' + digit;
        return;
    }
    
    if (calc_state.input_position == 0 && digit == 0) {
        calc_state.input[0] = '0';
        calc_state.input_position = 1;
        return;
    }
    
    if (calc_state.input_position >= CALC_MAX_INPUT_LENGTH) {
        return;
    }
    
    calc_state.input[calc_state.input_position++] = '0' + digit;
    calc_state.input[calc_state.input_position] = '\0';
}

static void handle_operation(calc_operation_t op) {
    if (calc_state.error_state) {
        return;
    }
    
    if (calc_state.operation != OP_NONE && !calc_state.result_displayed) {
        calc_calculate();
    }
    
    calc_state.operation = op;
    calc_state.stored_value = atof(calc_state.input);
    
    strcpy(calc_state.input, "0");
    calc_state.input_position = 1;
    calc_state.has_decimal = false;
}

static void handle_equals(void) {
    if (calc_state.error_state) {
        return;
    }
    
    calc_calculate();
    calc_state.operation = OP_NONE;
}