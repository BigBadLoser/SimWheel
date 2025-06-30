#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "hardware/pio.h"

void leds_init(void);
void leds_tick_animation(void);
void leds_start_chase(uint32_t color);
void leds_stop_animation(void);
void leds_stop_all_animations(void);
void leds_start_group_chase(const char* group_name, uint32_t color);

void set_upper_led(int index, uint32_t color);
void set_upper_led_group(const char* name, uint32_t color);
uint32_t* get_upper_led_buffer(void);
int get_upper_led_count(void);

void set_lower_led(int index, uint32_t color);
void set_lower_led_group(const char* name, uint32_t color);
uint32_t* get_lower_led_buffer(void);
int get_lower_led_count(void);

void c_show_lower_leds(PIO pio, uint sm);
void c_show_upper_leds(PIO pio, uint sm);

bool c_get_led_dirty(void);
void c_set_led_dirty(void);
void c_clear_led_dirty(void);

