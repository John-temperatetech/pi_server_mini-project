#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include "Adafruit_SHT4x.h"
#include <Wire.h>
#include "arduino_secrets.h"

// Time
const unsigned long updateInterval = 1000 * 30;
unsigned long updateTime;

WiFiClient espClient;
PubSubClient client(espClient);

#define PCAADDR 0x70
#define SHT4x_I2C_BUSES 4

Adafruit_SHT4x sht4_sensors[SHT4x_I2C_BUSES];

void selectI2CBus(uint8_t busIndex) {
  Wire.beginTransmission(PCAADDR);
  Wire.write(1 << busIndex);
  Wire.endTransmission();
}

void initializeSensor(uint8_t busIndex, Adafruit_SHT4x& sensor) {
  selectI2CBus(busIndex);
  if (!sensor.begin()) {
    Serial.print("Couldn't find SHT4x on bus ");
    Serial.println(busIndex);
    delay(1000);
    sensor.begin();
  }
  sensor.setPrecision(SHT4X_HIGH_PRECISION);
  sensor.setHeater(SHT4X_NO_HEATER);
}

void setup() {
  Serial.begin(9600);
  Wire.begin();
  WiFi.begin(ssid, password);

  for (uint8_t i = 0; i < SHT4x_I2C_BUSES; i++) {
    initializeSensor(i, sht4_sensors[i]);
  }

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");

  // OTA Setup
  ArduinoOTA.setHostname("ESP32_1");
  ArduinoOTA.setPassword("coldeasy");
  ArduinoOTA.onStart([]() {
    Serial.println("Start OTA update");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd OTA update");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();

  // MQTT init
  client.setServer(MQTT_SERVER, MQTT_PORT);
}

void loop() {
  ArduinoOTA.handle();
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  Serial.println(WiFi.localIP());

  // publish every 30 seconds
  if (millis() - updateTime >= updateInterval) {
    for (uint8_t i = 0; i < SHT4x_I2C_BUSES; i++) {
      readAndPublishSensorData(i, sht4_sensors[i]);
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
      client.publish("esp32_1/status", "ESP32_1 reconnected");
    }

    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5 seconds");
      delay(5000);
    }
  }
}

void readAndPublishSensorData(uint8_t busIndex, Adafruit_SHT4x& sensor) {
  selectI2CBus(busIndex);
  sensors_event_t humidity, temp;
  sensor.getEvent(&humidity, &temp);
  
  char tempStr[10], humidityStr[10];
  snprintf(tempStr, sizeof(tempStr), "%.2f", temp.temperature);
  snprintf(humidityStr, sizeof(humidityStr), "%.2f", humidity.relative_humidity);

  // MQTT publish
  char topicT[64], topicH[64];
  snprintf(topicT, sizeof(topicT), "%s/sensor%d/temp", MQTT_BASE_TOPIC, busIndex + 1);
  snprintf(topicH, sizeof(topicH), "%s/sensor%d/humidity", MQTT_BASE_TOPIC, busIndex + 1);
  client.publish(topicT, tempStr);
  client.publish(topicH, humidityStr);

  Serial.print("Sensor ");
  Serial.print(busIndex + 1);
  Serial.print(": Temp=");
  Serial.print(tempStr);
  Serial.print("°C, Humidity=");
  Serial.print(humidityStr);
  Serial.println("%");
}

