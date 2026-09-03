#ifndef MOISTURE_ADC_H
#define MOISTURE_ADC_H

#include "esp_adc/adc_oneshot.h"
#include "sys_queues.h"

// Map ADC1 channels for 4 plants: GPIO 4, 5, 6, 7
static const adc_channel_t MOISTURE_CHANNELS[NUM_PLANTS] = {
    ADC_CHANNEL_3, // GPIO 4
    ADC_CHANNEL_4, // GPIO 5
    ADC_CHANNEL_5, // GPIO 6
    ADC_CHANNEL_6  // GPIO 7
};

esp_err_t moisture_adc_init(adc_oneshot_unit_handle_t *out_handle);
esp_err_t moisture_adc_read_channel(adc_oneshot_unit_handle_t handle, int plant_idx, int *out_raw_val);

#endif // MOISTURE_ADC_H