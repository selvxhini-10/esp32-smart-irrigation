#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

#include "sys_queues.h"
#include "pump_driver.h"
#include "moisture_adc.h"
#include "bme280_i2c.h"

static const char *TAG = "APP_MAIN";

QueueHandle_t xSensorQueue = NULL;
static SemaphoreHandle_t i2c_bus_mutex = NULL;
static adc_oneshot_unit_handle_t adc_handle;

// GPIO assignments
static const gpio_num_t PUMP_PINS[3] = {GPIO_NUM_1, GPIO_NUM_2, GPIO_NUM_3};
#define MOISTURE_DRY_THRESHOLD 2800

// --- TASK 1: Sensor Reader Task ---
void sensor_task(void *pvParameters)
{
    sensor_data_t sensor_payload;
    bme280_data_t env_data;

    while (1) {
        // 1. Read 3 Moisture Sensors
        for (int i = 0; i < 3; i++) {
            // Read ADC channel mapped to each plant
            moisture_adc_read_channel(adc_handle, i, &sensor_payload.moisture_raw[i]);
        }

        // 2. Read BME280 Environment Data (Mutex protected)
        if (bme280_read_data(i2c_bus_mutex, &env_data) == ESP_OK) {
            sensor_payload.temperature = env_data.temperature;
            sensor_payload.humidity = env_data.humidity;
        }

        // 3. Send payload to Queue (Non-blocking or short wait)
        if (xQueueSend(xSensorQueue, &sensor_payload, pdMS_TO_TICKS(100)) != pdTRUE) {
            ESP_LOGW(TAG, "Sensor Queue Full! Overwrite or dropped reading.");
        }

        // Sample every 3 seconds independently of pump status
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

// --- TASK 2: Irrigation Control Task ---
void irrigation_task(void *pvParameters)
{
    sensor_data_t received_data;

    while (1) {
        // Wait indefinitely until new data arrives in the queue
        if (xQueueReceive(xSensorQueue, &received_data, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "--- Telemetry Received ---");
            ESP_LOGI(TAG, "Temp: %.1f°C | Hum: %.1f%%", received_data.temperature, received_data.humidity);

            for (int i = 0; i < 3; i++) {
                ESP_LOGI(TAG, "Plant %d Moisture: %d", i + 1, received_data.moisture_raw[i]);

                if (received_data.moisture_raw[i] > MOISTURE_DRY_THRESHOLD) {
                    ESP_LOGW(TAG, "Plant %d DRY! Activating Pump %d...", i + 1, i + 1);
                    pump_set_state(PUMP_PINS[i], true);
                    vTaskDelay(pdMS_TO_TICKS(2000)); // Run pump for 2s
                    pump_set_state(PUMP_PINS[i], false);
                }
            }
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting Smart Irrigation Architecture Phase 2...");

    // Create FreeRTOS Queue for up to 5 sensor readings
    xSensorQueue = xQueueCreate(5, sizeof(sensor_data_t));

    // Initialize Drivers
    for (int i = 0; i < 3; i++) {
        pump_init(PUMP_PINS[i]);
    }
    moisture_adc_init(&adc_handle);
    bme280_i2c_init(&i2c_bus_mutex);

    // Spawn Independent FreeRTOS Tasks
    xTaskCreatePinnedToCore(sensor_task, "sensor_task", 4096, NULL, 2, NULL, 0);
    xTaskCreatePinnedToCore(irrigation_task, "irrigation_task", 4096, NULL, 3, NULL, 1);
}