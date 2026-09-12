# ESP32-S3 Smart Irrigation System & Telemetry Stack

A full-stack embedded systems and IoT data pipeline featuring FreeRTOS-driven sensor telemetry on an ESP32-S3 (simulated via QEMU), real-time MQTT message broker, Telegraf data ingestion, InfluxDB time-series storage, and live Grafana dashboard visualization.

![Architecture Diagram](docs/architecture.png)

## 📌 Architecture Overview

+------------------+      Serial      +-----------------------+      MQTT      +--------------------+
|  ESP32-S3 (QEMU) |--------------->|  qemu_mqtt_relay.py   |-------------->| Mosquitto (Broker) |
|  FreeRTOS Tasks  |  (JSON Stream)  |  (Host Serial Relay)  |  Port 1883     +--------------------+
+------------------+                  +-----------------------+                         |
v
+------------------+       Flux       +-----------------------+      Line      +--------------------+
| Grafana Dashboard|<-----------------|   InfluxDB v2 Time-   |<---------------+  Telegraf Metrics  |
|  (Port 3000)     |   Port 8086      |    Series Database    |   Protocol     |     Collector      |
+------------------+                  +-----------------------+                +--------------------+

## 🛠️ Technology Stack

* **Embedded / Firmware:** ESP32-S3, ESP-IDF (C), FreeRTOS, QEMU (Xtensa emulator)
* **Host Telemetry Relay:** Python 3.11, Paho-MQTT, `subprocess` / `pyserial`
* **Infrastructure Containerization:** Docker, Docker Compose
* **Message Broker:** Eclipse Mosquitto (MQTT)
* **Data Pipeline & Storage:** Telegraf, InfluxDB 2.7 (Flux Query Language)
* **Data Visualization:** Grafana 10+

🛠️ Hardware & Circuit EngineeringThe physical hardware and QEMU simulation models utilize a 5V DC power architecture with high-side switching for inductive/capacitive actuator loads.⚡ Power Delivery & RegulationPrimary Power Supply: 5V DC, 2A Wall Adapter supplying main power rails.MCU Power: 5V rail feeds the ESP32-S3 onboard LDO regulator, stepping down to 3.3V DC for the ESP32-S3 SoC, sensor VCC, and signal reference levels.Pump Load Power: 5V rail directly powers the mini DC water pumps to prevent voltage sag on the 3.3V logic rail during high inrush current motor startups.🔌 Actuator Drive & Switching Circuit (MOSFETs)Each DC pump or solenoid valve is driven via an N-Channel Logic-Level Power MOSFET (e.g., IRLZ44N or AO3400A):Gate Drive Resistor ($R_G$ = 220 Ω): Placed inline between the ESP32-S3 GPIO output pin and the MOSFET Gate. Limits transient ringing and protects GPIO pins against high peak current caused by MOSFET gate capacitance during rapid switching.Gate Pull-Down Resistor ($R_{PD}$ = 10 kΩ): Connected between MOSFET Gate and GND. Holds the MOSFET firmly in an off-state during MCU reset/boot stages before GPIO pins are initialized as outputs.Flyback / Free-Wheeling Diode (1N4007 or SS14 Schottky): Connected in parallel across the pump terminals (cathode to +5V, anode to MOSFET Drain). Dissipates high-voltage inductive spikes caused by motor coil collapsing magnetic fields when turned off, preventing MOSFET Drain-Source breakdown.Source Connection: MOSFET Source connected directly to Common System Ground (GND).🌱 Moisture Sensor CircuitrySensor Type: Capacitive Soil Moisture Sensor (v1.2).Interface: Analog output ($0.1\text{V} - 3.0\text{V}$) connected directly to ESP32-S3 ADC1 channels (e.g., GPIO1 / ADC1_CH0).Filtering: $0.1\,\mu\text{F}$ decoupling capacitor placed across sensor Signal and GND near the MCU pin to attenuate high-frequency switching noise from pumps.💻 Firmware & Low-Level MCU DevelopmentThe firmware is written in C utilizing the ESP-IDF v5.x framework, built around a modular architecture targeting the ESP32-S3 Dual-Core LX7 processor.⚙️ Low-Level Hardware DriversADC Driver (ESP-IDF esp_adc API):Configured for 12-bit resolution ($0 - 4095$ raw digital counts).Uses ADC Attenuation ADC_ATTEN_DB_12 ($0-3.3\text{V}$ range).Implements multi-sampling multisample averaging ($N=64$ samples) in hardware loop to smooth out raw sensor noise before conversion.GPIO Driver (driver/gpio.h):Actuator pins initialized in GPIO_MODE_OUTPUT with active pull-downs disabled.UART Serial Logging (driver/uart.h):UART_NUM_0 configured at 115200 Baud (8N1) for streaming JSON-encoded sensor packets to the host relay script.🔄 FreeRTOS Task ArchitectureThe core runtime uses FreeRTOS tasks distributed across both Xtensa LX7 cores to separate deterministic real-time control from data processing:                  +-----------------------------------+
                  |        FreeRTOS Scheduler         |
                  +-----------------------------------+
                                    |
          +-------------------------+-------------------------+
          | Core 0                                            | Core 1
          v                                                   v
