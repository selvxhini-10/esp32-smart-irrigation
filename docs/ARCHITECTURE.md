# System Architecture & Technical Specifications

## 1. Embedded Firmware Layer (ESP32-S3 / FreeRTOS)
* **Sensors System:** Reads capacitive soil moisture raw ADC values and BME280 telemetry.
* **Task Management:** Dedicated FreeRTOS tasks handle sensor sampling, calibration scaling, and serializing telemetry payloads into formatted JSON strings over `UART0`.
* **QEMU Emulation:** Target compiled using ESP-IDF Xtensa toolchain for QEMU simulation (`qemu-system-xtensa`).

## 2. Telemetry Ingestion Pipeline
1. **Serial-to-MQTT Relay (`qemu_mqtt_relay.py`):** Spawns QEMU as a subprocess, monitors standard output for JSON telemetry packets, and publishes to Mosquitto MQTT topic `plant/telemetry`.
2. **Mosquitto MQTT Broker:** Listens on port `1883` and handles message transport.
3. **Telegraf Metrics Collector:** Subscribes to `plant/telemetry`, parses incoming JSON payloads, and writes formatted line-protocol metrics to InfluxDB v2.
4. **InfluxDB v2:** Bucket `irrigation_metrics` stores raw time-series data indexed by `plant_id` and sensor attributes.
5. **Grafana:** Queries InfluxDB using Flux queries to render continuous time-series graphs, moisture percentage gauges, and pump actuation indicators.