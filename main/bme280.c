#include <stdlib.h>
#include "bme280.h"
#include "esp_log.h"

static const char *TAG = "BME280";

struct bme280_dev_t {
    i2c_master_dev_handle_t i2c_dev;
    // Calibration parameters can be stored here for BME280 compensation formulas
};

esp_err_t bme280_init(i2c_master_bus_handle_t bus_handle, uint8_t dev_addr, bme280_handle_t *ret_handle) {
    if (bus_handle == NULL || ret_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    struct bme280_dev_t *dev = calloc(1, sizeof(struct bme280_dev_t));
    if (!dev) {
        return ESP_ERR_NO_MEM;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = dev_addr,
        .scl_speed_hz = 100000,
    };

    esp_err_t ret = i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev->i2c_dev);
    if (ret != ESP_OK) {
        free(dev);
        return ret;
    }

    ESP_LOGI(TAG, "BME280 device added to I2C bus at address 0x%02X", dev_addr);

    *ret_handle = dev;
    return ESP_OK;
}

esp_err_t bme280_read_data(bme280_handle_t dev_handle, bme280_data_t *data) {
    if (dev_handle == NULL || data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // Perform I2C register reads & compensation math here
    data->temperature = 24.5f; 
    data->humidity = 48.0f;
    data->pressure = 1013.25f;

    return ESP_OK;
}

esp_err_t bme280_del(bme280_handle_t dev_handle) {
    if (dev_handle == NULL) return ESP_ERR_INVALID_ARG;
    
    i2c_master_bus_rm_device(dev_handle->i2c_dev);
    free(dev_handle);
    return ESP_OK;
}