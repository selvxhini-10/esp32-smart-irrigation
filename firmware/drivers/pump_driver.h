#ifndef PUMP_DRIVER_H
#define PUMP_DRIVER_H

#include "driver/gpio.h"

// Define GPIO 4 as default Pump Control Pin
#define DEFAULT_PUMP_GPIO GPIO_NUM_4

esp_err_t pump_init(gpio_num_t gpio_num);
esp_err_t pump_set_state(gpio_num_t gpio_num, bool turn_on);

#endif // PUMP_DRIVER_H