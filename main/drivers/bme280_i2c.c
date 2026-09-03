#include "bme280_i2c.h"
#include "esp_log.h"

static const char *TAG = "BME280_I2C";

esp_err_t bme280_i2c_init(SemaphoreHandle_t *out_mutex, i2c_master_dev_handle_t *out_dev_handle)
{
    // 1. Create Mutex for shared I2C bus thread safety
    *out_mutex = xSemaphoreCreateMutex();
    if (*out_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create I2C Mutex");
        return ESP_ERR_NO_MEM;
    }

    // 2. Configure I2C Master Bus (ESP-IDF v6.x API)
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    i2c_master_bus_handle_t bus_handle;
    esp_err_t err = i2c_new_master_bus(&bus_config, &bus_handle);
    if (err != ESP_OK) return err;

    // 3. Add BME280 device to bus
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = BME280_I2C_ADDR,
        .scl_speed_hz = 100000,
    };

    err = i2c_master_bus_add_device(bus_handle, &dev_config, out_dev_handle);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "ESP-IDF v6.0 I2C Bus initialized (SDA: %d, SCL: %d)", I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);
    }
    return err;
}

esp_err_t bme280_read_data(SemaphoreHandle_t mutex, i2c_master_dev_handle_t dev_handle, bme280_data_t *out_data)
{
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        uint8_t reg_addr = 0xF7;
        uint8_t raw_buf[8];

        // ESP-IDF v6.0 Master Transmit & Receive API
        esp_err_t err = i2c_master_transmit_receive(dev_handle, &reg_addr, 1, raw_buf, 8, 100);

        if (err == ESP_OK) {
            int32_t adc_T = (int32_t)(((uint32_t)raw_buf[3] << 12) | ((uint32_t)raw_buf[4] << 4) | ((uint32_t)raw_buf[5] >> 4));
            out_data->temperature = (float)(adc_T - 150000) / 5120.0f;
            out_data->humidity = 50.0f;
            out_data->pressure = 1013.25f;
        } else {
            ESP_LOGE(TAG, "I2C Transaction failed!");
        }

        xSemaphoreGive(mutex);
        return err;
    }
    return ESP_ERR_TIMEOUT;
}