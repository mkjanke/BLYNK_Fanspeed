// Wemos Mini D1 Fan speed controler
//
// Reads DHT11 temperature probe and sets computer fan speed
//    based on temperature
//    using PWM motor controller
//
// Upload temperature data to Blynk if Wifi connected.
//
// Inspiration and sample code from Everlanders:
//      https://youtu.be/aS3BiYaEfiw
//      https://youtu.be/KX67lBrizPg
//
// Blynk reconnection logic via:
// https://community.blynk.cc/t/c-blynk-legacy-code-examples-for-basic-tasks/22596/20
//
// Libraries:
//    Arduino OTA
//    Blynk library Version >= 1.0.1
//    Adafruit DHT Sensor Library
//
// To get started:
//    Rename settings.h.default to settings.h 
//    Edit WiFi and Blynk parameters in settings.h

#include "fanspeed.h"

char buffer[32]; //Scratch space for Blynk text output

// Constants for uptime calculations
const uint32_t millis_in_day = 1000 * 60 * 60 * 24;
const uint32_t millis_in_hour = 1000 * 60 * 60;
const uint32_t millis_in_minute = 1000 * 60;

const char ssid[] = WIFI_SSID;
const char pass[] = WIFI_PASSWORD;
const char auth[] = BLYNK_API_KEY;

// DHT Temperature Probe setup
DHT dht(DHTPIN, DHTTYPE);

// Global vars for current temperature & humidity, updated by timers
float dhtTemp = 0.0;
float dhtHumidity = 0.0;

// Defaults for fan settings
// Overriden by Blynk Vpins
int fanSpeed = PWM_WRITE_RANGE;     //Start fan at max speed
int fanOverride = 0;                // Blynk Vpin V6
int fanStartTemp = FAN_START_TEMP;  // Blynk Vpin V7
int pwmLowSpeed = PWM_LOW_SPEED;    // Blynk Vpin V8
int fanMaxTemp = FAN_MAX_TEMP;      // Blynk Vpin V9
int pwmHighSpeed = PWM_WRITE_RANGE; // Max fan speed (= 100)
int relayOut = HIGH;

//Set up Blynk Timer and terminal widget
BlynkTimer timer;
WidgetTerminal blynkTerminal(V5);

/// Track Blynk reconnects
int ReCnctFlag;  // Reconnection Flag
int ReCnctCount = 0;  // Reconnection counter

//Blynk callbacks
BLYNK_CONNECTED() {
  // Request the latest state from the server
  Blynk.syncVirtual(V4);
  Blynk.syncVirtual(V7); // Fan start temperature
  Blynk.syncVirtual(V8); // Fan Minimum Speed (10 - 100)
  Blynk.syncVirtual(V9); // Fan max temperature 
  
  ReCnctCount = 0;
}

//D7 output. Set HIGH to close relay
BLYNK_WRITE(V3)
{
  int relayOut = param.asInt();
  digitalWrite(RELAY_OUT, relayOut);  // Should toggle relay
}

// Manual override - Reads V4 from Blynk app. If '1' then fan will run at 100%
BLYNK_WRITE(V4)
{
  fanOverride = param.asInt();
  calcFanSpeed();
  controlFanSpeed(fanSpeed); 
}

BLYNK_WRITE(V5) //Blynk Terminal widget
{
  // Input from Widget
  if (String("?") == param.asStr())
  {
    dumpESPStatus();
  }
  else
  {
    blynkTerminal.println(F("Type '?' to dump status"));
    blynkTerminal.flush();
  }
}

// Reads V7 from Blynk app. Sets/overrides Temp at which fan will start
BLYNK_WRITE(V7)
{
  fanStartTemp = (param.asInt() <= fanMaxTemp) ? param.asInt() : fanMaxTemp; 
  calcFanSpeed();
  controlFanSpeed(fanSpeed);   
}

