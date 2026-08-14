#ifndef MOISTURE_ADC_H
#define MOISTURE_ADC_H

#include "esp_adc/adc_oneshot.h"

// GPIO 5 corresponds to ADC1 Channel 4 on ESP32-S3
#define DEFAULT_MOISTURE_ADC_CHANNEL ADC_CHANNEL_4

esp_err_t moisture_adc_init(adc_oneshot_unit_handle_t *out_handle);
esp_err_t moisture_adc_read(adc_oneshot_unit_handle_t handle, int *out_raw_val);

#endif // MOISTURE_ADC_H