#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct app_t app_t;

typedef void (*app_init_func)(app_t* app);
typedef void (*app_update_func)(app_t* app);
typedef void (*app_render_func)(app_t* app);
typedef void (*app_process_key_func)(app_t* app, uint8_t key_code);
typedef void (*app_destroy_func)(app_t* app);

struct app_t {
    const char* name;
    const char* icon;
    void* app_data;
    app_init_func init;
    app_update_func update;
    app_render_func render;
    app_process_key_func process_key;
    app_destroy_func destroy;
};

typedef app_t* (*app_register_func)(void);

void app_manager_init(void);
void app_manager_register_app(app_register_func register_func);
app_t* app_manager_get_current_app(void);
void app_manager_switch_to_app(int app_index);
void app_manager_launch_app(const char* app_name);
void app_manager_return_to_launcher(void);
int app_manager_get_app_count(void);
app_t* app_manager_get_app(int index);