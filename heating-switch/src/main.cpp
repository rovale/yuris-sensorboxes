#include <SimpleInternetThing.h> // SimpleInternetThing library (https://github.com/rovale/SimpleInternetThing).
#include <ArduinoJson.h>         // Take a version 5, not the version 6 beta.
#include <Secrets.h>             // Add a secrets file.

const char mqttTopicBase[] = "rovale/vkv";
const char id[] = "heating-switch"; // This is added to the MQTT topic name.
const char name[] = "The Heating Switch SimpleInternetThing";
const char version[] = "0.0.1";

const int indicatorLedPin = 5;

const int relayPin = 17;

SimpleInternetThing simpleSensorBox(
    mqttTopicBase, id, name, version,
    ssid, wiFiPassword,
    mqttServer, mqttPort, caCert, mqttUsername, mqttPassword,
    indicatorLedPin);

unsigned long maxOnDuration = 75000;
unsigned long lastTurnedOnAt = -1 * maxOnDuration;
bool isOn = true;

String getTelemetryMessage()
{
  StaticJsonBuffer<400> jsonBuffer;
  JsonObject &jsonObject = jsonBuffer.createObject();

  jsonObject["isOn"] = isOn;

  String jsonString;
  jsonObject.printTo(jsonString);
  return jsonString;
}

void turnOn()
{
    lastTurnedOnAt = millis();
    if (!isOn) {
        isOn = true;
        digitalWrite(relayPin, HIGH);
        simpleSensorBox.publishData("telemetry/status", getTelemetryMessage());
    }
}

void turnOff()
{
    digitalWrite(relayPin, LOW);
    if (isOn)
    {
        isOn = false;
        simpleSensorBox.publishData("telemetry/status", getTelemetryMessage());
    }
}

void onCommand(String commandName, JsonObject &root)
{
  if (commandName == "turnOn")
  {
      turnOn();
  }
  else if (commandName == "turnOff")
  {
      turnOff();
  }
  else
  {
    Serial.println("Unknown command.");
  }
}

void setup()
{
  pinMode(relayPin, OUTPUT);
  turnOff();

  Serial.begin(115200);
  Serial.println();
  Serial.print("Sensor ");
  Serial.print(id);
  Serial.print(" version ");
  Serial.print(version);
  Serial.println(".");

  simpleSensorBox.setup();
  simpleSensorBox.inverseIndicatorLed();
  simpleSensorBox.onCommand(onCommand);
}

void loop()
{
  simpleSensorBox.loop();

  if (millis() - lastTurnedOnAt >= maxOnDuration)
  {
      turnOff();
  }
}