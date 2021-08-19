# BLYNK Fanspeed

Wemos Mini D1 Fan speed controler

Reads DHT11 temperature probe and sets computer fan speed using PWM motor controller

Uploads temperature data to Blynk if Wifi connected.

Inspiration and sample code from Everlanders:

      https://youtu.be/aS3BiYaEfiw
      https://youtu.be/KX67lBrizPg

Blynk reconnection logic via:

      https://community.blynk.cc/t/c-blynk-legacy-code-examples-for-basic-tasks/22596/20

## Libraries:
    Arduino OTA
    Blynk libraries Version >= 1.0.1
    Adafruit DHT Sensor Library

## To get started:
    Rename setup.h.default to setup.h 
    Edit WiFi and Blynk parameters in setings.h

## Blynk Vpin Assignments

      V0  Uptime/Heartbeat (String)
      V1  DHT11 Temperature (Double, Degrees F)
      V2  DHT11 Humidity (Double, Min 0 Max 100)
      V3  Unused
      V4  Fan speed override (Integer, Min 0 Max 1)
      V5  Blynk Terminal Widget
      V6  Fan Speed (Integer, Percent, Min 0 Max 100)
