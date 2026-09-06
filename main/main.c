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
QueueHandle_t xMqttQueue = NULL; // Separate queue for MQTT publisher
static SemaphoreHandle_t i2c_bus_mutex = NULL;
static i2c_master_dev_handle_t bme280_dev_handle = NULL;
static adc_oneshot_unit_handle_t adc_handle;

// MOSFET Gate Pins for 4 Pumps: GPIO 1, 2, 3, 10
static const gpio_num_t PUMP_PINS[NUM_PLANTS] = {
    GPIO_NUM_1,
    GPIO_NUM_2,
    GPIO_NUM_3,
    GPIO_NUM_10};

#define MOISTURE_DRY_THRESHOLD 2000 // Trigger watering if raw ADC > 2000
#define MOISTURE_WET_THRESHOLD 1500 // Stop watering once raw ADC drops <= 1500
#define MAX_WATERING_CYCLES 5       // Max delivery cycles after priming

typedef struct
{
    gpio_num_t gpio_pin;
    uint32_t prime_time_ms; // Time needed for water to reach the pot
    uint32_t pulse_time_ms; // Time for active watering volume per burst
    uint32_t soak_dwell_ms; // Time for water to filter down to the sensor
} plant_hydraulic_config_t;

// Custom calibration per plant location
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
    bme280_data_t env_data = {0};

    while (1)
    {
        // 1. Sample 4 Moisture Channels
        for (int i = 0; i < NUM_PLANTS; i++)
        {
            moisture_adc_read_channel(adc_handle, i, &sensor_payload.moisture_raw[i]);
        }

        // 2. Sample BME280 (Silently fall back to 0 if unused)
        if (bme280_read_data(i2c_bus_mutex, bme280_dev_handle, &env_data) == ESP_OK)
        {
            sensor_payload.temperature = env_data.temperature;
            sensor_payload.humidity = env_data.humidity;
            sensor_payload.pressure = env_data.pressure;
        }

        // 3. Dispatch telemetry payload to both control & MQTT queues
        xQueueSend(xSensorQueue, &sensor_payload, pdMS_TO_TICKS(100));
        xQueueSend(xMqttQueue, &sensor_payload, pdMS_TO_TICKS(100));

        vTaskDelay(pdMS_TO_TICKS(8000)); // Sample every 8s
    }
}

// --- TASK 2: Independent 4-Channel Irrigation Control Task ---
void irrigation_task(void *pvParameters)
{
    sensor_data_t rx_data;

    while (1)
    {
        // Block until new sensor telemetry arrives (sampled every 8s)
        if (xQueueReceive(xSensorQueue, &rx_data, portMAX_DELAY) == pdTRUE)
        {
            for (int i = 0; i < NUM_PLANTS; i++)
            {
                if (rx_data.moisture_raw[i] > MOISTURE_DRY_THRESHOLD)
                {
                    const plant_hydraulic_config_t *cfg = &PLANT_CONFIGS[i];

                    ESP_LOGW(TAG, "Plant %d DRY (%d). Priming line for %lu ms...",
                             i + 1, rx_data.moisture_raw[i], cfg->prime_time_ms);

                    // Continuous priming cycle
                    pump_set_state(cfg->gpio_pin, true);
                    vTaskDelay(pdMS_TO_TICKS(cfg->prime_time_ms));

                    int cycle_count = 0;
                    int current_moisture = rx_data.moisture_raw[i];

                    while (current_moisture > MOISTURE_WET_THRESHOLD && cycle_count < MAX_WATERING_CYCLES)
                    {
                        ESP_LOGI(TAG, "Plant %d: Pulse (%lu ms)...", i + 1, cfg->pulse_time_ms);
                        pump_set_state(cfg->gpio_pin, true);
                        vTaskDelay(pdMS_TO_TICKS(cfg->pulse_time_ms));

                        pump_set_state(cfg->gpio_pin, false);
                        cycle_count++;

                        ESP_LOGI(TAG, "Plant %d: Soaking for %lu ms...", i + 1, cfg->soak_dwell_ms);
                        vTaskDelay(pdMS_TO_TICKS(cfg->soak_dwell_ms));

                        // Check for fresh telemetry payload safely without deadlocking or drain-spinning
                        sensor_data_t feedback_data;
                        if (xQueueReceive(xSensorQueue, &feedback_data, pdMS_TO_TICKS(100)) == pdTRUE)
                        {
                            current_moisture = feedback_data.moisture_raw[i];
                        }

                        ESP_LOGI(TAG, "Plant %d Post-Soak: %d (Target <= %d)",
                                 i + 1, current_moisture, MOISTURE_WET_THRESHOLD);
                    }

                    pump_set_state(cfg->gpio_pin, false);

                    if (cycle_count >= MAX_WATERING_CYCLES)
                    {
                        ESP_LOGE(TAG, "Plant %d: Max cycles reached! Check sensor.", i + 1);
                    }
                    else
                    {
                        ESP_LOGI(TAG, "Plant %d: Target moisture achieved in %d cycles.", i + 1, cycle_count);
                    }
                }
                else
                {
                    ESP_LOGD(TAG, "Plant %d MOIST (%d <= %d).",
                             i + 1, rx_data.moisture_raw[i], MOISTURE_DRY_THRESHOLD);
                }
            }
        }
        // Yield execution to prevent CPU starvation and log flooding
        vTaskDelay(pdMS_TO_TICKS(100));
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
            mqtt_publish_telemetry(&net_data);
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

    // Create dual queues for concurrent consumption
    xSensorQueue = xQueueCreate(10, sizeof(sensor_data_t));
    xMqttQueue   = xQueueCreate(10, sizeof(sensor_data_t));

    // Initialize Pump GPIOs
    for (int i = 0; i < NUM_PLANTS; i++)
    {
        ESP_ERROR_CHECK(pump_init(PUMP_PINS[i]));
    }

    // Initialize Moisture ADC & I2C Drivers
    ESP_ERROR_CHECK(moisture_adc_init(&adc_handle));
    ESP_ERROR_CHECK(bme280_i2c_init(&i2c_bus_mutex, &bme280_dev_handle));

    // Initialize Network Stack (Wi-Fi + MQTT)
    ESP_LOGI(TAG, "Starting Network Interfaces...");
    if (wifi_init_sta() != ESP_OK) {
        ESP_LOGE(TAG, "Wi-Fi failed to connect! Halting network task initialization.");
        // Decide whether to abort or run offline irrigation mode
        return; 
    }
    mqtt_app_start();

    ESP_LOGI(TAG, "Spawning FreeRTOS Concurrent Worker Tasks...");

    xTaskCreatePinnedToCore(sensor_task, "sensor_task", 4096, NULL, 2, NULL, 0);
    xTaskCreatePinnedToCore(irrigation_task, "irrigation_task", 4096, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(mqtt_publisher_task, "mqtt_publisher_task", 4096, NULL, 2, NULL, 0);
}