// Reads V8 from Blynk app. Sets/overrides fan low speed
BLYNK_WRITE(V8)
{
  pwmLowSpeed = (param.asInt() <= pwmHighSpeed) ? param.asInt() : pwmHighSpeed; 
  calcFanSpeed();
  controlFanSpeed(fanSpeed);
}

// Reads V9 from Blynk app. Sets/overrides temp at which fan will reach max speed
BLYNK_WRITE(V9)
{
  fanMaxTemp = (param.asInt() >= fanStartTemp) ? param.asInt() : fanMaxTemp;
  calcFanSpeed();
  controlFanSpeed(fanSpeed); 
}

void setup()
{
  // Debug console
  Serial.begin(9600);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(RELAY_OUT, OUTPUT);
  digitalWrite(RELAY_OUT, HIGH);  //Set relay pin high to hold relay open

  timer.setInterval(LED_HEARTBEAT, toggleLed);

  // Set up PWM for fan control
  analogWriteRange(PWM_WRITE_RANGE);      //Fan range 0-100
  analogWriteFreq(PWM_FREQUENCY);         //Set PWN frequency
  digitalWrite(MOTOR_IN1, HIGH);          //Start with fan on high

  // PWM Fan control pins
  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);
  
  //Set up DHT11 sensor
  dht.begin();
  timer.setInterval(DHT_READ_INTERVAL, dht11Read);
  
  // Non-blocking Wifi connection
  WiFi.begin(ssid, pass);

  // Initial Blynk Connection
  // Using Blynk.config and Blynk.connect instead of Blynk.start()
  // so that loop() will still run when disconnected from WiFi
  Blynk.config(auth, BLYNK_DEFAULT_DOMAIN, BLYNK_DEFAULT_PORT);
  Blynk.connect(); //reconnect logic handled in loop()

  timer.setInterval(MY_BLYNK_HEARTBEAT, heartBeat);

  ArduinoOTA.setHostname(BLYNK_DEVICE_NAME);
  ArduinoOTA.begin();

  dumpESPStatus();
}

void loop()
{
  timer.run();
  ArduinoOTA.handle();

  if (Blynk.connected())  // If connected run as normal
  {  
    Blynk.run();
  } 
  else if (ReCnctFlag == 0)  // If NOT connected and not already trying to reconnect, set timer to try to reconnect in 60 seconds
  {
    ReCnctFlag = 1;  // Set reconnection Flag
    Serial.println("Starting reconnection timer in 60 seconds...");

    timer.setTimeout(60000L, []() {  // Lambda Reconnection Timer Function
      ReCnctFlag = 0;  // Reset reconnection Flag
      ReCnctCount++;  // Increment reconnection Counter
      Serial.print("Attempting reconnection #");
      Serial.println(ReCnctCount);
      Blynk.connect();  // Try to reconnect to the server
    });  // END Timer Function
  }
}

/* Function Definitions */
void controlFanSpeed(int percent)
{
  if (fanOverride)
    fanSpeed = PWM_WRITE_RANGE; //Full speed
  else
    fanSpeed = percent;

  analogWrite(MOTOR_IN1, fanSpeed);
  Serial.print("Fan Speed: ");
  Serial.println(fanSpeed);

  Blynk.virtualWrite(V6, fanSpeed);
  dumpSensorStatus();
}

// Called by Blynk timer. 
// Lets me know it's running
void toggleLed()
{
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}

// Calc and update device uptime in Blynk
void heartBeat()
{
  uint8_t days = millis() / (millis_in_day);
  uint8_t hours = (millis() - (days * millis_in_day)) / millis_in_hour;
  uint8_t minutes = (millis() - (days * millis_in_day) - (hours * millis_in_hour)) / millis_in_minute;
  snprintf(buffer, sizeof(buffer), "Uptime: %2dd%2dh%2dm", days, hours, minutes);
  Blynk.virtualWrite(V0, buffer);
}

