#ifndef SYS_QUEUES_H
#define SYS_QUEUES_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define NUM_PLANTS 4

typedef struct {
    int moisture_raw[NUM_PLANTS]; // Channels 1, 2, 3, 4
    float temperature;
    float humidity;
    float pressure;
} sensor_data_t;

extern QueueHandle_t xSensorQueue;

#endif // SYS_QUEUES_H