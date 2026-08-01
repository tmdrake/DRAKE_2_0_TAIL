/*
 * Drake Tail – ESP32
 * Color C + themes T supported for app team UI
 */
#if !defined(ESP32)
#error This code is designed to run on ESP32!
#endif

#include <esp_task_wdt.h>
#include <esp_now.h>
#include <esp_wifi.h>
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
int micGate = 100;
int micGain = 100;
int mode = 0;
int lastMode = -1;
bool enableSound = true;
int masterBrightness = 80;
int animSpeed = 50;
int themeId = 0;  // last theme 0-4

uint8_t solidR = 150, solidG = 0, solidB = 255;

float micEma = 0;
const float MIC_EMA_ALPHA = 0.35f;

#include <EEPROM.h>
#define EEPROM_SIZE 8
EEPROMClass MODE("M");
EEPROMClass SENSITIVITY("S");
EEPROMClass ENABLESOUND("E");
EEPROMClass BRIGHTNESS("B");
EEPROMClass SPEED("V");
EEPROMClass GATE("G");
EEPROMClass GAIN("A");
EEPROMClass COLR("CR");
EEPROMClass COLG("CG");
EEPROMClass COLB("CB");
EEPROMClass THEME("T");

void applyMasterBrightness() {
  uint8_t scaled = map(constrain(masterBrightness, 0, 100), 0, 100, 0, 255);
  spikes.setBrightness(scaled);
  spikes.show();
}

void saveSolidColor() {
  if (COLR.begin(EEPROM_SIZE)) { COLR.put(0, solidR); COLR.commit(); }
  if (COLG.begin(EEPROM_SIZE)) { COLG.put(0, solidG); COLG.commit(); }
  if (COLB.begin(EEPROM_SIZE)) { COLB.put(0, solidB); COLB.commit(); }
}

long readMicLevel() {
  long delta = (long)analogRead(MIC) - OFFSET;
  if (delta < 0) delta = 0;
  long level = (delta * micGain) / 100 + sensitivity;
  if (level < 0) level = 0;
  micEma = MIC_EMA_ALPHA * (float)level + (1.0f - MIC_EMA_ALPHA) * micEma;
  long smoothed = (long)(micEma + 0.5f);
  if (smoothed < 0) smoothed = 0;
  if ((unsigned long)smoothed > lastmiclevel) lastmiclevel = smoothed;
  return smoothed;
}

