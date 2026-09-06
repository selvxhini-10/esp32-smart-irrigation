#include "mqtt_app.h"
#include <stdio.h>
#include "mqtt_client.h"
#include "esp_log.h"
#include "sdkconfig.h"

static const char *TAG = "MQTT_APP";
static esp_mqtt_client_handle_t client = NULL;
static bool is_connected = false;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    // Remove: esp_mqtt_event_handle_t event = event_data;
    (void)event_data; // Prevents unused variable warning

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT Connected to Broker!");
            is_connected = true;
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT Disconnected.");
            is_connected = false;
            break;
        default:
            break;
    }
}

esp_err_t mqtt_app_start(void)
{
    // Uses the macro created from Kconfig.projbuild
    esp_mqtt_client_config_t mqtt_cfg = {
    .broker.address.uri = "mqtt://10.0.0.84:1883",
};

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    return esp_mqtt_client_start(client);
}

void mqtt_publish_telemetry(const sensor_data_t *data)
{
    if (!is_connected || client == NULL) return;

    char json_payload[256];
    snprintf(json_payload, sizeof(json_payload),
             "{\"p1\":%d,\"p2\":%d,\"p3\":%d,\"p4\":%d,\"temp\":%.1f,\"hum\":%.1f}",
             data->moisture_raw[0], data->moisture_raw[1],
             data->moisture_raw[2], data->moisture_raw[3],
             data->temperature, data->humidity);

    int msg_id = esp_mqtt_client_publish(client, TOPIC_TELEMETRY, json_payload, 0, 1, 0);
    ESP_LOGI(TAG, "Published JSON Payload (Msg ID %d): %s", msg_id, json_payload);
}