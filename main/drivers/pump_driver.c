#include "pump_driver.h"
#include "esp_log.h"

static const char *TAG = "PUMP_DRIVER";

esp_err_t pump_init(gpio_num_t gpio_num)
{
    gpio_reset_pin(gpio_num);
    esp_err_t err = gpio_set_direction(gpio_num, GPIO_MODE_OUTPUT);
    if (err == ESP_OK) {
        gpio_set_level(gpio_num, 0); // Start OFF
        ESP_LOGI(TAG, "Pump initialized on GPIO %d", gpio_num);
    }
    return err;
}

esp_err_t pump_set_state(gpio_num_t gpio_num, bool turn_on)
{
    esp_err_t err = gpio_set_level(gpio_num, turn_on ? 1 : 0);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Pump State -> %s", turn_on ? "ON" : "OFF");
    }
    return err;
}