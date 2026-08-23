#ifndef MOISTURE_ADC_H
#define MOISTURE_ADC_H

#include "esp_adc/adc_oneshot.h"

// Define ADC Channels corresponding to GPIOs:
// GPIO 4 -> ADC1 Channel 3
// GPIO 5 -> ADC1 Channel 4
// GPIO 6 -> ADC1 Channel 5
extern const adc_channel_t MOISTURE_ADC_CHANNELS[3];

esp_err_t moisture_adc_init(adc_oneshot_unit_handle_t *out_handle);
esp_err_t moisture_adc_read_channel(adc_oneshot_unit_handle_t handle, int channel_index, int *out_raw_val);

#endif // MOISTURE_ADC_H