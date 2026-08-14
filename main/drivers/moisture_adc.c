#include "moisture_adc.h"
#include "esp_log.h"

static const char *TAG = "MOISTURE_ADC";

esp_err_t moisture_adc_init(adc_oneshot_unit_handle_t *out_handle)
{
    adc_oneshot_unit_init_config_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    esp_err_t err = adc_oneshot_new_unit(&init_config, out_handle);
    if (err != ESP_OK) return err;

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    err = adc_oneshot_config_channel(*out_handle, DEFAULT_MOISTURE_ADC_CHANNEL, &config);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Moisture ADC initialized on ADC1 Channel %d", DEFAULT_MOISTURE_ADC_CHANNEL);
    }
    return err;
}

esp_err_t moisture_adc_read(adc_oneshot_unit_handle_t handle, int *out_raw_val)
{
    return adc_oneshot_read(handle, DEFAULT_MOISTURE_ADC_CHANNEL, out_raw_val);
}