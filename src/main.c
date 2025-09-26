#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "board_pins.h"
#include "keys.h"
#include "display.h"
#include "apps/app.h"
#include "boot_animation.h"

int main() {
    stdio_init_all();
    
    display_init();
    
    // Show boot animation
    show_boot_animation();
    
    keys_init();
    app_manager_init();
    
    absolute_time_t next_key_scan = get_absolute_time();
    absolute_time_t next_frame = get_absolute_time();
    
    while (true) {
        if (absolute_time_diff_us(get_absolute_time(), next_key_scan) <= 0) {
            keys_poll();
            next_key_scan = delayed_by_ms(get_absolute_time(), 10);
        }
        
        if (absolute_time_diff_us(get_absolute_time(), next_frame) <= 0) {
            app_t* current_app = app_manager_get_current_app();
            
            if (current_app) {
                key_event_t event;
                while (keys_get_event(&event)) {
                    if (event.event_type == KEY_EVENT_PRESSED && current_app->process_key) {
                        current_app->process_key(current_app, event.key_code);
                    }
                }
                
                if (current_app->update) {
                    current_app->update(current_app);
                }
                
                display_clear(COLOR_BLACK);
                if (current_app->render) {
                    current_app->render(current_app);
                }
                display_update();
            }
            
            next_frame = delayed_by_ms(get_absolute_time(), 33);
        }
        
        tight_loop_contents();
    }
    
    return 0;
}