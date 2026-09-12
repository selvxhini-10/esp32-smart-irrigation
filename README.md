# 🌱 ESP32-S3 Smart Irrigation System & Telemetry Stack

A 4-channel closed-loop irrigation controller built on FreeRTOS, paired with a
production-style observability stack (MQTT → Telegraf → InfluxDB → Grafana),
all running through Docker and validated via Software-in-the-Loop (SIL)
simulation on QEMU.

This isn't a hobbyist "blink an LED and water a plant" project — it's an
exercise in building the same architectural pattern used in real industrial
and commercial IoT deployments: decoupled firmware tasks, a message broker,
a time-series database, and infrastructure-as-code provisioning, instead of
a closed-source dashboard like Blynk or Adafruit IO.

> 📄 Want the full story — hardware debugging, brownouts, design trade-offs,
> and what I'd do differently? See [`docs/ENGINEERING_JOURNAL.md`](docs/ENGINEERING_JOURNAL.md).
> For a deep technical breakdown of every layer, see [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md).

---

## 📌 Architecture at a Glance

```
┌──────────────────┐      Serial       ┌────────────────────────┐      MQTT      ┌────────────────────┐
│  ESP32-S3 (QEMU)  │ ───────────────▶ │  qemu_mqtt_relay.py    │ ─────────────▶ │ Mosquitto (Broker) │
│  FreeRTOS Tasks   │   (JSON Stream)   │  (Host Serial Relay)   │   Port 1883    └──────────┬──────────┘
└──────────────────┘                   └────────────────────────┘                          │
        ▲                                                                                    ▼
        │                                    ┌────────────────────────┐      Line       ┌──────────────────┐
        │                Flux                │   InfluxDB v2 Time-    │ ◀────────────── │ Telegraf Metrics │
┌──────────────────┐ ◀─────────────────────  │    Series Database    │    Protocol      │    Collector     │
│ Grafana Dashboard│      Port 8086          └────────────────────────┘                  └──────────────────┘
│   (Port 3000)    │
└──────────────────┘
```

Two operating modes are supported:

* **Hardware mode** — real ESP32-S3, sensors, and pumps on the bench.
* **Software-in-the-Loop (SIL) mode** — the exact same firmware image runs
  under QEMU, streaming simulated telemetry over serial, so the full
  network/data pipeline can be developed and demoed without a live circuit.
  (See the journal for *why* this mode exists — it was a deliberate pivot,
  not the original plan.)

---

## 🛠️ Technology Stack

| Layer | Technology |
|---|---|
| Embedded / Firmware | ESP32-S3, ESP-IDF (C), FreeRTOS, QEMU (Xtensa emulator) |
| Hardware Control | N-Channel MOSFET high-side switching, flyback diode protection |
| Host Telemetry Relay | Python 3.11, Paho-MQTT, `subprocess` / `pyserial` |
| Message Broker | Eclipse Mosquitto (MQTT) |
| Data Pipeline & Storage | Telegraf → InfluxDB 2.7 (Flux) |
| Visualization | Grafana 10+ |
| Infrastructure | Docker, Docker Compose, provisioning-as-code |

---

## ⚙️ System Capabilities

* **4 independent irrigation channels**, each with its own calibrated
  hydraulic profile (priming time, pulse duration, soak/dwell time) to
  account for different tubing lengths and elevation from the reservoir.
* **Closed-loop, pulsed control** — pumps are pulsed and re-evaluated against
  live sensor readings rather than run on a blind timer, to avoid
  over-watering caused by soil absorption lag.
* **Dedicated safety watchdog task** (highest FreeRTOS priority) enforcing a
  maximum pump runtime to protect against flooding on sensor failure.
* **MQTT telemetry** for soil moisture (4 channels) and BME280
  temperature/humidity, published as JSON.
* **Fully provisioned observability stack** — Grafana datasources and
  dashboards are checked into the repo as code and load automatically on
  `docker compose up`.

---

## 🔄 FreeRTOS Task Architecture

