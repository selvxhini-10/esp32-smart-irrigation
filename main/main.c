#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c_master.h"

// Custom Component Header
#include "bme280.h"

static const char *TAG = "MAIN";

void app_main(void) {
    // 1. Initialize master I2C bus
    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = GPIO_NUM_22,
        .sda_io_num = GPIO_NUM_21,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus_handle));

    // 2. Attach BME280 using the component API
    bme280_handle_t bme_handle;
    ESP_ERROR_CHECK(bme280_init(bus_handle, BME280_I2C_ADDR_DEFAULT, &bme_handle));

    // 3. Application Loop
    bme280_data_t env_data;
    while (1) {
        if (bme280_read_data(bme_handle, &env_data) == ESP_OK) {
            ESP_LOGI(TAG, "Temp: %.2f C | Humidity: %.2f %% | Pressure: %.2f hPa", 
                     env_data.temperature, env_data.humidity, env_data.pressure);
        }
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}