+-----------------------+                           +-----------------------+
|  vTask_SensorRead()   |--[Queue: Moisture Data]-->|   vTask_PumpControl() |
|  Period: 1000 ms      |                           |  (Event-driven / Auto)|
|  Priority: 2          |                           |  Priority: 3          |
+-----------------------+                           +-----------------------+
          |                                                   |
          v                                                   v
+-----------------------+                           +-----------------------+
|  vTask_TelemetryTx()  |                           |  vTask_SafetyWatchdog |
|  Serializes JSON      |                           |  Prevents dry-running |
|  Priority: 1          |                           |  Priority: 4 (Highest)|
+-----------------------+                           +-----------------------+
vTask_SensorRead (Priority 2): Periodically samples soil moisture ADC values and BME280 temperature/humidity sensors every 1000 ms. Applies linear calibration scaling to convert raw ADC millivolts into volumetric water content (VWC %).vTask_TelemetryTx (Priority 1): Reads current system state, formats a JSON string payload, and streams it out over UART0.vTask_PumpControl (Priority 3): Consumes queue metrics. Executes hysteresis thresholding logic (e.g., if Moisture $< 30\%$, trigger MOSFET Gate high for $N$ seconds).vTask_SafetyWatchdog (Priority 4): Monitors maximum pump run duration ($T_{\text{max}} = 10\text{s}$) to prevent flooding or motor burnout in case of sensor disconnects.

## 🚀 Getting Started

### Prerequisites

* [Docker Desktop](https://www.docker.com/products/docker-desktop/) installed and running
* [Python 3.10+](https://www.python.org/) installed on host
* ESP-IDF QEMU build environment (`qemu-system-xtensa`)

### 1. Launch Infrastructure Stack

Navigate to the `docker/` directory and spin up Mosquitto, Telegraf, InfluxDB, and Grafana:

```powershell
cd docker
docker compose up -d

Verify all containers are active:

PowerShell
docker compose ps
2. Run Telemetry Relay Script
From the repository root, install Python dependencies and launch the serial-to-MQTT relay:

PowerShell
pip install paho-mqtt
python qemu_mqtt_relay.py
3. Access Grafana Dashboard
Open http://localhost:3000 in your browser.

Credentials: Username admin / Password adminpassword123.

Open the Soil Moisture & Plant Telemetry dashboard to view live metric streams.

📂 Project Structure
smart-irrigation/
├── main/                       # ESP-IDF C source files & FreeRTOS tasks
├── docker/
│   ├── docker-compose.yml      # Infrastructure container stack
│   ├── telegraf/               # Telegraf configuration & MQTT parser rules
│   └── grafana/                # Provisioned datasources and dashboards
├── qemu_mqtt_relay.py          # Host serial-to-MQTT relay script
└── README.md

---