#include "pump_control.h"
#include "driver/gpio.h"

static const gpio_num_t PUMP_PINS[] = {GPIO_NUM_16, GPIO_NUM_17, GPIO_NUM_18, GPIO_NUM_19};

void pump_init(void) {
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL<<16) | (1ULL<<17) | (1ULL<<18) | (1ULL<<19),
        .pull_down_en = GPIO_PULLDOWN_ENABLE
    };
    gpio_config(&io_conf);
}

void pump_set_state(int pump_index, bool state) {
    if (pump_index >= 0 && pump_index < 4) {
        gpio_set_level(PUMP_PINS[pump_index], state);
    }
}