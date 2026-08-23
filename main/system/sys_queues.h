#ifndef SYS_QUEUES_H
#define SYS_QUEUES_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef struct {
    int moisture_raw[3];   // Readings for Plant 1, 2, 3
    float temperature;
    float humidity;
} sensor_data_t;

extern QueueHandle_t xSensorQueue;

#endif // SYS_QUEUES_H