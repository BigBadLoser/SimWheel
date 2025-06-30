#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "gpio.h"
#include "hardware/regs/io_bank0.h"
#include "hardware/structs/ioqspi.h"
#include "hardware/structs/sio.h"

//LR ENCODER A PINS: 5, 2, 18, 14, 12, 8
//LR ENCODER B PINS: 7, 4, 17, 16, 9, 6
//DB ENCODER A PINS: 41, 39, 38, 32
//DB ENCODER B PINS: 43, 37, 40, 30

const uint8_t encoder_A_pins[ENCODER_COUNT] = {5, 2, 18, 14, 12, 8, 41, 39, 38, 32};
const uint8_t encoder_B_pins[ENCODER_COUNT] = {7, 4, 17, 16, 9, 6, 43, 37, 40, 30};
int8_t encoder_deltas[ENCODER_COUNT] = { 0 };
uint8_t encoder_last_state[ENCODER_COUNT] = { 0 };

const uint8_t button_pins[BUTTON_COUNT] = { 46, 45, 42, 44, 33, 34, 35, 31}; //46, 45, 42, 44, 33, 34, 35, 31
const uint8_t toggle_pins[TOGGLE_COUNT] = { 11, 29};
const uint8_t shift_pins[SHIFT_COUNT] = { 19, 13};




// === Internal Struct ===
InputReport input_report;
static int last_encoder_state[ENCODER_COUNT];

// === Utility: Hat Switch Decode ===
const char* decode_hat_adc(uint16_t raw) {
    /*
    if (raw > 2980) return "down";
    else if (raw > 2950) return "left";
    else if (raw > 2870) return "up";
    else if (raw > 2815) return "right";
    else if (raw > 2700)  return "none";
    else return "unknown";
    */
   return "unknown";
}

bool input_report_changed(const InputReport* a, const InputReport* b) {
    if (memcmp(a->buttons, b->buttons, sizeof(a->buttons)) != 0) return true;
    if (memcmp(a->toggles, b->toggles, sizeof(a->toggles)) != 0) return true;
    if (memcmp(a->shifters, b->shifters, sizeof(a->shifters)) != 0) return true;
    if (memcmp(a->encoders, b->encoders, sizeof(a->encoders)) != 0) return true;
    if ((a->hat_button && !b->hat_button) || (!a->hat_button && b->hat_button)) return true;
    if ((a->hat && !b->hat) || (!a->hat && b->hat)) return true;
    if (a->hat && b->hat && strcmp(a->hat, b->hat) != 0) return true;
    
    return false;
}
/*
bool input_report_changed(const InputReport* a, const InputReport* b) {
    if (a->hat != b->hat && (!a->hat || !b->hat || strcmp(a->hat, b->hat) != 0))
        return true;

    for (int i = 0; i < BUTTON_COUNT; ++i)
        if (a->buttons[i] != b->buttons[i]) return true;

    for (int i = 0; i < TOGGLE_COUNT; ++i)
        if (a->toggles[i] != b->toggles[i]) return true;

    for (int i = 0; i < SHIFT_COUNT; ++i)
        if (a->shifters[i] != b->shifters[i]) return true;

    for (int i = 0; i < ENCODER_COUNT; ++i)
        if (a->encoders[i] != b->encoders[i]) return true;

    return false;
}
*/

// === Quadrature Decoder ===
void update_encoder_values(int encoders[ENCODER_COUNT]) {
    for (int i = 0; i < ENCODER_COUNT; i++) {
        int a = gpio_get(encoder_A_pins[i]);
        int b = gpio_get(encoder_B_pins[i]);
        int current = (a << 1) | b;
        int last = last_encoder_state[i];
        int delta = 0;

        if ((last == 0b00 && current == 0b01) ||
            (last == 0b01 && current == 0b11) ||
            (last == 0b11 && current == 0b10) ||
            (last == 0b10 && current == 0b00)) {
            delta = 1;
        } else if ((last == 0b00 && current == 0b10) ||
                   (last == 0b10 && current == 0b11) ||
                   (last == 0b11 && current == 0b01) ||
                   (last == 0b01 && current == 0b00)) {
            delta = -1;
        }

        encoders[i] += delta;
        last_encoder_state[i] = current;
    }
}

