#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "tusb.h"
#include "ws2812.pio.h"
#include "bsp/board.h"
#include "gpio.h"
#include "commands.h"
#include "leds.h"

#define LED_PIN         3 
#define NUM_LEDS        4
#define UPPER_NUM_LEDS 25
#define UPPER_LED_PIN 27
#define BRIGHTNESS      0.5f

volatile bool led_dirty = false;

static PIO pio = pio0;
static uint sm = 0;
static uint sm1 = 1;
static uint offset;

static InputReport last_input_report = {
    .hat = "invalid"
};



// ---------- Main ----------

void inputPollReport(){
    tud_task();
    update_input_report();
    char json_buffer[512];
    build_input_report_json(&input_report, json_buffer, sizeof(json_buffer));

    if (tud_cdc_connected() && tud_cdc_write_available() >= strlen(json_buffer) + 2) {
        if (input_report_changed(&input_report, &last_input_report)) {
            build_input_report_json(&input_report, json_buffer, sizeof(json_buffer));
            tud_cdc_n_write_str(0, json_buffer);
            tud_cdc_n_write_str(0, "\n");
            tud_cdc_n_write_flush(0);
            last_input_report = input_report; // Shallow copy is fine since it has no dynamic allocations
}
        }
}

void update_LEDs(){
    leds_tick_animation(); // Update any animations
    if(c_get_led_dirty()) {
        c_show_upper_leds(pio, sm1);
        c_show_lower_leds(pio, sm);
        c_clear_led_dirty();
    }
}

void send_color(uint32_t color){
    for (int i = 0; i < UPPER_NUM_LEDS; i++) {
        pio_sm_put_blocking(pio, sm1, color);
    }
}
#define POLL_INTERVAL_MS 25
#define DELAY_MS 50
int main() {
    board_init();
    tusb_init();
    sleep_ms(500); // Allow time for USB to initialize
    gpio_inputs_init(); // initializes all GPIOs
    stdio_init_all();
    offset = pio_add_program(pio, &ws2812_program);
    register_all_commands();
    ws2812_program_init(pio, sm, offset, LED_PIN, 700000, false);
    ws2812_program_init(pio, sm1, offset, UPPER_LED_PIN, 700000, false);

    while (!tud_cdc_connected()) {
        tud_task();
        sleep_ms(10);
    }

    absolute_time_t next_poll = make_timeout_time_ms(POLL_INTERVAL_MS);
    while (true) {
        tud_task();
        char serial_buf[128];
        static int serial_index = 0;
        while (tud_cdc_available()) {
            char c = tud_cdc_read_char();
            if (c == '\n' || c == '\r') {
                serial_buf[serial_index] = '\0';
                tud_cdc_write_str("Received: ");
                tud_cdc_write_str(serial_buf);
                tud_cdc_write_str("\r\n");
                tud_cdc_write_flush();
                dispatch_json_command(serial_buf);
                serial_index = 0;
            } else if (serial_index < sizeof(serial_buf) - 1) {
                serial_buf[serial_index++] = c;
            }
        }
        if (absolute_time_diff_us(get_absolute_time(), next_poll) <= 0) {
            next_poll = make_timeout_time_ms(POLL_INTERVAL_MS);
            inputPollReport(); // refresh input_report from GPIOs
            update_LEDs();
        }
            
    }
}