void setup() {
  pinMode(TRANSMITTER, OUTPUT);
  Ask_TX.init();
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(MIC, INPUT);

  spikes.begin();
  spikes.show();

  Serial.begin(115200);
  Serial.println("Drake Tail....GO! (C + T themes)");

  // STA to Head SoftAP (fixed channel 2) so ESP-NOW home channel matches peer
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);  // max practical TX for suit range
  WiFi.config(ip, gateway, subnet);
  // Third arg = channel: prefer Head SoftAP on ch 2 (see ESPNOW.md)
  WiFi.begin(ssid, NULL, 2);
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 5000) delay(100);
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi STA joined ");
    Serial.print(ssid);
    Serial.print(" ch=");
    Serial.print(WiFi.channel());
    Serial.print(" RSSI=");
    Serial.print(WiFi.RSSI());
    Serial.print(" BSSID=");
    const uint8_t *b = WiFi.BSSID();
    if (b) {
      char bb[24];
      sprintf(bb, "%02X:%02X:%02X:%02X:%02X:%02X", b[0], b[1], b[2], b[3], b[4], b[5]);
      Serial.println(bb);
    } else {
      Serial.println("?");
    }
  } else {
    Serial.println("WiFi STA: SoftAP not found — locking radio to ch2 for ESP-NOW");
    WiFi.disconnect(false, false);
    delay(50);
  }

  setupEspNow();
  // If SoftAP came up late, refresh peer from BSSID once more
  if (WiFi.status() == WL_CONNECTED) refreshHeadPeerFromSoftAP();

  Serial.println("Restoring settings..");
  if (MODE.begin(EEPROM_SIZE)) {
    MODE.get(0, mode);
    if (mode < 0 || mode > 10) { mode = 0; MODE.put(0, mode); MODE.commit(); }
  }
  if (SENSITIVITY.begin(EEPROM_SIZE)) {
    SENSITIVITY.get(0, sensitivity);
    if (sensitivity < -500 || sensitivity > 4000) sensitivity = 75;
  }
  if (ENABLESOUND.begin(EEPROM_SIZE)) {
    ENABLESOUND.get(0, enableSound);
    if (enableSound < 0 || enableSound > 1) enableSound = true;
  }
  if (BRIGHTNESS.begin(EEPROM_SIZE)) {
    BRIGHTNESS.get(0, masterBrightness);
    if (masterBrightness < 0 || masterBrightness > 100) masterBrightness = 80;
  }
  if (SPEED.begin(EEPROM_SIZE)) {
    SPEED.get(0, animSpeed);
    if (animSpeed < 0 || animSpeed > 100) animSpeed = 50;
  }
  if (GATE.begin(EEPROM_SIZE)) {
    GATE.get(0, micGate);
    if (micGate < 10 || micGate > 2000) micGate = 100;
  }
  if (GAIN.begin(EEPROM_SIZE)) {
    GAIN.get(0, micGain);
    if (micGain < 50 || micGain > 300) micGain = 100;
  }
  if (COLR.begin(EEPROM_SIZE)) COLR.get(0, solidR);
  if (COLG.begin(EEPROM_SIZE)) COLG.get(0, solidG);
  if (COLB.begin(EEPROM_SIZE)) COLB.get(0, solidB);
  if (THEME.begin(EEPROM_SIZE)) {
    THEME.get(0, themeId);
    if (themeId < 0 || themeId > 4) themeId = 0;
  }

  applyMasterBrightness();
  setupBLE();

  // ESP32 Arduino 3.x: esp_task_wdt_init takes a config struct
  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = (uint32_t)WDT_TIMEOUT * 1000U,
    .idle_core_mask = 0,
    .trigger_panic = true,
  };
  esp_task_wdt_reconfigure(&wdt_config);  // already init by core; reconfigure timeout
  enableLoopWDT();

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
  // Loop WDT (enableLoopWDT) is fed when loop returns; also reset mid-loop
  // so long suit-sync / ASK bursts don't edge the 30s timeout.
  esp_task_wdt_reset();
  t.update();
  if (!flashed) sound_detect();
  checkSerialBT();    // USB serial TUI / monitor (same cmds as BLE)
  pushLiveStatus();   // app STAT ~2 Hz while BLE connected
  pushSuitSync();     // Head/PAWB settings heartbeat ~30 s (+ once after boot)
  pushPhaseSync();    // Head anim phase ~25 Hz (rainbow/comet/breathe/wave)
  esp_task_wdt_reset();
}

void sound_detect() {
  // Modes 2–10 run continuously (VU needs live mic stream to Head every frame)
  if (mode >= 2 && mode <= 10) {
    mode_selector(mode);
    digitalWrite(LED_BUILTIN, LOW);
    return;
  }
  // Modes 0–1: sound-reactive when above gate; idle purple fade when quiet
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
  long micLevel = readMicLevel();
  if (enableSound && micLevel > micGate) {
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
  for (uint16_t i = 0; i < spikes.numPixels(); i++) spikes.setPixelColor(i, 0);
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
    case 0: soundloop(millis(), 50, false); break;
    case 1: soundloop(millis(), 50, true); break;
    case 2: soundcheck(); break;
    case 3: mode_rainbow_chase(); break;
    case 4: mode_comet(); break;
    case 5: mode_breathing(); break;
    case 6: mode_fire(); break;
    case 7: mode_sparkle(); break;
    case 8: mode_wave(); break;
    case 9: mode_solid(); break;
    case 10: mode_off(); break;
    default: mode = 0; break;
  }
}
