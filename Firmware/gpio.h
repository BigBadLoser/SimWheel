#ifndef GPIO_H
#define GPIO_H

#include <stdbool.h>
#include <stdint.h>

// Pin counts
#define ENCODER_COUNT 10
#define BUTTON_COUNT 8
#define TOGGLE_COUNT 2
#define SHIFT_COUNT 2
#define BUTTON_COUNT 8
#define TOGGLE_COUNT 2
#define SHIFT_COUNT 2
#define ENCODER_COUNT 10
#define HAT_DIR_PIN 47
#define HAT_BUTTON_PIN 10
#define HAT_ADC_CHANNEL 7  // GPIO 47 = ADC7 = hat switch

// Input report structure
typedef struct {
    bool buttons[BUTTON_COUNT];
    bool toggles[TOGGLE_COUNT];
    bool shifters[SHIFT_COUNT];
    int encoders[ENCODER_COUNT];
    bool hat_button; 
    uint16_t hat_adc; 
    const char* hat;
} InputReport;

// Global state
extern InputReport input_report;
extern const uint8_t encoder_A_pins[ENCODER_COUNT];
extern const uint8_t encoder_B_pins[ENCODER_COUNT];
extern int8_t encoder_deltas[ENCODER_COUNT];
extern uint8_t encoder_last_state[ENCODER_COUNT];

extern const uint8_t button_pins[BUTTON_COUNT];
extern const uint8_t toggle_pins[TOGGLE_COUNT];
extern const uint8_t shift_pins[SHIFT_COUNT];

// Functions
void gpio_inputs_init();
void update_input_report();
InputReport* get_input_report();
const char* decode_hat_adc(uint16_t raw);
void update_encoder_values();
void build_input_report_json(const InputReport* report, char* out, size_t max_len);
bool input_report_changed(const InputReport* a, const InputReport* b);
#endif // GPIO_H