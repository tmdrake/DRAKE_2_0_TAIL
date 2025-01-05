/*
 * Based On the Node32S - 4MB flash/NO-OTA! LARGE APP NEEDED. FLASH-ROM at 40Mhz.
 * Board version 2.0.17 (only works for now 12-29-2024)
 * 
 */
#if !defined(ESP32)
#error This code is designed to run on ESP32 and ESP32-based boards! Please check your Tools->Board setting.
#endif

#include <esp_task_wdt.h>
#define WDT_TIMEOUT 30  // Timeout in seconds
//esp_err_t ESP32_ERROR;

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

//Setup Blutooth
#include "BluetoothSerial.h"
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#if !defined(CONFIG_BT_SPP_ENABLED)
#error Serial Bluetooth not available or not enabled. It is only available for the ESP32 chip.
#endif

BluetoothSerial SerialBT;
boolean confirmRequestPending = true;

void setup() {
  // put your setup code here, to run once:
  //Configure Bluetooth
  SerialBT.enableSSP();
  SerialBT.onConfirmRequest(BTConfirmRequestCallback);
  SerialBT.onAuthComplete(BTAuthCompleteCallback);
  SerialBT.begin("TMDrake_tail");  //Bluetooth device name

  //Init ASK Transmitter
  pinMode(TRANSMITTER, OUTPUT);
  Ask_TX.init();

  //Init LED Pin
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(MIC, INPUT);

  //Neopixel Library init
  spikes.begin();  // INITIALIZE NeoPixel spikes object (REQUIRED)
  spikes.show();   // Turn OFF all pixels ASAP
  //spikes.setBrightness(200); //need full nightness

  //Enable timer
  //t.every(1, soundcheck);

  //Serial.begin(115200);
  Serial.begin(115200);
  Serial.println(__FILE__);
  Serial.println(__DATE__);
  Serial.println(__TIME__);
  Serial.println("Drake Tail....GO!");

  //Init WIFI (with static ip for direct communication)
  WiFi.mode(WIFI_STA);
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


  //esp_task_wdt_deinit();
  // // Task Watchdog configuration
  // esp_task_wdt_config_t wdt_config = {
  //   .timeout_ms = WDT_TIMEOUT * 1000,                 // Convertin ms
  //   .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,  // Bitmask of all cores, https://github.com/espressif/esp-idf/blob/v5.2.2/examples/system/task_watchdog/main/task_watchdog_example_main.c
  //   .trigger_panic = true                             // Enable panic to restart ESP32
  //  };
  // // WDT Init
  // ESP32_ERROR = esp_task_wdt_init(&wdt_config);
  //  Serial.println("Last Reset : " + String(esp_err_to_name(ESP32_ERROR)));
  //  esp_task_wdt_add(NULL);  //add current thread to WDT watch
  esp_task_wdt_init(WDT_TIMEOUT * 1000, true);  //conifgure the wdt for 30 seconds before firing off
  enableLoopWDT();

  /**************************************/
  if (udp_head_light.listen(1235)) {
    udp_head_light.onPacket([](AsyncUDPPacket packet) {
      //Serial.print("UDP Packet Type: ");
      //Serial.println(packet.isBroadcast()?"Broadcast":packet.isMulticast()?"Multicast":"Unicast");
      // Serial.print(", From: ");
      // Serial.print(packet.remoteIP());
      // Serial.print(":");
      // Serial.print(packet.remotePort());
      // Serial.print(", To: ");
      // Serial.print(packet.localIP());
      // Serial.print(":");
      // Serial.print(packet.localPort());
      //Serial.print(", Length: ");
      //Serial.println(packet.length());
      //Serial.print("Head Light (0-1024): ");
      //String headlight = String(packet.parseInt());
      //Serial.write(packet.data(), packet.length());
      //Serial.println(packet.parseInt());
      head_brightness = packet.parseInt();
      if (head_brightness < 0) head_brightness = 0;
    });
  }
  if (udp_head_temp.listen(1236)) {
    //Serial.print("UDP Listening on IP: ");
    //Serial.println(WiFi.localIP());
    udp_head_temp.onPacket([](AsyncUDPPacket packet) {
      //Serial.print("UDP Packet Type: ");
      //Serial.println(packet.isBroadcast()?"Broadcast":packet.isMulticast()?"Multicast":"Unicast");
      // Serial.print(", From: ");
      // Serial.print(packet.remoteIP());
      // Serial.print(":");
      // Serial.print(packet.remotePort());
      // Serial.print(", To: ");
      // Serial.print(packet.localIP());
      // Serial.print(":");
      // Serial.print(packet.localPort());
      //.print(", Length: ");
      //Serial.println(packet.length());
      //Serial.print("Head Temp: ");
      //Serial.print(packet.parseFloat());
      //Serial.println();
      head_temperature = packet.parseFloat();
      if (head_temperature < 0) head_temperature = 0;
    });
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  //rainbow(10); ///using a 10ms delay

  //Update Timer
  t.update(); //for flash and aother async task
  ///////////////?/


  if (!flashed) {
    //Sound Activation
    //soundcheck(); //M1
    sound_detect();  //M0
    ///////////////////////
    //mode_selector(mode);
  }

  /*Handles the BT interface task*/
  checkSerialBT();  //check serial_BT task

  //Handling Confirmation of SSP
  if (confirmRequestPending)
    SerialBT.confirmReply(true);
  //******************************
}



void sound_detect() {
  if (soundmode && enableSound) {

    //soundloop(millis(), 50, false);  //false = color phase, true = distinct colors
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
    //Serial.println(micLevel);
    //delay(10);
  if (micLevel > 100 /*TRIGGER SENSITIVITY*/) {
    soundmode = true;
    lastime = millis();  //reset our timeout
  }
}


void flash_lamp() {
  turn_all_on();
  t.after(100, turn_all_off);
  flashed = true;
  //delay(100); //Need to do this without delay

  //turn_all_off();
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


void BTConfirmRequestCallback(uint32_t numVal) {
  confirmRequestPending = true;
  Serial.println(numVal);
}

void BTAuthCompleteCallback(boolean success) {
  confirmRequestPending = false;
  if (success) {
    Serial.println("Pairing success!!");
  } else {
    Serial.println("Pairing failed, rejected by user!!");
  }
}

void mode_selector(int mode) {
  //Selects different mode like sound, lights, etc.

  switch (mode) {
    case 0:
      soundloop(millis(), 50, false);
      break;

    case 1:
      soundloop(millis(), 50, true);
      //xmas_fading();
      break;

    case 2:
      //fading();
      soundcheck();
      break;

    // case 3:
    //   //color_paturn_1();
    //   break;

    // case 4:
    //   //accelerometer_detect(); //Color by Acceleration
    //   break;

    // case 5:
    //   //sound_vu(); //close to an off state

    //   break;

    // case 6:
    //   //police_mode(); //close to an off state
    //   break;
    default:
      mode = 0;
  }
}