// === Setup GPIOs ===
void gpio_inputs_init(void) {
    //Enable all momentary push buttons
    for (int i = 0; i < BUTTON_COUNT; i++) gpio_init(button_pins[i]), gpio_set_dir(button_pins[i], false), gpio_pull_up(button_pins[i]);
    //Enable all toggle switches
    for (int i = 0; i < TOGGLE_COUNT; i++) gpio_init(toggle_pins[i]), gpio_set_dir(toggle_pins[i], false);
    //Enable the 2 shift buttons
    for (int i = 0; i < SHIFT_COUNT; i++) gpio_init(shift_pins[i]), gpio_set_dir(shift_pins[i], false), gpio_pull_up(shift_pins[i]);
    //Enable the encoders
    for (int i = 0; i < ENCODER_COUNT; i++) {
        gpio_init(encoder_A_pins[i]); gpio_set_dir(encoder_A_pins[i], false); gpio_pull_up(encoder_A_pins[i]);
        gpio_init(encoder_B_pins[i]); gpio_set_dir(encoder_B_pins[i], false); gpio_pull_up(encoder_B_pins[i]);
        last_encoder_state[i] = (gpio_get(encoder_A_pins[i]) << 1) | gpio_get(encoder_B_pins[i]);
    }
    //Enable the hat switch push button and ADC
    gpio_init(HAT_BUTTON_PIN); gpio_set_dir(HAT_BUTTON_PIN, false); gpio_pull_down(HAT_BUTTON_PIN);
    adc_init();
    gpio_set_function(HAT_DIR_PIN, 0);
    gpio_disable_pulls(HAT_DIR_PIN);  // disables both pull-up/down
}

// === Poll All GPIOs and Update Struct ===
void update_input_report(void) {
    printf("Shifter state: %d %d\n", input_report.buttons[0], input_report.shifters[1]);
    for (int i = 0; i < BUTTON_COUNT; i++) input_report.buttons[i] = gpio_get(button_pins[i]) == 0;
    for (int i = 0; i < TOGGLE_COUNT; i++) input_report.toggles[i] = gpio_get(toggle_pins[i]);
    for (int i = 0; i < SHIFT_COUNT; i++) input_report.shifters[i] = gpio_get(shift_pins[i]) == 0;
    update_encoder_values(input_report.encoders);
    adc_select_input(HAT_ADC_CHANNEL);
    uint16_t raw = adc_read();
    input_report.hat = decode_hat_adc(raw);
    input_report.hat_adc = raw;
    input_report.hat_button = gpio_get(HAT_BUTTON_PIN) == 0;
}

// === JSON Constructor ===


void build_input_report_json(const InputReport* report, char* out, size_t max_len) {
    int pos = 0;

    #define APPEND(fmt, ...) \
        do { \
            int n = snprintf(out + pos, max_len - pos, fmt, ##__VA_ARGS__); \
            if (n < 0 || n >= max_len - pos) return; \
            pos += n; \
        } while (0)

    APPEND("{");

    APPEND("\"buttons\":[");
    for (int i = 0; i < BUTTON_COUNT; i++) {
        APPEND("%s%d", i > 0 ? "," : "", report->buttons[i]);
    }
    APPEND("],");

    APPEND("\"toggles\":[");
    for (int i = 0; i < TOGGLE_COUNT; i++) {
        APPEND("%s%d", i > 0 ? "," : "", report->toggles[i]);
    }
    APPEND("],");

    APPEND("\"shifters\":[");
    for (int i = 0; i < SHIFT_COUNT; i++) {
        APPEND("%s%d", i > 0 ? "," : "", report->shifters[i]);
    }
    APPEND("],");

    APPEND("\"encoders\":[");
    for (int i = 0; i < ENCODER_COUNT; i++) {
        APPEND("%s%d", i > 0 ? "," : "", report->encoders[i]);
    }
    APPEND("],");

    APPEND("\"hat\":\"%s\"", report->hat ? report->hat : "");
    APPEND(",\"hat_raw\":%u", report->hat_adc);
    APPEND(",\"hat_push\":%u", report->hat_button);

    APPEND("}");

    #undef APPEND
}
// === Expose Struct If Needed ===
InputReport* get_input_report(void) {
    return &input_report;
}
