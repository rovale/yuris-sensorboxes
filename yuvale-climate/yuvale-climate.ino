#include <SensorBox.h>   // SensorBox library (https://github.com/rovale/SensorBox)
#include <ArduinoJson.h> // Take a version 5, not the version 6 beta
#include "DHTesp.h"

#include <Secrets-vkv.h>

const char mqttTopicBase[] = "yuvale/vkv";
const char sensorId[] = "cte1";
const char sensorName[] = "Ultra nerdy super duper floeper sloeper climate checker";
const char version[] = "0.0.2";

const int indicatorLedPin = 25;

SensorBox sensorBox(
    mqttTopicBase, sensorId, sensorName, version,
    ssid, wiFiPassword,
    mqttServer, 1883, mqttUsername, mqttPassword,
    indicatorLedPin);

const int dhtPin = 13;
DHTesp dht;

const int lightPin = 36;
const int rainPin = 35;

unsigned long lastTelemetryMessageAt = -15000;

void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.print("Sensor ");
  Serial.print(sensorId);
  Serial.print(" version ");
  Serial.println(version);

  sensorBox.setup();

  pinMode(lightPin, INPUT);
  pinMode(rainPin, INPUT);
  dht.setup(dhtPin, DHTesp::DHT22);
}

long analogSample(int pin)
{
  int samples = 50;
  unsigned long sum = 0;
  for (byte i = 0; i < samples; i++)
  {
    sum = sum + analogRead(pin);
    delayMicroseconds(500);
  }

  return sum / samples;
}

String getClimateMessage()
{
  StaticJsonBuffer<400> jsonBuffer;
  JsonObject &jsonObject = jsonBuffer.createObject();

  TempAndHumidity dhtValues = dht.getTempAndHumidity();

  jsonObject["humidity"] = dhtValues.humidity;
  jsonObject["temperature"] = dhtValues.temperature;
  jsonObject["light"] = analogSample(lightPin);
  jsonObject["rain"] = analogSample(rainPin);

  String jsonString;
  jsonObject.printTo(jsonString);
  return jsonString;
}

void loop()
{
  sensorBox.loop();

  if (millis() - lastTelemetryMessageAt >= 15000)
  {
    lastTelemetryMessageAt = millis();
    sensorBox.publishTelemetryData("climate", getClimateMessage());
  }
}
