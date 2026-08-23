#include "moisture_adc.h"
#include "esp_log.h"

static const char *TAG = "MOISTURE_ADC";

// Map array indices [0, 1, 2] to ADC1 channels for GPIO 4, 5, 6
const adc_channel_t MOISTURE_ADC_CHANNELS[3] = {
    ADC_CHANNEL_3, // GPIO 4 (Plant 1)
    ADC_CHANNEL_4, // GPIO 5 (Plant 2)
    ADC_CHANNEL_5  // GPIO 6 (Plant 3)
};

esp_err_t moisture_adc_init(adc_oneshot_unit_handle_t *out_handle)
{
    // ESP-IDF v6.x type name: adc_oneshot_unit_init_cfg_t
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    esp_err_t err = adc_oneshot_new_unit(&init_config, out_handle);
    if (err != ESP_OK) return err;

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };

    // Configure all 3 channels
    for (int i = 0; i < 3; i++) {
        err = adc_oneshot_config_channel(*out_handle, MOISTURE_ADC_CHANNELS[i], &config);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to config ADC channel index %d", i);
            return err;
        }
    }

    ESP_LOGI(TAG, "Moisture ADC initialized for 3 channels (GPIO 4, 5, 6)");
    return ESP_OK;
}

esp_err_t moisture_adc_read_channel(adc_oneshot_unit_handle_t handle, int channel_index, int *out_raw_val)
{
    if (channel_index < 0 || channel_index >= 3) {
        return ESP_ERR_INVALID_ARG;
    }
    return adc_oneshot_read(handle, MOISTURE_ADC_CHANNELS[channel_index], out_raw_val);
}