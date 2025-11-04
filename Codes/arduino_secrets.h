// WiFi
#define ssid "your router ssid"
#define password "your router password"

// MQTT
#define MQTT_SERVER "192.168.1.12"   // Replace with your MQTT broker IP or hostname
#define MQTT_PORT 1883 //mqtt port
#define MQTT_USER "Temperate_InHouseBroker-Pi5" // mqtt auth username
#define MQTT_PASS "gdKR1KoS"                        // mqtt auth pass
#define MQTT_CLIENT_ID "UNOR4_1" //arbitrary
//#define MQTT_CLIENT_ID "ESP32_1"
#define MQTT_BASE_TOPIC "InHouse_UNOR4_1" // base topic, telemetry will be <base>/ntc1 etc.
//#define MQTT_BASE_TOPIC "InHouse_ESP32_1"
#define MQTT_STATUS_TOPIC MQTT_BASE_TOPIC "/status"
