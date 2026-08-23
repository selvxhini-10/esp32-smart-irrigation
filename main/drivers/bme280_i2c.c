#include "bme280_i2c.h"
#include "esp_log.h"

static const char *TAG = "BME280_I2C";

esp_err_t bme280_i2c_init(SemaphoreHandle_t *out_mutex)
{
    *out_mutex = xSemaphoreCreateMutex();
    if (*out_mutex == NULL) return ESP_ERR_NO_MEM;

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
    if (err != ESP_OK) return err;

    // --- I2C BUS SCANNER ---
    ESP_LOGI(TAG, "Scanning I2C bus on SDA GPIO %d, SCL GPIO %d...", I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);
    int devices_found = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        esp_err_t scan_err = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(50));
        i2c_cmd_link_delete(cmd);

        if (scan_err == ESP_OK) {
            ESP_LOGI(TAG, " -> Found I2C device at address: 0x%02X", addr);
            devices_found++;
        }
    }

    if (devices_found == 0) {
        ESP_LOGE(TAG, "No I2C devices responded! Check wiring, power (3.3V), and pull-ups.");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t bme280_read_data(SemaphoreHandle_t mutex, bme280_data_t *out_data)
{
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        
        // 1. Send register address 0xF7 (start of Press/Temp/Hum data)
        uint8_t reg_addr = 0xF7;
        uint8_t raw_buf[8];

        esp_err_t err = i2c_master_write_read_device(
            I2C_MASTER_NUM, 
            BME280_I2C_ADDR, 
            &reg_addr, 1, 
            raw_buf, 8, 
            pdMS_TO_TICKS(100)
        );

        if (err == ESP_OK) {
            // Unpack 20-bit raw temperature reading
            int32_t adc_T = (int32_t)(((uint32_t)raw_buf[3] << 12) | ((uint32_t)raw_buf[4] << 4) | ((uint32_t)raw_buf[5] >> 4));
            
            // Standard uncompensated conversion approximation for testing
            out_data->temperature = (float)(adc_T - 150000) / 5120.0f;
            out_data->humidity = 50.0f;    // Baseline test placeholders
            out_data->pressure = 1013.25f;
        } else {
            ESP_LOGE(TAG, "I2C read failed at addr 0x%02X", BME280_I2C_ADDR);
        }

        xSemaphoreGive(mutex);
        return err;
    }
    return ESP_ERR_TIMEOUT;
}