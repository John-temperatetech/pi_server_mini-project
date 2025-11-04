# Pi5_Localserver_MiniProject
A Simple tutorial on how to setup time-series dashboard of sensor data hosted on your LAN using RasberryPi-5 as mqtt host and local server
# Raspberry Pi 5 Local MQTT + InfluxDB + Grafana Data Logger & Visualization Setup

This guide walks you through setting up a complete data logging and visualization stack on a Raspberry Pi 5 using Mosquitto (MQTT broker), InfluxDB (time-series database), Telegraf (collector/forwarder), and Grafana (dashboard). It's designed for IoT/sensor projects, and supports multiple MQTT devices (ESP32, Arduino, etc.).

---

## 1. Prepare Raspberry Pi

**Hardware Required:**
- Raspberry Pi 5 (4GB)
- microSD card (≥32GB recommended)
- Official Raspberry Pi PSU (27W)

**Steps:**
- Install Raspberry Pi OS using [Raspberry Pi Imager](https://www.raspberrypi.com/software/).
- Configure:
  - Hostname
  - Username & password
  - Wi-Fi
  - Timezone
  - **Enable SSH** for remote access

---

## 2. Connect to Raspberry Pi via SSH (Local Network)

On another computer (your pc and not pi) connected to the same network:
- Open PowerShell or Terminal:
  ```
  ssh <username>@<hostname>.local
  ```
- Enter your password. You now have remote terminal access.

---

## 3. Update System

```bash
sudo apt update && sudo apt upgrade -y
```
- Installs latest packages for your Pi.

---

## 4. Install Mosquitto (MQTT Broker)

```bash
sudo apt install mosquitto mosquitto-clients -y
sudo systemctl enable mosquitto
sudo systemctl start mosquitto
```

**Test Broker:**
- Terminal 1:
  ```
  mosquitto_sub -t test/topic &
  ```
- Terminal 2:
  ```
  mosquitto_pub -t test/topic -m "hello"
  ```
- Terminal 1 should print `hello`.

---

## 5. Configure Mosquitto

```bash
sudo nano /etc/mosquitto/mosquitto.conf
```
Add at the end:
```
listener 1883
allow_anonymous false
password_file /etc/mosquitto/passwd
```
- **Save:** `Ctrl+O`, `Enter`, `Ctrl+X`
- **Restart:** 
  ```bash
  sudo systemctl restart mosquitto
  ```

---

## 6. Enable Mosquitto Authentication

```bash
sudo mosquitto_passwd -c /etc/mosquitto/passwd <MQTT_USER>
```
- Enter and re-enter password for `<MQTT_USER>`

---

## 7. Restart Mosquitto

```bash
sudo systemctl restart mosquitto
```

---

## 8. Install InfluxDB (Local Time-Series Database)

```bash
wget -qO- https://repos.influxdata.com/influxdata-archive_compat.key | gpg --dearmor | sudo tee /usr/share/keyrings/influxdb.gpg > /dev/null
echo "deb [signed-by=/usr/share/keyrings/influxdb.gpg] https://repos.influxdata.com/debian bookworm stable" | sudo tee /etc/apt/sources.list.d/influxdb.list
sudo apt update
sudo apt install influxdb2 -y
sudo systemctl enable influxdb
sudo systemctl start influxdb
```

- Access InfluxDB at: `http://<pi-ip>:8086`
- Create:
  - **Organization:** `<Your-org>`
  - **Bucket:** `<Your-bucket>`
  - **API Token:** Copy and save your token (give all access permissions).

---

## 9. Install & Configure Telegraf

```bash
sudo apt install telegraf -y
sudo nano /etc/telegraf/telegraf.conf
```
- Go to end of file (`Ctrl+/`, Enter, Return).
- **Add inputs for each device:**

```toml
# Device 1
[[inputs.mqtt_consumer]]
  servers = ["tcp://localhost:1883"]
  topics = ["MQTT_BASE_TOPIC_DEVICE1/#"]
  username = "<MQTT_USER>"
  password = "<MQTT_PASS>"
  data_format = "value"
  data_type = "float"
  name_override = "<measurement_query1>" # Unique name per device/project

# Device 2
[[inputs.mqtt_consumer]]
  servers = ["tcp://localhost:1883"]
  topics = ["MQTT_BASE_TOPIC_DEVICE2/#"]
  username = "<MQTT_USER>"
  password = "<MQTT_PASS>"
  data_format = "value"
  data_type = "float"
  name_override = "<measurement_query2>"
```

- **Add Output Section:**

```toml
[[outputs.influxdb_v2]]
  urls = ["http://localhost:8086"]
  token = "<Your-api>"
  organization = "<Your-org>"
  bucket = "<Your-bucket>"
```

- **Save and exit.**
- **Restart Telegraf:**  
  ```bash
  sudo systemctl restart telegraf
  ```

---

## 10. Install Grafana (Dashboard Visualization)

```bash
sudo apt install -y apt-transport-https software-properties-common
wget -q -O - https://packages.grafana.com/gpg.key | sudo apt-key add -
echo "deb https://packages.grafana.com/oss/deb stable main" | sudo tee /etc/apt/sources.list.d/grafana.list
sudo apt update
sudo apt install grafana -y
sudo systemctl enable grafana-server
sudo systemctl start grafana-server
```

- Access Grafana at: `http://<pi-ip>:3000`
- Default credentials: `admin` / `admin`
- **Add InfluxDB Data Source:**
  - Query language: Flux
  - URL: `http://localhost:8086`
  - Org: `<Your-org>`
  - Bucket: `<Your-bucket>`
  - Token: `<Your-api>`
  - **Save & Test** – errors indicate incorrect details.

---

## 11. Data Flow & Visualization

**Flow:**  
Device publishes → Mosquitto receives → Telegraf ingests → InfluxDB stores → Grafana visualizes

**Steps:**
1. Open InfluxDB (`http://<pi-ip>:8086`).
   - Data Explorer:
     - Select `<Your-bucket>`
     - Filter `_measurement` = `<measurement_query1/2>`
     - Filter `_field` = `value`
     - Filter by `<hostname>` and `topic`
     - Select relevant MQTT topics (e.g., `MQTT_BASE_TOPIC_DEVICE1/subtopic/...`)
     - Submit. Use Script Editor to copy Flux query.
2. Grafana:
   - Home → Dashboards → New Dashboard → Add Visualization
   - Select InfluxDB data source
   - Paste Flux query in queries tab
   - Refresh, customize dashboard as needed

---

## 12. Restart Mosquitto & Telegraf After Changes

Whenever you modify Mosquitto or Telegraf configs:
```bash
sudo systemctl restart mosquitto
sudo systemctl restart telegraf
```

---

## Sample Arduino Sketches

- **UNOR4_1** and **ESP32_1_CrateMonitor**:  
  Example sensor data publishers (see `UNOR4_1.ino`, `ESP32_1_CrateMonitor.ino` in repo).

---

## Troubleshooting Tips

- **Mosquitto**: Use `sudo journalctl -u mosquitto` for logs.
- **InfluxDB**: Confirm bucket/org/token permissions.
- **Telegraf**: Check `/var/log/telegraf/telegraf.log` for ingestion errors.
- **Grafana**: Check data source test result for connection errors.

---

## References

- [Mosquitto MQTT](https://mosquitto.org/)
- [InfluxDB Docs](https://docs.influxdata.com/)
- [Telegraf Docs](https://github.com/influxdata/telegraf)
- [Grafana Docs](https://grafana.com/docs/)

---

## Summary

This setup provides a robust, local data pipeline for IoT/sensor projects:
- **MQTT broker:** Secure, multi-device communication
- **InfluxDB:** Efficient time-series storage
- **Telegraf:** Flexible data ingestion
- **Grafana:** Powerful dashboard visualization

You can expand by adding more devices (duplicate/add more `inputs.mqtt_consumer` blocks), experiment with dashboards, or integrate notifications/alerts.

---
