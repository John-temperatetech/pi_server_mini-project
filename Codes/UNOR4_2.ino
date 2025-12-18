#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "arduino_secrets.h"
#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into pin 2 on the Arduino
#define ONE_WIRE_BUS 12
const int busIndex = 1;


//float CALIBRATION_OFFSET = (((RawValue - RawLow) * ReferenceRange) / RawRange) + ReferenceLow;
//example
/*float RawHigh = 99.6;
float RawLow = 0.5;
float ReferenceHigh = 99.9;
float ReferenceLow = 0;
float RawRange = RawHigh - RawLow;
float ReferenceRange = ReferenceHigh - ReferenceLow;*/

//Calibration
#define CALIBRATION_OFFSET 0.0

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature sensors(&oneWire);

// Time
const unsigned long updateInterval = 1000 * 5;
unsigned long updateTime;

WiFiClient wifiClient;
PubSubClient client(wifiClient);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Wire.begin();
  WiFi.begin(ssid, password);
  // Connect WiFi
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("WiFi connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Start up the library
  sensors.begin();

  // MQTT init
  client.setServer(MQTT_SERVER, MQTT_PORT);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  Serial.println(WiFi.localIP());

  // call sensors.requestTemperatures() to issue a global temperature and Requests to all devices on the bus
  sensors.requestTemperatures();
  if (millis() - updateTime >= updateInterval) {
    for (int i = 0; i < busIndex; i++) {
      float rawTempC = sensors.getTempCByIndex(i);
      float calibratedTempC = rawTempC + CALIBRATION_OFFSET;

      char tempStr[10];
      snprintf(tempStr, sizeof(tempStr), "%.2f", calibratedTempC);
      // MQTT topics
      char topicT[64];
      snprintf(topicT, sizeof(topicT), "%s/sensor%d/temp", MQTT_BASE_TOPIC, i + 1);
      client.publish(topicT, tempStr);

      if (rawTempC == -127.00) {
        Serial.println("Error: Could not read temperature data");
      } else {
        Serial.print("Sensor ");
        Serial.print(busIndex + 1);
        Serial.print(": Temp=");
        Serial.print(tempStr);
      }
    }
    updateTime = millis();
  }
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