//DHT11 temp sensor read, Blynk update
void dht11Read()
{
  // Read Temp & Humidity
  float newH = dht.readHumidity();
  float newT = dht.readTemperature(DHT_TEMP_F);

  // Check if reading was successful
  if (isnan(newT) || isnan(newH)) 
  {
    blynkTerminal.println(F("Failed to read from DHT sensor!"));
    blynkTerminal.flush();
    fanSpeed = PWM_WRITE_RANGE; //If invalid temp, run fan at full speed
  }
  else 
  {
    dhtTemp = newT;
    dhtHumidity = newH;
    Blynk.virtualWrite(V1, dhtTemp);
    Blynk.virtualWrite(V2, dhtHumidity);
    calcFanSpeed();
  }
  controlFanSpeed(fanSpeed);
  // dumpSensorStatus();
}

// Calc fan speed based on current temp and fan parameters.
void calcFanSpeed(){
  if (dhtTemp < fanStartTemp)
    fanSpeed = 0;
  else
    if (dhtTemp > fanMaxTemp)
      fanSpeed = pwmHighSpeed;
    else
      // Linear fan speed. I.E 75F to 90F will result in fan speed from 40% to 100%
      fanSpeed = map(dhtTemp, fanStartTemp, fanMaxTemp, pwmLowSpeed, pwmHighSpeed);
}

// Output device status to Blynk terminal widget
void dumpESPStatus()
{
  blynkTerminal.println(F("Blynk Library v" BLYNK_VERSION));

  snprintf(buffer, sizeof(buffer), "IP: %s", WiFi.localIP().toString().c_str());
  blynkTerminal.println(buffer);
  snprintf_P(buffer, sizeof(buffer), PSTR("Last Reset: %s"), ESP.getResetInfo().c_str());
  blynkTerminal.println(buffer);
  blynkTerminal.flush();

  snprintf_P(buffer, sizeof(buffer), PSTR("ESP Core: %s\nFirm: %s"), ESP.getCoreVersion().c_str(), BLYNK_FIRMWARE_VERSION);
  blynkTerminal.println(buffer);
  snprintf_P(buffer, sizeof(buffer), PSTR("Flash: %dkB\nSketch: %dkB"), ESP.getFlashChipRealSize()/1024, ESP.getSketchSize()/1024);
  blynkTerminal.println(buffer);
  blynkTerminal.flush();

  snprintf_P(buffer, sizeof(buffer), PSTR("Heap Mem: %dkb"), ESP.getFreeHeap() / 1024);
  blynkTerminal.println(buffer);
  snprintf_P(buffer, sizeof(buffer), PSTR("WiFi Strength: %ddB"), WiFi.RSSI());
  blynkTerminal.println(buffer);
  snprintf(buffer, sizeof(buffer), "VCC: %3.2fV", ESP.getVcc() / 1024.0);
  blynkTerminal.println(buffer);

  uint8_t days = millis() / (millis_in_day);
  uint8_t hours = (millis() - (days * millis_in_day)) / millis_in_hour;
  uint8_t minutes = (millis() - (days * millis_in_day) - (hours * millis_in_hour)) / millis_in_minute;
  snprintf(buffer, sizeof(buffer), "Uptime: %2dd%2dh%2dm", days, hours, minutes);
  blynkTerminal.println(buffer);

  blynkTerminal.flush();
}

// Output sensor status to terminal widget
void dumpSensorStatus()
{
  uint8_t days = millis() / (millis_in_day);
  uint8_t hours = (millis() - (days * millis_in_day)) / millis_in_hour;
  uint8_t minutes = (millis() - (days * millis_in_day) - (hours * millis_in_hour)) / millis_in_minute;
  snprintf(buffer, sizeof(buffer), "Uptime: %2dd%2dh%2dm", days, hours, minutes);
  blynkTerminal.println(buffer);

  snprintf_P(buffer, sizeof(buffer), PSTR("Temp: %3.1f Humidity: %3.1f"), dhtTemp, dhtHumidity);
  blynkTerminal.println(buffer);
  snprintf_P(buffer, sizeof(buffer), PSTR("Fan Speed: %d%"), fanSpeed);
  blynkTerminal.println(buffer);
  blynkTerminal.flush();
}