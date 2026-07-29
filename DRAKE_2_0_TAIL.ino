/*
 * Based On the Node32S - 4MB flash/NO-OTA! LARGE APP NEEDED. FLASH-ROM at 40Mhz.
 * Board version 2.0.17
 *
 * Updated July 2026:
 *   - NimBLE NUS + modes 0-10 + B/V controls
 *   - Binary unicast mic stream to Head
 *   - Live STAT push to app over BLE
 */
#if !defined(ESP32)
#error This code is designed to run on ESP32 and ESP32-based boards! Please check your Tools->Board setting.
#endif

#include <esp_task_wdt.h>
#define WDT_TIMEOUT 30

#define LED_PIN 22
#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel spikes(12, LED_PIN, NEO_GRB + NEO_KHZ800);
#define MIC A0
#define OFFSET 1600
#define MAXBRIGHTNESS 75

#include <Timer.h>
Timer t;

#define TRANSMITTER 17
#include <RH_ASK.h>
#ifdef RH_HAVE_HARDWARE_SPI
#include <SPI.h>
#endif
RH_ASK Ask_TX(2000, 0, TRANSMITTER, 0);

#include "WiFi.h"
#include "AsyncUDP.h"
const char* ssid = "TMDRAKE";
IPAddress ip(192, 168, 4, 10);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);
AsyncUDP udp;
AsyncUDP udp_head_temp;
AsyncUDP udp_head_light;

boolean soundmode = false;
unsigned long lastime = 0;
bool flashed = false;
unsigned long lastmiclevel = -1;
int head_brightness = -1;
float head_temperature = -1;

int sensitivity = 75;
int mode = 0;
int lastMode = -1;
bool enableSound = true;
int masterBrightness = 80;
int animSpeed = 50;

#include <EEPROM.h>
#define EEPROM_SIZE 8
EEPROMClass MODE("M");
EEPROMClass SENSITIVITY("S");
EEPROMClass ENABLESOUND("E");
EEPROMClass BRIGHTNESS("B");
EEPROMClass SPEED("V");

void applyMasterBrightness() {
  uint8_t scaled = map(constrain(masterBrightness, 0, 100), 0, 100, 0, 255);
  spikes.setBrightness(scaled);
  spikes.show();
}

void setup() {
  pinMode(TRANSMITTER, OUTPUT);
  Ask_TX.init();

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(MIC, INPUT);

  spikes.begin();
  spikes.show();

  Serial.begin(115200);
  Serial.println(__FILE__);
  Serial.println(__DATE__);
  Serial.println(__TIME__);
  Serial.println("Drake Tail....GO! (binary mic + live STAT)");

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(ssid, NULL);
  WiFi.config(ip, gateway, subnet);

  Serial.println("Restoring settings..");
  if (MODE.begin(EEPROM_SIZE)) {
    MODE.get(0, mode);
    if (mode < 0 || mode > 10) { mode = 0; MODE.put(0, mode); MODE.commit(); }
    Serial.print("M:"); Serial.println(mode);
  }
  if (SENSITIVITY.begin(EEPROM_SIZE)) {
    SENSITIVITY.get(0, sensitivity);
    if (sensitivity < 0 || sensitivity > 4000) { sensitivity = 75; SENSITIVITY.put(0, sensitivity); SENSITIVITY.commit(); }
    Serial.print("S:"); Serial.println(sensitivity);
  }
  if (ENABLESOUND.begin(EEPROM_SIZE)) {
    ENABLESOUND.get(0, enableSound);
    if (enableSound < 0 || enableSound > 1) { enableSound = true; ENABLESOUND.put(0, enableSound); ENABLESOUND.commit(); }
    Serial.print("E:"); Serial.println(enableSound);
  }
  if (BRIGHTNESS.begin(EEPROM_SIZE)) {
    BRIGHTNESS.get(0, masterBrightness);
    if (masterBrightness < 0 || masterBrightness > 100) { masterBrightness = 80; BRIGHTNESS.put(0, masterBrightness); BRIGHTNESS.commit(); }
    Serial.print("B:"); Serial.println(masterBrightness);
  }
  if (SPEED.begin(EEPROM_SIZE)) {
    SPEED.get(0, animSpeed);
    if (animSpeed < 0 || animSpeed > 100) { animSpeed = 50; SPEED.put(0, animSpeed); SPEED.commit(); }
    Serial.print("V:"); Serial.println(animSpeed);
  }

  applyMasterBrightness();
  setupBLE();

  esp_task_wdt_init(WDT_TIMEOUT * 1000, true);
  enableLoopWDT();

  // Head → Tail feedback (unicast preferred on Head side now)
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
  t.update();

  if (!flashed) {
    sound_detect();
  }

  // Live telemetry to the app (~2 Hz when connected)
  pushLiveStatus();
}

void sound_detect() {
  if (mode >= 3 && mode <= 10) {
    mode_selector(mode);
    digitalWrite(LED_BUILTIN, LOW);
    return;
  }

  if (soundmode && enableSound) {
    mode_selector(mode);
    digitalWrite(LED_BUILTIN, HIGH);
    if (millis() - lastime > 10000) {
      soundmode = false;
      resetBrightnessandDirection();
      sendbackgroundloopReset();
    }
  } else {
    fading();
    digitalWrite(LED_BUILTIN, LOW);
    lastmiclevel = 0;
  }

  long micLevel = analogRead(MIC) - OFFSET;
  if (micLevel > 100) {
    soundmode = true;
    lastime = millis();
  }
}

void flash_lamp() {
  turn_all_on();
  t.after(100, turn_all_off);
  flashed = true;
}

void turn_all_off() {
  for (uint16_t i = 0; i < spikes.numPixels(); i++)
    spikes.setPixelColor(i, spikes.Color(0, 0, 0));
  spikes.show();
  flashed = false;
}

void turn_all_on() {
  for (uint16_t i = 0; i < spikes.numPixels(); i++)
    spikes.setPixelColor(i, spikes.Color(150, 150, 150));
  spikes.show();
}

void mode_selector(int m) {
  if (m != lastMode) {
    resetModeState();
    lastMode = m;
  }

  switch (m) {
    case 0:  soundloop(millis(), 50, false); break;
    case 1:  soundloop(millis(), 50, true);  break;
    case 2:  soundcheck();                  break;
    case 3:  mode_rainbow_chase();           break;
    case 4:  mode_comet();                   break;
    case 5:  mode_breathing();               break;
    case 6:  mode_fire();                    break;
    case 7:  mode_sparkle();                 break;
    case 8:  mode_wave();                    break;
    case 9:  mode_solid();                   break;
    case 10: mode_off();                     break;
    default: mode = 0;                       break;
  }
}
