#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "tusb.h"
#include "commands.h"
#include "hardware/pio.h"
#include "ws2812.pio.h"  
#include "leds.h"       
#define MAX_COMMANDS 8


typedef void (*json_handler_fn)(const char* json);

typedef struct {
    const char* type;
    json_handler_fn handler;
} json_route_t;

static json_route_t command_table[MAX_COMMANDS];
static int command_count = 0;


typedef struct {
    char type[16]; //"led", "status"
} base_command_t;

typedef struct {
    int index;
    char color[10];
    float brightness;
} led_command_t;


static uint32_t hex_to_rgb(const char* hex, float brightness) {
    if (hex[0] == '#') hex++;
    unsigned int r, g, b;
    sscanf(hex, "%02x%02x%02x", &r, &g, &b);
    r = (uint8_t)(r * brightness);
    g = (uint8_t)(g * brightness);
    b = (uint8_t)(b * brightness);
    return (b << 16) | (g << 8) | r;
}

bool extract_json_value(const char* json, const char* key, char* out, size_t maxlen) {
    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\":", key);
    char* start = strstr(json, pattern);
    if (!start) return false;

    start += strlen(pattern);

    // Skip whitespace
    while (*start == ' ' || *start == '\t') start++;

    if (*start == '"') {
        // It's a quoted string
        start++;
        char* end = strchr(start, '"');
        if (!end || end - start >= maxlen) return false;
        strncpy(out, start, end - start);
        out[end - start] = '\0';
    } else {
        // It's a number, bool, or null
        char* end = start;
        while (*end && *end != ',' && *end != '}' && *end != ' ' && *end != '\n' && *end != '\r')
            end++;
        if (end - start >= maxlen) return false;
        strncpy(out, start, end - start);
        out[end - start] = '\0';
    }

    return true;
}

bool parse_base_command(const char* json, base_command_t* cmd) {
    if (!extract_json_value(json, "type", cmd->type, sizeof(cmd->type))) {
        return false;
    }
    return true;
}

bool parse_led_command(const char* raw, led_command_t* out) {
    char buf[32];

    if (extract_json_value(raw, "index", buf, sizeof(buf)))
        out->index = atoi(buf);

    if (extract_json_value(raw, "color", buf, sizeof(buf)))
        strncpy(out->color, buf, sizeof(out->color));

    if (extract_json_value(raw, "brightness", buf, sizeof(buf)))
        out->brightness = strtof(buf, NULL);

    return true;
}

static void handle_led_command(const char* json) {
    base_command_t base;
    if(!parse_base_command(json, &base)) {
        tud_cdc_write_str("CMD ERROR: Missing 'type'\r\n");
        tud_cdc_write_flush();
        return;
    }
    led_command_t cmd = {0};
    if (!parse_led_command(json, &cmd)) {
        tud_cdc_write_str("CMD ERROR: Invalid LED command format. \r\n");
        tud_cdc_write_flush();
        return;
    }
    uint32_t color = hex_to_rgb(cmd.color, cmd.brightness);

    //LED ALIAS COMMAND HANDLING
    char alias[16];
    if (extract_json_value(json, "alias", alias, sizeof(alias))) {
        uint32_t color = hex_to_rgb(cmd.color, cmd.brightness);
        if (strcmp(base.type, "upper_led") == 0) {
            set_upper_led_group(alias, color);
            c_set_led_dirty();
            tud_cdc_write_str("CMD OK: Upper LED group updated.\r\n");
            tud_cdc_write_flush();
        } else {
            tud_cdc_write_str("CMD ERROR: Aliases only supported for upper_led.\r\n");
            tud_cdc_write_flush();
        }
        return;
    }
    //INDIVIDUAL LED COMMAND HANDLING
    if (strcmp(base.type, "upper_led") == 0 && cmd.index < get_upper_led_count()) {
        set_upper_led(cmd.index, color);
        c_set_led_dirty();
        tud_cdc_write_str("CMD OK: Upper LED updated. \r\n");
    } else if (strcmp(base.type, "lower_led") == 0 && cmd.index < get_lower_led_count()) {
        set_lower_led(cmd.index, color);
        c_set_led_dirty();
        tud_cdc_write_str("CMD OK: Lower LED updated. \r\n");
        tud_cdc_write_flush();
    } else {
        tud_cdc_write_str("CMD ERROR: Invalid LED index. \r\n");
        tud_cdc_write_flush();
    }

}



static void handle_start_chase(const char* json) {
    char color_str[10];
    if (!extract_json_value(json, "color", color_str, sizeof(color_str))) {
        tud_cdc_write_str("CMD ERROR: Missing color for chase.\r\n");
        tud_cdc_write_flush();
        return;
    }
    uint32_t color = hex_to_rgb(color_str, 1.0f);
    leds_start_chase(color);
    tud_cdc_write_str("CMD OK: Chase animation started.\r\n");
    tud_cdc_write_flush();
}

static void handle_stop_chase(const char* json) {
    leds_stop_animation();
    tud_cdc_write_str("CMD OK: Chase animation stopped.\r\n");
    tud_cdc_write_flush();
}
static void handle_animation_command(const char* json) {
    char group[32] = {0};
    char color_hex[10] = {0};

    if (!extract_json_value(json, "type", group, sizeof(group))) {
        tud_cdc_write_str("CMD ERROR: Missing 'type'\r\n");
        tud_cdc_write_flush();
        return;
    }

    if (strcmp(group, "stop_anim") == 0) {
        leds_stop_all_animations();
        tud_cdc_write_str("CMD OK: Animations stopped\r\n");
        tud_cdc_write_flush();
        return;
    }

    if (!extract_json_value(json, "group", group, sizeof(group))) {
        tud_cdc_write_str("CMD ERROR: Missing 'group' for animation\r\n");
        tud_cdc_write_flush();
        return;
    }

    if (!extract_json_value(json, "color", color_hex, sizeof(color_hex))) {
        tud_cdc_write_str("CMD ERROR: Missing 'color' for animation\r\n");
        tud_cdc_write_flush();
        return;
    }

    uint32_t color = hex_to_rgb(color_hex, 1.0f);
    leds_start_group_chase(group, color);

    tud_cdc_write_str("CMD OK: Animation started\r\n");
    tud_cdc_write_flush();
}


void dispatch_json_command(const char* json) {
    base_command_t base;
    if (!parse_base_command(json, &base)) {
        tud_cdc_write_str("CMD ERROR: Missing 'type'\r\n");
        tud_cdc_write_flush();
        return;
    }

    for (int i = 0; i < command_count; i++) {
        if (strcmp(base.type, command_table[i].type) == 0) {
            command_table[i].handler(json);
            return;
        }
    }

    tud_cdc_write_str("CMD ERROR: Unknown command type\r\n");
    tud_cdc_write_flush();
}

void register_json_handler(const char* type, json_handler_fn handler) {
    if (command_count < MAX_COMMANDS) {
        command_table[command_count++] = (json_route_t){type, handler};
    }
}

void register_all_commands(void) {
    register_json_handler("upper_led", handle_led_command);
    register_json_handler("lower_led", handle_led_command);
    register_json_handler("start_chase", handle_start_chase);
    register_json_handler("stop_chase", handle_stop_chase);
    register_json_handler("start_anim", handle_animation_command);
    register_json_handler("stop_anim", handle_animation_command);
}
