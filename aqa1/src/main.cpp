#include <SimpleInternetThing.h> // SimpleInternetThing library (https://github.com/rovale/SimpleInternetThing)
#include <ArduinoJson.h>         // Take a version 5, not the version 6 beta
#include <Wire.h>                // I2C library
#include <ccs811.h>              // CCS811 library (https://github.com/maarten-pennings/CCS811)

#include <Secrets.h>

const char mqttTopicBase[] = "yuvale/vkv";
const char id[] = "aqa1";
const char name[] = "Ultra nerdy super duper floeper sloeper air quality alarm";
const char version[] = "0.0.2";
const int indicatorLedPin = 25;

SimpleInternetThing aqa1(
    mqttTopicBase, id, name, version,
    ssid, wiFiPassword,
    mqttServer, mqttPort, caCert, mqttUsername, mqttPassword,
    indicatorLedPin);

const int sdaPin = 21;
const int sclPin = 22;
const int wakePin = -1;

unsigned long lastTelemetryMessageAt = -15000;
unsigned long lastLedUpdate = -1000;
unsigned long ccs811Updates = 0;

CCS811 ccs811(wakePin);
int ledPins[] = {32, 33, 25, 26, 27, 14, 13, 23, 19, 18, 5, 17, 16, 4, 2};
int ledCount = sizeof(ledPins) / sizeof(int);

void onCommand(String commandName, JsonObject &root)
{
  if (commandName == "updateCcs811")
  {
    if (root.is<float>("temperature") && root.is<float>("humidity"))
    {
      float temperature = root.get<float>("temperature");
      float humidity = root.get<float>("humidity");

      int t = temperature + 25 * 512;
      int h = humidity * 512;
      if (ccs811.set_envdata(t, h))
      {
        ccs811Updates++;
      }
    }
  }
  else
  {
    Serial.println("Unknown command.");
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.print("Sensor ");
  Serial.print(id);
  Serial.print(" version ");
  Serial.println(version);

  aqa1.setup();
  aqa1.onCommand(onCommand);

  Wire.begin(sdaPin, sclPin);
  bool ok = ccs811.begin();
  if (!ok)
    Serial.println("setup: CCS811 begin FAILED");
  ok = ccs811.start(CCS811_MODE_1SEC);
  if (!ok)
    Serial.println("setup: CCS811 start FAILED");

  for (int i = 0; i <= ledCount; i++)
  {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], HIGH);
    delay(100);
  }
  delay(1000);
  for (int i = 0; i <= ledCount; i++)
  {
    digitalWrite(ledPins[i], LOW);
    delay(100);
  }

  delay(1000);
}

String getClimateMessage(unsigned int co2, unsigned int tvoc, unsigned int raw)
{
  StaticJsonBuffer<400> jsonBuffer;
  JsonObject &jsonObject = jsonBuffer.createObject();

  jsonObject["co2"] = co2;
  jsonObject["tvoc"] = tvoc;
  jsonObject["raw"] = raw;
  jsonObject["ccs811Updates"] = ccs811Updates;

  String jsonString;
  jsonObject.printTo(jsonString);
  return jsonString;
}

unsigned int co2Values[30];
unsigned int rawDataValues[30];
unsigned int tvocValues[30];
byte measurementCount = 0;

void loop()
{
  unsigned long currentMillis = millis();

  aqa1.loop();

  if (currentMillis - lastLedUpdate >= 1000)
  {
    lastLedUpdate = currentMillis;

    uint16_t co2, tvoc, errstat, raw;
    ccs811.read(&co2, &tvoc, &errstat, &raw);

    if (errstat == CCS811_ERRSTAT_OK)
    {
      co2Values[measurementCount] = co2;
      rawDataValues[measurementCount] = raw;
      tvocValues[measurementCount] = tvoc;
      measurementCount++;

      int value = map(co2, 400, 2500, 0, ledCount);
 
      for (int i = 0; i <= ledCount; i++)
      {
        if (i <= value)
        {
          digitalWrite(ledPins[i], HIGH);
        }
        else
        {
          digitalWrite(ledPins[i], LOW);
        }
      }

      Serial.print("value=");
      Serial.print(value);
      Serial.print(" ");
      Serial.print("co2=");
      Serial.print(co2);
      Serial.print(" ppm  ");
      Serial.print("tvoc=");
      Serial.print(tvoc);
      Serial.print(" ppb  ");
      Serial.println();

      if (currentMillis - lastTelemetryMessageAt >= 15000)
      {
        unsigned long co2Sum = 0;
        unsigned long rawDataSum = 0;
        unsigned long tvocSum = 0;
        for (int i = 0; i < measurementCount; i++)
        {
          co2Sum += co2Values[i];
          rawDataSum += rawDataValues[i];
          tvocSum += tvocValues[i];
        }
        unsigned int co2Average = co2Sum / measurementCount;
        unsigned int rawDataAverage = rawDataSum / measurementCount;
        unsigned int tvocAverage = tvocSum / measurementCount;

        lastTelemetryMessageAt = currentMillis;
        aqa1.publishData("telemetry/climate", getClimateMessage(co2Average, tvocAverage, rawDataAverage));

        measurementCount = 0;
      }
    }
    else if (errstat == CCS811_ERRSTAT_OK_NODATA)
    {
      Serial.println("CCS811: waiting for (new) data");
    }
    else if (errstat & CCS811_ERRSTAT_I2CFAIL)
    {
      Serial.println("CCS811: I2C error");
    }
    else
    {
      Serial.print("CCS811: errstat=");
      Serial.print(errstat, HEX);
      Serial.print("=");
      Serial.println(ccs811.errstat_str(errstat));
    }
  }
}
