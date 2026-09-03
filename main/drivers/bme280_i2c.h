#ifndef BME280_I2C_H
#define BME280_I2C_H

#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define I2C_MASTER_SDA_IO      GPIO_NUM_8    // Adjust to your S3 SDA pin
#define I2C_MASTER_SCL_IO      GPIO_NUM_9    // Adjust to your S3 SCL pin
    #define BME280_I2C_ADDR        0x77          // Primary address (or 0x77)

typedef struct {
    float temperature; // Degrees C
    float pressure;    // hPa
    float humidity;    // % RH
} bme280_data_t;

esp_err_t bme280_i2c_init(SemaphoreHandle_t *out_mutex, i2c_master_dev_handle_t *out_dev_handle);
esp_err_t bme280_read_data(SemaphoreHandle_t mutex, i2c_master_dev_handle_t dev_handle, bme280_data_t *out_data);

#endif // BME280_I2C_H