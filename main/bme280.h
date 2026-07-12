#pragma once

#include "driver/i2c_master.h"
#include "esp_err.h"

#define BME280_I2C_ADDR_DEFAULT 0x76 // Common address (or 0x77)

typedef struct {
    float temperature; // in Celsius
    float humidity;    // in %
    float pressure;    // in hPa
} bme280_data_t;

// Opaque handle for the BME280 device
typedef struct bme280_dev_t* bme280_handle_t;

/**
 * @brief Initialize and attach BME280 sensor to an existing I2C bus master
 */
esp_err_t bme280_init(i2c_master_bus_handle_t bus_handle, uint8_t dev_addr, bme280_handle_t *ret_handle);

/**
 * @brief Read temperature, humidity, and pressure from the sensor
 */
esp_err_t bme280_read_data(bme280_handle_t dev_handle, bme280_data_t *data);

/**
 * @brief Free resources associated with the BME280 device
 */
esp_err_t bme280_del(bme280_handle_t dev_handle);