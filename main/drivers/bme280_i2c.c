#include "bme280_i2c.h"
#include "esp_log.h"

static const char *TAG = "BME280_I2C";

esp_err_t bme280_i2c_init(SemaphoreHandle_t *out_mutex)
{
    // Create Mutex for shared I2C bus thread safety
    *out_mutex = xSemaphoreCreateMutex();
    if (*out_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create I2C Mutex");
        return ESP_ERR_NO_MEM;
    }

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) return err;

    err = i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "I2C initialized (SDA: %d, SCL: %d)", I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);
    }
    return err;
}

esp_err_t bme280_read_data(SemaphoreHandle_t mutex, bme280_data_t *out_data)
{
    // Acquire Mutex before starting I2C transaction
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        
        // --- Simulated BME280 Raw I2C Read Register Call ---
        // In full hardware deployment, insert BME280 compensation formulas here
        out_data->temperature = 23.5f;
        out_data->humidity = 48.2f;
        out_data->pressure = 1013.25f;

        // Release Mutex after I2C transaction completes
        xSemaphoreGive(mutex);
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "I2C Mutex timeout! Could not read BME280.");
        return ESP_ERR_TIMEOUT;
    }
}