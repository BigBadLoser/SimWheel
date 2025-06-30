
#include "leds.h"
#include <string.h>
#include <stdio.h>
#include "commands.h"
#include <stdlib.h>

#define UPPER_NUM_LEDS 25
#define LOWER_NUM_LEDS 4

static bool led_dirty = true;
static uint32_t upper_led_colors[UPPER_NUM_LEDS];
static uint32_t lower_led_colors[LOWER_NUM_LEDS] = {0};
static float led_brightness = 0.5f;
// Aliases
typedef struct {
    const char* name;
    int start;
    int count;
} LedAlias;

typedef struct {
    bool active;
    uint32_t color;
    int phase;
    const LedAlias* alias;
} LedAnimation;

#define MAX_ANIMATIONS 3
static LedAnimation animations[MAX_ANIMATIONS];

static const LedAlias upper_led_aliases[] = {
    { "status_left",   0, 2 },
    { "bar_left",      2, 3 },
    { "tach",          5, 15 },
    { "bar_right",    20, 3 },
    { "status_right", 23, 2 }
};

#define NUM_LED_ALIASES (sizeof(upper_led_aliases)/sizeof(LedAlias))

void set_upper_led(int index, uint32_t color) {
    if (index >= 0 && index < UPPER_NUM_LEDS) {
        upper_led_colors[index] = color;
    }
}

void set_lower_led(int index, uint32_t color) {
    if (index >= 0 && index < LOWER_NUM_LEDS) {
        lower_led_colors[index] = color;
    }
}

void set_upper_led_group(const char* name, uint32_t color) {
    for (int i = 0; i < NUM_LED_ALIASES; i++) {
        if (strcmp(name, upper_led_aliases[i].name) == 0) {
            for (int j = 0; j < upper_led_aliases[i].count; j++) {
                set_upper_led(upper_led_aliases[i].start + j, color);
            }
            c_set_led_dirty();
            break;
        }
    }
}

uint32_t* get_upper_led_buffer(void) {
    return upper_led_colors;
}

int get_upper_led_count(void) {
    return UPPER_NUM_LEDS;
}

uint32_t* get_lower_led_buffer(void) {
    return lower_led_colors;
}
int get_lower_led_count(void) {
    return LOWER_NUM_LEDS;
}
// Animation state
static struct {
    bool active;
    uint32_t color;
    int phase;
} anim = { false, 0, 0 };

void leds_start_chase(uint32_t color) {
    anim.active = true;
    anim.color = color;
    anim.phase = 0;
}

void leds_stop_animation(void) {
    anim.active = false;
}

void leds_tick_animation(void) {
    for (int i = 0; i < MAX_ANIMATIONS; i++) {
        if (!animations[i].active) continue;

        const LedAlias* a = animations[i].alias;
        animations[i].phase = (animations[i].phase + 1) % a->count;

        for (int j = 0; j < a->count; j++) {
            int idx = a->start + j;
            uint32_t color = (j == animations[i].phase) ? animations[i].color : 0;
            set_upper_led(idx, color);
        }
    }

    c_set_led_dirty();
}

void leds_init(void) {
    memset(upper_led_colors, 0, sizeof(upper_led_colors));
    anim.active = false;
}



void c_show_lower_leds(PIO pio, uint sm) {
    const uint32_t* colors = lower_led_colors;
    for (int i = 0; i < LOWER_NUM_LEDS; i++) {
        pio_sm_put_blocking(pio, sm, colors[i] & 0xFFFFFF);
    }
}

void c_show_upper_leds(PIO pio, uint sm1) {
    const uint32_t* colors = upper_led_colors;
    for (int i = 0; i < UPPER_NUM_LEDS; i++) {
        pio_sm_put_blocking(pio, sm1, colors[i] & 0xFFFFFF);
    }
}

static uint32_t hex_to_rgb(const char* hex, float brightness) {
    if (hex[0] == '#') hex++;
    unsigned int r, g, b;
    sscanf(hex, "%02x%02x%02x", &r, &g, &b);
    r = (uint8_t)(r * brightness);
    g = (uint8_t)(g * brightness);
    b = (uint8_t)(b * brightness);
    return (b << 16) | (g << 8) | r;
}

bool c_get_led_dirty(void) {
    return led_dirty;
}

void c_set_led_dirty(void) {
    led_dirty = true;
}

void c_clear_led_dirty(void) {
    led_dirty = false;
}

void leds_start_group_chase(const char* group_name, uint32_t color) {
    for (int i = 0; i < NUM_LED_ALIASES; i++) {
        if (strcmp(upper_led_aliases[i].name, group_name) == 0) {
            for (int j = 0; j < MAX_ANIMATIONS; j++) {
                if (!animations[j].active) {
                    animations[j].active = true;
                    animations[j].color = color;
                    animations[j].phase = 0;
                    animations[j].alias = &upper_led_aliases[i];
                    return;
                }
            }
        }
    }
}

void leds_stop_all_animations(void) {
    for (int i = 0; i < MAX_ANIMATIONS; i++) {
        animations[i].active = false;
    }
}