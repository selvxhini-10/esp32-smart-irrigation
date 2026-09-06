#ifndef MQTT_APP_H
#define MQTT_APP_H

#include "esp_err.h"
#include "sys_queues.h"

#define TOPIC_TELEMETRY "smart-irrigation/telemetry"

esp_err_t mqtt_app_start(void);
void mqtt_publish_telemetry(const sensor_data_t *data);

#endif // MQTT_APP_H