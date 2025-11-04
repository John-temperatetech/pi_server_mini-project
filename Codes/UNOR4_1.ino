#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_BME280.h>
#include "arduino_secrets.h"

// Time
const unsigned long updateInterval = 1000 * 30;
unsigned long updateTime;

WiFiClient wifiClient;
PubSubClient client(wifiClient);

#define PCAADDR 0x70
#define BME280_I2C_BUSES 1

Adafruit_BME280 bme_sensors[BME280_I2C_BUSES];

void selectI2CBus(uint8_t busIndex) {
  Wire.beginTransmission(PCAADDR);
  Wire.write(1 << busIndex);
  Wire.endTransmission();
}

void initializeSensor(uint8_t busIndex, Adafruit_BME280 &sensor) {
  selectI2CBus(busIndex);
  if (!sensor.begin(0x76)) {  // default BME280 I2C address
    Serial.print("Couldn't find BME280 on bus ");
    Serial.println(busIndex);
    delay(1000);
    sensor.begin(0x76);  // retry once
  }
}

void setup() {
  Serial.begin(9600);
  Wire.begin();
  WiFi.begin(ssid, password);
    // Initialize BME280 sensors
    for (uint8_t i = 0; i < BME280_I2C_BUSES; i++) {
    initializeSensor(i, bme_sensors[i]);
  }

  // Connect WiFi
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("WiFi connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // MQTT init
  client.setServer(MQTT_SERVER, MQTT_PORT);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  Serial.println(WiFi.localIP());

  // publish every 30 seconds
  if (millis() - updateTime >= updateInterval) {
    for (uint8_t i = 0; i < BME280_I2C_BUSES; i++) {
      readAndPublishSensorData(i, bme_sensors[i]);
    }
    updateTime = millis();
    delay(500);
  }

  delay(1000);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS)) {
      Serial.println("connected");
      client.publish("uno_r4/status", "Uno R4 connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5 seconds");
      delay(5000);
    }
  }
}

void readAndPublishSensorData(uint8_t busIndex, Adafruit_BME280 &sensor) {
  selectI2CBus(busIndex);

  float temperature = sensor.readTemperature();
  float humidity = sensor.readHumidity();
  float pressure = sensor.readPressure() / 100.0F;  // hPa

  char tempStr[10], humidityStr[10], pressureStr[10];
  snprintf(tempStr, sizeof(tempStr), "%.2f", temperature);
  snprintf(humidityStr, sizeof(humidityStr), "%.2f", humidity);
  snprintf(pressureStr, sizeof(pressureStr), "%.2f", pressure);

  // MQTT topics
  char topicT[64], topicH[64], topicP[64];
  snprintf(topicT, sizeof(topicT), "%s/sensor%d/temp", MQTT_BASE_TOPIC, busIndex + 1);
  snprintf(topicH, sizeof(topicH), "%s/sensor%d/humidity", MQTT_BASE_TOPIC, busIndex + 1);
  snprintf(topicP, sizeof(topicP), "%s/sensor%d/pressure", MQTT_BASE_TOPIC, busIndex + 1);

  // Strip newline if present
  tempStr[strcspn(tempStr, "\r\n")] = 0;
  humidityStr[strcspn(humidityStr, "\r\n")] = 0;
  pressureStr[strcspn(pressureStr, "\r\n")] = 0;

  client.publish(topicT, tempStr);
  client.publish(topicH, humidityStr);
  client.publish(topicP, pressureStr);

  Serial.print("Sensor ");
  Serial.print(busIndex + 1);
  Serial.print(": Temp=");
  Serial.print(tempStr);
  Serial.print("°C, Humidity=");
  Serial.print(humidityStr);
  Serial.print("%, Pressure=");
  Serial.print(pressureStr);
  Serial.println(" hPa");
}
