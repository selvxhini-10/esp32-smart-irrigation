#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "sys_queues.h"
#include "network/mqtt_app.h"
#include "network/wifi_app.h"
#include "pump_driver.h"
#include "moisture_adc.h"
#include "bme280_i2c.h"

static const char *TAG = "APP_MAIN";

QueueHandle_t xSensorQueue = NULL;
QueueHandle_t xMqttQueue = NULL; 

// Tell GCC to ignore unused warnings for hardware handles during simulation
static SemaphoreHandle_t i2c_bus_mutex __attribute__((unused)) = NULL;
static adc_oneshot_unit_handle_t adc_handle __attribute__((unused));

static const gpio_num_t PUMP_PINS[NUM_PLANTS] __attribute__((unused)) = {
    GPIO_NUM_1,
    GPIO_NUM_2,
    GPIO_NUM_3,
    GPIO_NUM_10
};

#define MOISTURE_DRY_THRESHOLD 2000 
#define MOISTURE_WET_THRESHOLD 1500 
#define MAX_WATERING_CYCLES 5       

typedef struct
{
    gpio_num_t gpio_pin;
    uint32_t prime_time_ms; 
    uint32_t pulse_time_ms; 
    uint32_t soak_dwell_ms; 
} plant_hydraulic_config_t;

static const plant_hydraulic_config_t PLANT_CONFIGS[NUM_PLANTS] = {
    {.gpio_pin = GPIO_NUM_1, .prime_time_ms = 10000, .pulse_time_ms = 3000, .soak_dwell_ms = 5000},
    {.gpio_pin = GPIO_NUM_2, .prime_time_ms = 10000, .pulse_time_ms = 3000, .soak_dwell_ms = 5000},
    {.gpio_pin = GPIO_NUM_3, .prime_time_ms = 13000, .pulse_time_ms = 3000, .soak_dwell_ms = 6000},
    {.gpio_pin = GPIO_NUM_10, .prime_time_ms = 18000, .pulse_time_ms = 4000, .soak_dwell_ms = 7000},
};

// --- TASK 1: Multi-Channel Sensor Sampling Task ---
void sensor_task(void *pvParameters)
{
    sensor_data_t sensor_payload = {0};
    static uint32_t sim_adc[NUM_PLANTS] = {2100, 1800, 2200, 1400};

    while (1)
    {
        for (int i = 0; i < NUM_PLANTS; i++)
        {
            sim_adc[i] += 50; 
            if (sim_adc[i] > 2800) sim_adc[i] = 1300; 

            sensor_payload.moisture_raw[i] = sim_adc[i];
        }

        xQueueSend(xSensorQueue, &sensor_payload, pdMS_TO_TICKS(100));
        xQueueSend(xMqttQueue, &sensor_payload, pdMS_TO_TICKS(100));

        vTaskDelay(pdMS_TO_TICKS(8000)); 
    }
}

// --- TASK 2: 4-Channel Telemetry & Irrigation Simulator ---
void irrigation_task(void *pvParameters)
{
    sensor_data_t rx_data;

    while (1) {
        if (xQueueReceive(xSensorQueue, &rx_data, portMAX_DELAY) == pdTRUE)
        {
            for (int i = 0; i < NUM_PLANTS; i++)
            {
                const plant_hydraulic_config_t *cfg = &PLANT_CONFIGS[i];

                if (rx_data.moisture_raw[i] > MOISTURE_DRY_THRESHOLD)
                {
                    ESP_LOGW(TAG, "[SIMULATION] Plant %d DRY (%d). Simulating Line Prime (%lu ms)...",
                             i + 1, rx_data.moisture_raw[i], cfg->prime_time_ms);

                    vTaskDelay(pdMS_TO_TICKS(1000));

                    int cycle_count = 0;
                    uint32_t sim_moisture = rx_data.moisture_raw[i];

                    while (sim_moisture > MOISTURE_WET_THRESHOLD && cycle_count < MAX_WATERING_CYCLES)
                    {
                        ESP_LOGI(TAG, "[SIMULATION] Plant %d: Pulsing Pump...", i + 1);
                        vTaskDelay(pdMS_TO_TICKS(500)); 
                        cycle_count++;

                        if (sim_moisture > 250) {
                            sim_moisture -= 250;
                        } else {
                            sim_moisture = MOISTURE_WET_THRESHOLD;
                        }

                        ESP_LOGI(TAG, "[SIMULATION] Plant %d Soaking... New Value: %lu", i + 1, sim_moisture);
                        vTaskDelay(pdMS_TO_TICKS(1000)); 
                    }

                    ESP_LOGI(TAG, "[SIMULATION] Plant %d: Target Moisture Achieved (%lu) in %d cycles.",
                             i + 1, sim_moisture, cycle_count);
                }
                else
                {
                    ESP_LOGI(TAG, "Plant %d MOIST (%d). No watering needed.",
                             i + 1, rx_data.moisture_raw[i]);
                }
            }
        }
    }
}

// --- TASK 3: MQTT Network Publisher Task ---
void mqtt_publisher_task(void *pvParameters)
{
    sensor_data_t net_data;

    while (1)
    {
        if (xQueueReceive(xMqttQueue, &net_data, portMAX_DELAY) == pdTRUE)
        {
            // Logging telemetry output for Python MQTT forwarding relay
            ESP_LOGI("MQTT_PUB", "Publishing telemetry to broker...");
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Initializing NVS Flash...");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "Initializing 4-Channel System Capacity...");

    xSensorQueue = xQueueCreate(10, sizeof(sensor_data_t));
    xMqttQueue   = xQueueCreate(10, sizeof(sensor_data_t));

    ESP_LOGI(TAG, "Spawning FreeRTOS Concurrent Worker Tasks...");

    xTaskCreatePinnedToCore(sensor_task, "sensor_task", 4096, NULL, 2, NULL, 0);
    xTaskCreatePinnedToCore(irrigation_task, "irrigation_task", 4096, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(mqtt_publisher_task, "mqtt_publisher_task", 4096, NULL, 2, NULL, 0);
}