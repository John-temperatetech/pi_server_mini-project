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
sudo mkdir -p /etc/apt/keyrings
curl -fsSL https://packages.grafana.com/gpg.key | sudo gpg --dearmor -o /etc/apt/keyrings/grafana.gpg

echo "deb [signed-by=/etc/apt/keyrings/grafana.gpg] https://packages.grafana.com/oss/deb stable main" | sudo tee /etc/apt/sources.list.d/grafana.list

sudo apt update
sudo apt install grafana -y
sudo systemctl enable grafana-server
sudo systemctl start grafana-server
systemctl status grafana-server
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

# AWS InfluxDB, Telegraf, Grafana, and S3 Backup Setup

This guide documents the steps to set up an EC2-based InfluxDB/Telegraf/Grafana stack with S3 backups, ready for Grafana monitoring and Pi telemetry.

## IAM Setup

### 1. User Groups
- **Name:** Admins
- **Policy:** `AdministratorAccess`

### 2. Users
- **User name:** `<User1_admin>`
- **AWS Console access:** Checked
- **Console Password:** `<console_pass>`
- **Console sign-in URL:** `[https://<root_user>.signin.aws.amazon.com/console]`
- **MFA:** `<My_auth>`

### 3. Roles
- **Trusted entity type:** AWS service
- **Use case:** EC2 → EC2
- **Permissions policies:** `AmazonS3FullAccess`
- **Role name:** `EC2-S3-Backup-Role`
- **Role ARN:** `arn:aws:iam::<AWS_ID>:role/EC2-S3-Backup-Role`
- **Instance profile ARN:** `arn:aws:iam::<AWS_ID>:instance-profile/EC2-S3-Backup-Role`

## EC2 Instance Setup

- **Region:** Asia Pacific (Mumbai) `ap-south-1`
- **Instance name:** `<inhouse_webserver_unique_token>`
- **OS (AMI):** Ubuntu Server 24.04 LTS
- **Instance type:** `t3.micro`
- **Key Pair:** `<inhousekey>`
- **Security Group:**
    - **SSH:** My IP
    - **HTTP, HTTPS**
    - **Custom TCP (add after launch):**
        - Port 1883 (Telegraf) — My IP
        - Port 8086 (InfluxDB) — My IP
        - Port 3000 (Grafana) — My IP

### EC2 Setup Steps

1. During launch, configure storage as required.
2. **Advanced:** Attach `EC2-S3-Backup-Role` as IAM instance profile.
3. Launch instance.
4. Post-launch:
    - Actions > Security > Modify IAM role > select `EC2-S3-Backup-Role`.
5. Allocate and associate an Elastic IP:
    - **IPv4 address:** `<ELASTIC_IP>`
6. Edit security group (launch-wizard-1) to add inbound rules for ports 1883, 8086, 3000 as above.

## S3 Backup Setup

1. **Create Bucket**  
   - Go to S3 > Buckets > Create bucket  
   - **Region:** `ap-south-1`
   - **Type:** General purpose
   - **Name:** `<AWS_ID>influx-backups-inhouse`
   - **Object ownership:** ACLs disabled
   - **Block all public access:** Enabled
   - Create bucket

2. **Bucket Lifecycle Rule**  
   - S3 > Buckets > `<AWS_ID>-influx-backups-inhouse` > Management > Lifecycle rules > Create rule:
     - **Name:** `lifecycle_1`
     - **Action:** Transition current versions to Glacier Flexible Retrieval after 30 days

## InfluxDB, Telegraf & Grafana Installation on EC2

### 1. SSH into EC2

On your machine:
```sh
cd C:\Users\<NAME>\Desktop\aws
ssh -i <inhousekey.pem> ubuntu@<ELASTIC_IP>
```

### 2. Install InfluxDB 2.x

```sh
wget -qO- https://repos.influxdata.com/influxdata-archive_compat.key | gpg --dearmor | sudo tee /usr/share/keyrings/influxdb.gpg > /dev/null
echo "deb [signed-by=/usr/share/keyrings/influxdb.gpg] https://repos.influxdata.com/debian bookworm stable" | sudo tee /etc/apt/sources.list.d/influxdb.list
sudo apt update
sudo apt install influxdb2 -y
sudo systemctl enable influxdb
sudo systemctl start influxdb
```

### 3. Install Telegraf

```sh
sudo apt install telegraf -y
sudo systemctl enable --now telegraf
```

