#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// Custom Drivers
#include "pump_driver.h"
#include "moisture_adc.h"
#include "bme280_i2c.h"

#define MOISTURE_DRY_THRESHOLD 2800

static const char *TAG = "APP_MAIN";

// Shared Handles
static adc_oneshot_unit_handle_t adc_handle;
static SemaphoreHandle_t i2c_bus_mutex = NULL;

void app_main(void)
{
    ESP_LOGI(TAG, "Initializing Modular Hardware Drivers...");

    // 1. Initialize Pump Driver
    ESP_ERROR_CHECK(pump_init(DEFAULT_PUMP_GPIO));

    // 2. Initialize Moisture ADC Driver
    ESP_ERROR_CHECK(moisture_adc_init(&adc_handle));

    // 3. Initialize I2C Bus & BME280 Driver
    ESP_ERROR_CHECK(bme280_i2c_init(&i2c_bus_mutex));

    ESP_LOGI(TAG, "Phase 1 Initialization Complete. Starting Control Loop...");

    bme280_data_t env_data;
    int raw_moisture = 0;

    while (1) {
        // Read Moisture Sensor
        if (moisture_adc_read(adc_handle, &raw_moisture) == ESP_OK) {
            ESP_LOGI(TAG, "Soil Moisture: %d", raw_moisture);
        }

        // Read BME280 Sensor (Protected by I2C Mutex)
        if (bme280_read_data(i2c_bus_mutex, &env_data) == ESP_OK) {
            ESP_LOGI(TAG, "Env Temp: %.1f°C | Hum: %.1f%% | Press: %.1fhPa", 
                     env_data.temperature, env_data.humidity, env_data.pressure);
        }

        // Pump Control Logic
        if (raw_moisture > MOISTURE_DRY_THRESHOLD) {
            ESP_LOGW(TAG, "Soil is DRY! Activating pump...");
            pump_set_state(DEFAULT_PUMP_GPIO, true);
            vTaskDelay(pdMS_TO_TICKS(2000)); // Run for 2s
            pump_set_state(DEFAULT_PUMP_GPIO, false);
        } else {
            ESP_LOGI(TAG, "Soil Moisture Level OK.");
        }

        vTaskDelay(pdMS_TO_TICKS(5000)); // Sample every 5 seconds
    }
}