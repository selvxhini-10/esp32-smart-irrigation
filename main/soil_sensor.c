#include "soil_sensor.h"

static adc_oneshot_unit_handle_t adc1_handle;
static const adc_channel_t CHANNELS[] = {ADC_CHANNEL_4, ADC_CHANNEL_5, ADC_CHANNEL_6, ADC_CHANNEL_7};

void soil_sensor_init(void) {
    adc_oneshot_unit_init_cfg_t init_config = {.unit_id = ADC_UNIT_1};
    adc_oneshot_new_unit(&init_config, &adc1_handle);
    
    adc_oneshot_chan_cfg_t config = {.atten = ADC_ATTEN_DB_12};
    for(int i=0; i<4; i++) adc_oneshot_config_channel(adc1_handle, CHANNELS[i], &config);
}

int soil_sensor_read(int sensor_index) {
    int val = 0;
    adc_oneshot_read(adc1_handle, CHANNELS[sensor_index], &val);
    return val;
}