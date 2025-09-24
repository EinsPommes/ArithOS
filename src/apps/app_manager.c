#include "app.h"
#include <string.h>
#include <stdlib.h>

#define MAX_APPS 10

static app_t* apps[MAX_APPS];
static int app_count = 0;
static int current_app_index = -1;

extern app_t* register_launcher_app(void);
extern app_t* register_calculator_app(void);
extern app_t* register_stopwatch_app(void);
extern app_t* register_wifi_scanner_app(void);
extern app_t* register_badusb_app(void);
extern app_t* register_packet_sniffer_app(void);
extern app_t* register_bruteforce_app(void);
extern app_t* register_code_editor_app(void);

void app_manager_init(void) {
    memset(apps, 0, sizeof(apps));
    app_count = 0;
    current_app_index = -1;
    
    app_manager_register_app(register_launcher_app);
    app_manager_register_app(register_calculator_app);
    app_manager_register_app(register_stopwatch_app);
    app_manager_register_app(register_wifi_scanner_app);
    app_manager_register_app(register_badusb_app);
    app_manager_register_app(register_packet_sniffer_app);
    app_manager_register_app(register_bruteforce_app);
    app_manager_register_app(register_code_editor_app);
    
    current_app_index = 0;
    if (apps[current_app_index] && apps[current_app_index]->init) {
        apps[current_app_index]->init(apps[current_app_index]);
    }
}

void app_manager_register_app(app_register_func register_func) {
    if (app_count >= MAX_APPS || !register_func) {
        return;
    }
    
    app_t* app = register_func();
    if (app) {
        apps[app_count++] = app;
    }
}

app_t* app_manager_get_current_app(void) {
    if (current_app_index >= 0 && current_app_index < app_count) {
        return apps[current_app_index];
    }
    return NULL;
}

void app_manager_switch_to_app(int app_index) {
    if (app_index < 0 || app_index >= app_count) {
        return;
    }
    
    if (current_app_index >= 0 && current_app_index < app_count) {
        if (apps[current_app_index]->destroy) {
            apps[current_app_index]->destroy(apps[current_app_index]);
        }
    }
    
    current_app_index = app_index;
    
    if (apps[current_app_index]->init) {
        apps[current_app_index]->init(apps[current_app_index]);
    }
}

void app_manager_launch_app(const char* app_name) {
    for (int i = 0; i < app_count; i++) {
        if (strcmp(apps[i]->name, app_name) == 0) {
            app_manager_switch_to_app(i);
            return;
        }
    }
}

void app_manager_return_to_launcher(void) {
    app_manager_switch_to_app(0);
}

int app_manager_get_app_count(void) {
    return app_count;
}

app_t* app_manager_get_app(int index) {
    if (index >= 0 && index < app_count) {
        return apps[index];
    }
    return NULL;
}