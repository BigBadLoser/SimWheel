#include <stdbool.h>
#include "hardware/pio.h"
#ifndef COMMANDS_H
#define COMMANDS_H



void register_all_commands(void);
void dispatch_json_command(const char* json);
void c_show_leds(PIO pio, uint sm);
void c_show_upper_leds(PIO pio, uint sm);
void c_clear_led_dirty(void);
bool c_led_dirty(void);

#endif // COMMANDS_H