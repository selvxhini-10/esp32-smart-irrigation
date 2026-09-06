#include "bme280_i2c.h"
#include "esp_log.h"

static const char *TAG = "BME280_I2C";

esp_err_t bme280_i2c_init(SemaphoreHandle_t *out_mutex, i2c_master_dev_handle_t *out_dev_handle)
{
    // Create mutex so caller logic relying on the handle doesn't crash
    if (out_mutex != NULL && *out_mutex == NULL) {
        *out_mutex = xSemaphoreCreateMutex();
    }
    
    ESP_LOGW(TAG, "BME280 I2C driver temporarily disabled to prevent log spam.");
    return ESP_OK;
}

esp_err_t bme280_read_data(SemaphoreHandle_t mutex, i2c_master_dev_handle_t dev_handle, bme280_data_t *data)
{
    if (data == NULL) return ESP_ERR_INVALID_ARG;

    // Return dummy values to keep queue payloads valid
    data->temperature = 0.0f;
    data->humidity = 0.0f;
    data->pressure = 0.0f;

    return ESP_OK;
}