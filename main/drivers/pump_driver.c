#include "pump_driver.h"
#include "esp_log.h"

static const char *TAG = "PUMP_DRIVER";
#define DRY_RUN_MODE 1  // Set to 1 to block physical pump actuation

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

esp_err_t pump_set_state(gpio_num_t gpio_num, bool state)
{
    if (DRY_RUN_MODE) {
        // Log state changes without touching hardware GPIOs
        ESP_LOGI("PUMP_DRIVER", "[DRY RUN] Pump on GPIO %d target state -> %s", 
                 gpio_num, state ? "ON" : "OFF");
        return ESP_OK;
    }

    // Normal physical hardware toggle
    return gpio_set_level(gpio_num, state ? 1 : 0);
}