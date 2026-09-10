import subprocess
import re
import json
import paho.mqtt.client as mqtt

MQTT_BROKER = "localhost"
MQTT_PORT = 1883
MQTT_TOPIC = "home/irrigation/telemetry"

QEMU_CMD = [
    r"C:\Users\selva\Downloads\qemu-xtensa-softmmu-esp_develop_9.2.2_20260417-x86_64-w64-mingw32\qemu\bin\qemu-system-xtensa.exe",
    "-machine", "esp32s3",
    "-drive", "file=build/merged-flash.bin,if=mtd,format=raw",
    "-serial", "mon:stdio",
    "-nographic"
]

def main():
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    try:
        client.connect(MQTT_BROKER, MQTT_PORT, 60)
        print(f"[RELAY] Connected to MQTT Broker at {MQTT_BROKER}:{MQTT_PORT}")
    except Exception as e:
        print(f"[RELAY ERROR] Could not connect to Mosquitto: {e}")
        return

    print("[RELAY] Launching QEMU and listening for serial output...")
    process = subprocess.Popen(
        QEMU_CMD,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1
    )

    plant_status_pattern = re.compile(r"Plant (\d+) (MOIST|DRY) \((\d+)\)")
    soak_pattern = re.compile(r"Plant (\d+) Soaking\.\.\. New Value: (\d+)")

    for line in iter(process.stdout.readline, ''):
        # Filter out QEMU internal warnings from serial logs
        if "qemu_add_wait_object" in line:
            continue
            
        print(line, end='', flush=True)

        match_status = plant_status_pattern.search(line)
        if match_status:
            plant_id = int(match_status.group(1))
            status = match_status.group(2)
            moisture = int(match_status.group(3))

            payload = {
                "plant_id": plant_id,
                "status": status,
                "moisture_raw": moisture
            }
            client.publish(f"{MQTT_TOPIC}/plant_{plant_id}", json.dumps(payload))
            print(f"\n--> [MQTT PUBLISHED] Topic: {MQTT_TOPIC}/plant_{plant_id} | Payload: {payload}", flush=True)
            continue

        match_soak = soak_pattern.search(line)
        if match_soak:
            plant_id = int(match_soak.group(1))
            moisture = int(match_soak.group(2))

            payload = {
                "plant_id": plant_id,
                "status": "WATERING",
                "moisture_raw": moisture
            }
            client.publish(f"{MQTT_TOPIC}/plant_{plant_id}", json.dumps(payload))
            print(f"\n--> [MQTT PUBLISHED] Topic: {MQTT_TOPIC}/plant_{plant_id} | Payload: {payload}", flush=True)

    process.stdout.close()
    process.wait()

if __name__ == "__main__":
    main()