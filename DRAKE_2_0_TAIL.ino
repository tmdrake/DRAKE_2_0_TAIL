/*
 * Based On the Node32S - 4MB flash/NO-OTA! LARGE APP NEEDED. FLASH-ROM at 40Mhz.
 * Board version 2.0.17 (only works for now 12-29-2024)
 *
 * Updated July 2026:
 *   - Classic Bluetooth SPP replaced with NimBLE Nordic UART Service (NUS)
 *   - Requires NimBLE-Arduino library (Library Manager -> "NimBLE-Arduino")
 *   - Device advertises as "TMDrake_tail" with NUS service UUID
 */
#if !defined(ESP32)
#error This code is designed to run on ESP32 and ESP32-based boards! Please check your Tools->Board setting.
#endif

#include <esp_task_wdt.h>
#define WDT_TIMEOUT 30  // Timeout in seconds

#define LED_PIN 22  //Where our driver is connected to..
#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel spikes(12, LED_PIN, NEO_GRB + NEO_KHZ800);
#define MIC A0
#define OFFSET 1600       //Voltage offset for zero point on mic Input
#define MAXBRIGHTNESS 75  //For background loop [0-100%]
//#define DEBUG_MIC         /* Uncomment to Debug Mic*/

#include <Timer.h>
Timer t;


////For Coms to ->Pawbs
#define TRANSMITTER 17  //Where the ASK RF modules is attached
#include <RH_ASK.h>
#ifdef RH_HAVE_HARDWARE_SPI
#include <SPI.h>  // Not actually used but needed to compile
#endif
RH_ASK Ask_TX(2000, 0, TRANSMITTER, 0);  // ESP8266 or ESP32: do not use pin 11 or 2


//WIFI Client
#include "WiFi.h"
#include "AsyncUDP.h"
const char* ssid = "TMDRAKE";
//const char * password = "***********";
IPAddress ip(192, 168, 4, 10);  //Local IP so we dont
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);
AsyncUDP udp;
AsyncUDP udp_head_temp;
AsyncUDP udp_head_light;
////////////////////////////////////////////


//switch between sound mode when loud sound is herd
boolean soundmode = false;
unsigned long lastime = 0;
bool flashed = false;
unsigned long lastmiclevel = -1;
int head_brightness = -1;
float head_temperature = -1;
//////////////

//Settings varables................................
int sensitivity = 75;  //Default Value
int mode = 0;
bool enableSound = true;  //Disables sound//Controls detection of sound
/////////////////////////////////////////////////////

//FOR SAVING SETTINGS
#include <EEPROM.h>
#define EEPROM_SIZE 8
EEPROMClass MODE("M");
EEPROMClass SENSITIVITY("S");
EEPROMClass ENABLESOUND("E");
//Might use eppromEX to store everything from a construct

// BLE is now handled in Serial_RoutineBT.ino (NimBLE NUS)
// No more BluetoothSerial / Classic SPP

void setup() {
  // put your setup code here, to run once:

  //Init ASK Transmitter
  pinMode(TRANSMITTER, OUTPUT);
  Ask_TX.init();

  //Init LED Pin
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(MIC, INPUT);

  //Neopixel Library init
  spikes.begin();  // INITIALIZE NeoPixel spikes object (REQUIRED)
  spikes.show();   // Turn OFF all pixels ASAP

  Serial.begin(115200);
  Serial.println(__FILE__);
  Serial.println(__DATE__);
  Serial.println(__TIME__);
  Serial.println("Drake Tail....GO! (NimBLE NUS)");

  //Init WIFI (with static ip for direct communication)
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);          // Disable power-save for lower latency
  WiFi.begin(ssid, NULL);
  WiFi.config(ip, gateway, subnet);

  //RESTORE EEPROM
  Serial.println("Restoring settings..");
  if (MODE.begin(EEPROM_SIZE)) {
    MODE.get(0, mode);
    if (mode < 0 || mode >= 10) {
      mode = 0;
      MODE.put(0, mode);
      MODE.commit();
    }
    Serial.print("M:");
    Serial.println(mode);
  }
  if (SENSITIVITY.begin(EEPROM_SIZE)) {
    SENSITIVITY.get(0, sensitivity);
    if (sensitivity < 0 || sensitivity > 4000) {
      sensitivity = 75;
      SENSITIVITY.put(0, sensitivity);
      SENSITIVITY.commit();
    }
    Serial.print("S:");
    Serial.println(sensitivity);
  }
  if (ENABLESOUND.begin(EEPROM_SIZE)) {
    ENABLESOUND.get(0, enableSound);
    if (enableSound < 0 || enableSound > 1) {
      enableSound = true;
      ENABLESOUND.put(0, enableSound);
      ENABLESOUND.commit();
    }
    Serial.print("E:");
    Serial.println(enableSound);
  }

  // Start NimBLE Nordic UART Service (defined in Serial_RoutineBT.ino)
  setupBLE();

  esp_task_wdt_init(WDT_TIMEOUT * 1000, true);
  enableLoopWDT();

  /**************************************/
  if (udp_head_light.listen(1235)) {
    udp_head_light.onPacket([](AsyncUDPPacket packet) {
      head_brightness = packet.parseInt();
      if (head_brightness < 0) head_brightness = 0;
    });
  }
  if (udp_head_temp.listen(1236)) {
    udp_head_temp.onPacket([](AsyncUDPPacket packet) {
      head_temperature = packet.parseFloat();
      if (head_temperature < 0) head_temperature = 0;
    });
  }
}

void loop() {
  //Update Timer
  t.update();  //for flash and other async task

  if (!flashed) {
    //Sound Activation
    sound_detect();  //M0
  }

  /* BLE is event-driven via NimBLE callbacks - no polling needed */
  // checkSerialBT();  // kept as empty stub for compatibility
}



void sound_detect() {
  if (soundmode && enableSound) {

    mode_selector(mode);

    digitalWrite(LED_BUILTIN, HIGH);
    if (millis() - lastime > 10000) {
      soundmode = false;  //put the system back into fading mode, after 10 seconds.
      resetBrightnessandDirection();
      sendbackgroundloopReset();
    }
  } else {


    fading();
    digitalWrite(LED_BUILTIN, LOW);
    lastmiclevel = 0;
  }
  ///Check for Sound!
  long micLevel = analogRead(MIC) - OFFSET;  //adafruit offset
  if (micLevel > 100 /*TRIGGER SENSITIVITY*/) {
    soundmode = true;
    lastime = millis();  //reset our timeout
  }
}


void flash_lamp() {
  turn_all_on();
  t.after(100, turn_all_off);
  flashed = true;
}

//Below are routines to save code
void turn_all_off() {
  /*
  Turns all LED's off on the strip, resets flash off to resume animation
  */
  for (uint16_t i = 0; i < spikes.numPixels(); i++)
    spikes.setPixelColor(i, spikes.Color(0, 0, 0));
  spikes.show();
  flashed = false;
}

void turn_all_on() {
  /*Turns all LED's on on the strip*/
  for (uint16_t i = 0; i < spikes.numPixels(); i++)
    spikes.setPixelColor(i, spikes.Color(150, 150, 150));
  spikes.show();
}

void mode_selector(int mode) {
  //Selects different mode like sound, lights, etc.
  //Todo: Select modes to run the lighting programs

  switch (mode) {
    case 0:
      soundloop(millis(), 50, false);
      break;

    case 1:
      soundloop(millis(), 50, true);
      break;

    case 2:
      soundcheck();
      break;

    // Future modes 3-10 will go here

    default:
      mode = 0;
  }
}