```
                  ┌─────────────────────────────────┐
                  │        FreeRTOS Scheduler        │
                  └─────────────────────────────────┘
                                    │
          ┌─────────────────────────┴─────────────────────────┐
          │ Core 0                                             │ Core 1
          ▼                                                    ▼
┌────────────────────────┐                          ┌─────────────────────────┐
│  vTask_SensorRead()    │──[Queue: Moisture Data]──▶│  vTask_PumpControl()    │
│  Period: 1000 ms       │                           │  (Event-driven)         │
│  Priority: 2           │                           │  Priority: 3            │
└────────────────────────┘                          └─────────────────────────┘
          │                                                    │
          ▼                                                    ▼
┌────────────────────────┐                          ┌─────────────────────────┐
│  vTask_TelemetryTx()   │                           │  vTask_SafetyWatchdog   │
│  Serializes JSON       │                           │  Max pump runtime guard │
│  Priority: 1           │                           │  Priority: 4 (highest)  │
└────────────────────────┘                          └─────────────────────────┘
```

Full rationale for this decomposition (why sequential `app_main()` logic
breaks down, and why FreeRTOS queues instead of shared globals) is in
[`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md#freertos-task-decomposition).

---

## 🚀 Getting Started

### Prerequisites

* [Docker Desktop](https://www.docker.com/products/docker-desktop/)
* [Python 3.10+](https://www.python.org/)
* ESP-IDF QEMU build environment (`qemu-system-xtensa`)

### 1. Launch the infrastructure stack

```powershell
cd docker
docker compose up -d
docker compose ps   # confirm mosquitto, influxdb, telegraf, grafana are all "Up"
```

### 2. Run the telemetry relay (SIL mode)

From the repository root:

```powershell
pip install paho-mqtt
python qemu_mqtt_relay.py
```

This launches the compiled firmware under QEMU, parses its serial output,
and republishes it as MQTT telemetry — no physical hardware required.

### 3. View the dashboard

Open [http://localhost:3000](http://localhost:3000) (`admin` / see
`docker-compose.yml` for the configured password) and open the **Soil
Moisture & Plant Telemetry** dashboard.

> ⚠️ **Security note:** the checked-in Mosquitto config allows anonymous
> connections and the Grafana/InfluxDB credentials are placeholders for
> local development only. Do not deploy this compose file to a
> publicly-reachable host without changing them — see Limitations below.

---

## 📂 Project Structure

```
smart-irrigation/
├── firmware/                   # ESP-IDF C source, FreeRTOS tasks, drivers
│   ├── drivers/                 # pump, ADC/moisture, BME280 (I2C)
│   ├── network/                 # Wi-Fi station + MQTT client
│   └── system/                  # shared queues/types
├── docker/
│   ├── docker-compose.yml       # Mosquitto, InfluxDB, Telegraf, Grafana
│   ├── mosquitto/                # broker config
│   ├── telegraf/                 # MQTT → InfluxDB parsing rules
│   └── grafana/provisioning/     # datasources + dashboards as code
├── docs/
│   ├── ARCHITECTURE.md          # full technical deep-dive
│   └── ENGINEERING_JOURNAL.md   # build log, debugging, decisions, learnings
├── qemu_mqtt_relay.py           # host serial-to-MQTT relay for SIL testing
└── README.md
```

---

## 🚧 Current Limitations / Known Issues

* **Hardware validation is partial.** The control and telemetry pipeline is
  fully validated end-to-end in SIL mode; bench validation of continuous
  multi-pump operation was paused after repeated brownout resets (root
  cause understood, fix identified, hardware fix not yet installed — see
  the journal).
* **`pump_driver.c` ships with `DRY_RUN_MODE` hard-compiled to `1`** as a
  safety default — physical GPIO actuation is intentionally disabled until
  the power-supply fix is verified on the bench.
* **No TLS/auth on MQTT or the local stack** — acceptable for a local
  dev/demo network, not for anything internet-facing.
* **BME280 driver is currently a stub** returning fixed dummy values (the
  sensor was pulled from the design over I2C addressing/reliability
  issues — see journal); moisture channels are unaffected.
* Secrets (InfluxDB token, Grafana admin password) are currently committed
  in plaintext for local dev convenience and should move to `.env` /
  Docker secrets before any shared or hosted use.

## 🗺️ Roadmap

* Install bulk decoupling capacitor on the pump rail and re-validate
  hardware mode against the SIL baseline.
* Local HMI: a "Cheap Yellow Display" (ESP32 + LVGL) showing live gauges
  and a manual "force water" override at the edge.
* Centralized log correlation (Loki/Promtail) alongside InfluxDB metrics
  for single-pane debugging.
* Move secrets out of source control; add MQTT auth/TLS.