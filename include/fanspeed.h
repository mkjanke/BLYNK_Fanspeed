#include <Arduino.h>

//Arduino OTA includes
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// DHT Sensor includes and defines
#include <Adafruit_Sensor.h>
#include <DHT.h>

//Local settings and parameters
#include "settings.h"

// Must follow settings.h or Blynk will use wrong API config
#include <BlynkSimpleEsp8266.h>

// Function prototypes
void dht11Read();       //Read Temp sensor & update Blynk
void toggleLed();       // Toggle LED
void heartBeat();       // Update Blynk with heartbeat message
void dumpESPStatus();      //Dump ESP8266 status to terminal window
void dumpSensorStatus();      //Dump Sensor status to terminal window
void controlFanSpeed(int percent); 

//So ESP.getVCC() reads correct voltage
ADC_MODE(ADC_VCC);

