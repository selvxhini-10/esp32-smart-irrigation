#pragma once
#include "esp_adc/adc_oneshot.h"

void soil_sensor_init(void);
int soil_sensor_read(int sensor_index);