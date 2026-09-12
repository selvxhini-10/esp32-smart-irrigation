#include "moisture_adc.h"
#include "esp_log.h"

static const char *TAG = "MOISTURE_ADC";

esp_err_t moisture_adc_init(adc_oneshot_unit_handle_t *out_handle)
{
    // Correct struct name in ESP-IDF: adc_oneshot_unit_init_cfg_t
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
        .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };

    esp_err_t err = adc_oneshot_new_unit(&init_config, out_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create ADC oneshot unit instance!");
        return err;
    }

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };

    for (int i = 0; i < NUM_PLANTS; i++) {
        err = adc_oneshot_config_channel(*out_handle, MOISTURE_CHANNELS[i], &config);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to configure ADC Channel for Plant %d", i + 1);
            return err;
        }
    }

    ESP_LOGI(TAG, "Successfully initialized %d ADC moisture channels.", NUM_PLANTS);
    return ESP_OK;
}

esp_err_t moisture_adc_read_channel(adc_oneshot_unit_handle_t handle, int plant_idx, int *out_raw_val)
{
    if (plant_idx < 0 || plant_idx >= NUM_PLANTS) return ESP_ERR_INVALID_ARG;
    return adc_oneshot_read(handle, MOISTURE_CHANNELS[plant_idx], out_raw_val);
}