### 4. Optional: Install Grafana (self-hosted)

```sh
sudo apt install -y apt-transport-https software-properties-common
wget -q -O - https://packages.grafana.com/gpg.key | sudo apt-key add -
echo "deb https://packages.grafana.com/oss/deb stable main" | sudo tee /etc/apt/sources.list.d/grafana.list
sudo apt update
sudo apt install grafana -y
sudo systemctl enable --now grafana-server
```

Or, use Amazon Managed Grafana (no server to manage, easier scaling, SSO, less maintenance).

---

## InfluxDB Configuration

- **URL:** `http://<ELASTIC_IP:8086`
- **Username:** `<user_influxdb>`
- **Password:** `<pass_influxdb>`
- **Org:** `<YOUR_CLOUD_ORG>`
- **Bucket:** `<YOUR_CLOUD_BUCKET>`
- **API Token:**  
  `<YOUR_API_TOKEN>`

## Telegraf Output Configuration

On the device (for example, a Raspberry Pi), open Telegraf config:

```sh
sudo nano /etc/telegraf/telegraf.conf
```

Add at the end:

```toml
# Output: remote influxdb (cloud)
[[outputs.influxdb_v2]]
  urls = ["http://<ELASTIC_IP>:8086"]
  token = "<YOUR_CLOUD_API_TOKEN>"
  organization = "<YUR_CLOUD_ORG>"
  bucket = "<YOUR_CLOUD_BUCKET>"
```

---

## Ports and Firewall (Security Group)

- **SSH (22):** My IP
- **HTTP (80), HTTPS (443):** Open as needed
- **Telegraf (1883):** My IP
- **InfluxDB (8086):** My IP
- **Grafana (3000):** My IP

---

# Raspberry Pi Static IP Configuration (NetworkManager)

## Context

This documents the exact steps and findings from configuring a static local IP address on a Raspberry Pi running a NetworkManager/netplan-based setup (dhcpcd not present).

---

## Initial Goal

Assign a fixed local IP address to the Raspberry Pi so it can act as a reliable local server.

Target configuration:

* Interface: `wlan0`
* Static IP: `192.168.1.12/24`
* Gateway: `192.168.1.1`
* DNS: `192.168.1.1`, `8.8.8.8`

---

## Attempt 1: dhcpcd (Not Applicable)

Edited:

```
sudo nano /etc/dhcpcd.conf
```

Configuration added:

```
interface wlan0
static ip_address=192.168.1.12/24
static routers=192.168.1.1
static domain_name_servers=192.168.1.1 8.8.8.8
```

Restart attempt:

```
sudo systemctl restart dhcpcd
```

Result:

```
Failed to restart dhcpcd.service: Unit dhcpcd.service not found.
```

Conclusion:

* `dhcpcd` is not installed or used on this system.
* The system is managed by NetworkManager via netplan.

---

## Network State Verification

Check device status:

```
nmcli device status
```

Output:

```
DEVICE         TYPE      STATE        CONNECTION
wlan0          wifi      connected    netplan-wlan0-temperatetech24g
lo             loopback  connected    lo
p2p-dev-wlan0  wifi-p2p  disconnected --
eth0           ethernet  unavailable  --
```

List connections:

```
sudo nmcli connection show
```

Relevant connection:

```
netplan-wlan0-temperatetech24g  wifi  wlan0
```

---

## Correct Method: NetworkManager (nmcli)

Apply static IPv4 configuration:

```
sudo nmcli connection modify "netplan-wlan0-temperatetech24g" \
  ipv4.method manual \
  ipv4.addresses 192.168.1.12/24 \
  ipv4.gateway 192.168.1.1 \
  ipv4.dns "192.168.1.1 8.8.8.8"
```

Bring connection down and up (required):

```
sudo nmcli connection down "netplan-wlan0-temperatetech24g"
sudo nmcli connection up "netplan-wlan0-temperatetech24g"
```

---

## Verification

Check assigned IP:

```
ip addr show wlan0
```

Expected:

```
inet 192.168.1.12/24
```

Check routing:

```
ip route
```

Expected default route:

```
default via 192.168.1.1
```

---

## Key Notes

* Do **not** use `/etc/dhcpcd.conf` on NetworkManager-based Raspberry Pi OS images.
* Static IP must be set via `nmcli` or netplan YAML.
* Restarting `dhcpcd` will fail if the service is not installed.

---

## Reference Marker

Internal reference keyword stored: **START1**


**End of Guide**
