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
// Adafruit electret amp (AGC/normalizing): DC bias ~1.25 V on 0–3.3 V rail.
// 12-bit ADC: mid = 1.25/3.3 * 4095 ≈ 1551 (was 1600 manual → clipped half-wave).
#define MIC_VCC_MV        3300
#define MIC_BIAS_MV       1250
#define OFFSET            ((4095L * MIC_BIAS_MV) / MIC_VCC_MV)  // ~1551
// Fixed-rate envelope — VU / ESP-NOW fast. ASK mic = pulse-only for M0/M1/M2 paws.
#define MIC_SAMPLE_US     1000   // 1 kHz ADC ticks (non-blocking)
#define MIC_STREAM_MS     5      // envelope + ESP-NOW + Tail VU (~200 Hz)
#define MIC_ASK_PULSE_MS  40     // max ASK "m####" rate (sound modes only; no waitPacketSent)
// ADC counts below this after |raw−mid| are treated as silence (noise floor).
#define MIC_DEADBAND      12
// Adaptive scale: never divide by less than this (keeps soft sounds off full-scale).
#define MIC_SCALE_MIN     60
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

// S = post-envelope gain % (was a DC offset that forced a false “always-on” floor).
int sensitivity = 100;
int micGate = 40;
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
// Latest envelope after fixed-rate sampling (use this everywhere; never busy-loop ADC).
long micLevelCached = 0;
// Quiet baseline (AGC/room noise) — VU/wheel use (level − floor), not raw/peak.
float micNoiseFloor = 0.0f;
// Adaptive peak of *excess* above floor (shared M0/M1/M2).
long micScalePeak = 80;
static unsigned long micLastSampleUs = 0;
static unsigned long micLastStreamMs = 0;
static unsigned long micLastAskPulseMs = 0;
static long micPeakInWindow = 0;
static long lastSentMic = -9999;
static long lastAskExcess = -1;

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

/** Always begin → put → commit (ESP32 NVS is flaky if namespace not open). */
static void nvsPutInt(EEPROMClass &ns, int val) {
  if (ns.begin(EEPROM_SIZE)) {
    ns.put(0, val);
    ns.commit();
  }
}
static void nvsPutBool(EEPROMClass &ns, bool val) {
  if (ns.begin(EEPROM_SIZE)) {
    ns.put(0, val);
    ns.commit();
  }
}

/*
 * One ADC sample: full-wave |raw − 1.25 V mid| on 0–3.3 V / 12-bit.
 * True silence → 0 (no additive DC offset). S/A are % gains only.
 * Call only from micService() on MIC_SAMPLE_US ticks.
 */
static long micInstantAbs() {
  long raw = (long)analogRead(MIC);
  long delta = raw - (long)OFFSET;
  if (delta < 0) delta = -delta;
  if (delta < MIC_DEADBAND) return 0;
  // level = |Δ| × (gain%/100) × (sensitivity%/100)
  long level = (delta * (long)micGain * (long)sensitivity) / 10000L;
  if (level < 0) level = 0;
  return level;
}

/**
 * Non-blocking mic service — call every loop().
 *   1) Every MIC_SAMPLE_US (500 Hz): sample ADC, keep peak in window
 *   2) Every MIC_STREAM_MS (50 Hz): EMA that peak → micLevelCached, optional stream
 * Returns current cached level (always safe to read).
 */
long micService(bool streamOut) {
  unsigned long nowUs = micros();
  if ((long)(nowUs - micLastSampleUs) >= (long)MIC_SAMPLE_US) {
    // Catch up at most one missed slot so a long loop doesn't free-run ADC
    micLastSampleUs = nowUs;
    long inst = micInstantAbs();
    if (inst > micPeakInWindow) micPeakInWindow = inst;
  }

  unsigned long nowMs = millis();
  if (nowMs - micLastStreamMs >= (unsigned long)MIC_STREAM_MS) {
    micLastStreamMs = nowMs;
    long peak = micPeakInWindow;
    micPeakInWindow = 0;

    // Fast attack / slower release on the envelope (readable VU, still snappy)
    float p = (float)peak;
    if (p > micEma)
      micEma = 0.55f * p + 0.45f * micEma;
    else
      micEma = 0.20f * p + 0.80f * micEma;

    long smoothed = (long)(micEma + 0.5f);
    if (smoothed < 0) smoothed = 0;
    if (smoothed < 2) {
      smoothed = 0;
      micEma = 0;
    }
    micLevelCached = smoothed;
    lastmiclevel = (unsigned long)smoothed;  // STAT / live = raw envelope (for gate tuning)

    // Noise floor: follows quiet baseline (your ~250–270). Falls quick, rises slow.
    // Without this, adaptive peak ≈ floor and VU sits at 100% when "silent".
    float sf = (float)smoothed;
    if (micNoiseFloor < 1.0f)
      micNoiseFloor = sf;  // first sample
    else if (sf < micNoiseFloor)
      micNoiseFloor = 0.20f * sf + 0.80f * micNoiseFloor;
    else
      micNoiseFloor = 0.0015f * sf + 0.9985f * micNoiseFloor;

    // Excess above floor (+ small margin) — this is what lights the bar
    long floorI = (long)(micNoiseFloor + 0.5f);
    long margin = 12 + floorI / 20;  // a bit of headroom so hiss doesn't light bar
    long excess = smoothed - floorI - margin;
    if (excess < 0) excess = 0;

    // Peak tracks *excess* only (loudest recent hit above quiet)
    if (excess > micScalePeak)
      micScalePeak = excess;
    else if (micScalePeak > MIC_SCALE_MIN)
      micScalePeak = (micScalePeak * 99) / 100;
    if (micScalePeak < MIC_SCALE_MIN) micScalePeak = MIC_SCALE_MIN;

    if (streamOut) {
      // ESP-NOW: full rate excess → Head VU / Phase / Pulse
      if (abs(excess - lastSentMic) >= 2 || excess == 0) {
        lastSentMic = excess;
        int16_t level16 = (int16_t)constrain(excess, 0, 32767);
        espnowSendMic(level16);
      }

      // ASK → PAWB: pulse-only on M0/M1/M2 (not continuous stream; no waitPacketSent)
      // Modes 3–10 only need M#/C/R0 via forwardCmd — no mic ASK.
      if (mode >= 0 && mode <= 2) {
        bool hit = (excess >= 20);  // real energy above floor
        bool changed = (abs(excess - lastAskExcess) >= 25);
        bool due = (nowMs - micLastAskPulseMs >= (unsigned long)MIC_ASK_PULSE_MS);
        // Send on loud hit+change, or a quiet zero once so paws release
        if (due && ((hit && changed) || (excess == 0 && lastAskExcess > 0))) {
          micLastAskPulseMs = nowMs;
          lastAskExcess = excess;
          char msg[16];
          ltoa(excess, msg, 10);
          size_t n = strlen(msg);
          memmove(msg + 1, msg, n + 1);
          msg[0] = 'm';
          Ask_TX.send((uint8_t *)msg, n + 1);  // queue only — do not waitPacketSent
        }
      }
    }

#ifdef DEBUG_MIC
    Serial.print("Mic raw=");
    Serial.print(smoothed);
    Serial.print(" flr=");
    Serial.print((int)micNoiseFloor);
    Serial.print(" xs=");
    Serial.print(excess);
    Serial.print(" pk=");
    Serial.print(micScalePeak);
    Serial.print(" n=");
    Serial.println(micNorm01(), 2);
#endif
  }
  return micLevelCached;
}

/** Gate / display: last fixed-rate envelope (never a one-shot ADC read). */
long readMicLevel() {
  return micService(false);
}

/** Excess above noise floor (same units as streamed to Head). */
long micExcess() {
  long floorI = (long)(micNoiseFloor + 0.5f);
  long margin = 12 + floorI / 20;
  long excess = micLevelCached - floorI - margin;
  if (excess < 0) excess = 0;
  return excess;
}

/**
 * Shared intensity 0..1 for VU + Sound Phase/Pulse.
 * Quiet (at noise floor) → ~0; loud peaks → ~1. Not raw/peak (that stayed full).
 */
float micNorm01() {
  long peak = micScalePeak;
  if (peak < MIC_SCALE_MIN) peak = MIC_SCALE_MIN;
  long xs = micExcess();
  float n = (float)xs / (float)peak;
  if (n > 1.0f) n = 1.0f;
  if (n < 0.0f) n = 0.0f;
  // Small values stay off — bar should look empty until real hits
  if (n < 0.04f) n = 0.0f;
  return n;
}

/** 0..100 for VU bars / percent maps. */
int micNormPct() {
  return (int)(micNorm01() * 100.0f + 0.5f);
}

void setup() {
  pinMode(TRANSMITTER, OUTPUT);
  Ask_TX.init();
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(MIC, INPUT);
  // 0–3.3 V full scale (Adafruit mic bias 1.25 V, peaks toward 0 / 3.3)
  analogSetPinAttenuation(MIC, ADC_11db);
  analogReadResolution(12);

  spikes.begin();
  spikes.show();

  Serial.begin(115200);
  Serial.println("Drake Tail....GO! (C + T themes)");
  Serial.print("Mic: bias 1.25V OFFSET=");
  Serial.print((int)OFFSET);
  Serial.print(" sample=");
  Serial.print(1000000L / MIC_SAMPLE_US);
  Serial.print("Hz stream=");
  Serial.print(1000 / MIC_STREAM_MS);
  Serial.println("Hz (full-wave)");

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
    if (mode < 0 || mode > 10) { mode = 0; nvsPutInt(MODE, mode); }
  }
  if (SENSITIVITY.begin(EEPROM_SIZE)) {
    SENSITIVITY.get(0, sensitivity);
    // S is gain % now (10–400). Old “offset” values like 75 still work as 75% gain.
    if (sensitivity < 10 || sensitivity > 400) sensitivity = 100;
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
    if (micGate < 5 || micGate > 2000) micGate = 40;
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

  Serial.print("NVS: M=");
  Serial.print(mode);
  Serial.print(" B=");
  Serial.print(masterBrightness);
  Serial.print(" V=");
  Serial.print(animSpeed);
  Serial.print(" S=");
  Serial.print(sensitivity);
  Serial.print(" G=");
  Serial.print(micGate);
  Serial.print(" A=");
  Serial.print(micGain);
  Serial.print(" E=");
  Serial.print(enableSound ? 1 : 0);
  Serial.print(" T=");
  Serial.println(themeId);

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
  // Always tick mic at fixed 500 Hz (envelope for ≤200 Hz after LPF)
  bool streamMic = (mode >= 0 && mode <= 2) || soundmode;
  micService(streamMic);
  t.update();
  if (!flashed) sound_detect();
  checkSerialBT();    // USB serial TUI / monitor (same cmds as BLE)
  pushLiveStatus();   // app STAT ~2 Hz while BLE connected
  pushSuitSync();     // Head/PAWB settings heartbeat ~30 s (+ once after boot)
  pushPhaseSync();    // Head anim phase ~25 Hz (rainbow/comet/breathe/wave)
  esp_task_wdt_reset();
}

void sound_detect() {
  // Modes 2–10 run continuously (VU needs live mic stream to Head)
  if (mode >= 2 && mode <= 10) {
    mode_selector(mode);
    digitalWrite(LED_BUILTIN, LOW);
    return;
  }
  // Modes 0–1: color wheel ONLY while sound hits gate; else idle purple keep-alive
  if (soundmode && enableSound) {
    mode_selector(mode);
    digitalWrite(LED_BUILTIN, HIGH);
    // Drop back to idle soon after quiet (original feel: mostly off until sound)
    if (millis() - lastime > 2500) {
      soundmode = false;
      resetBrightnessandDirection();
      resetSoundloopState();
      sendbackgroundloopReset();
    }
  } else {
    fading();  // original power-save keep-alive + R0 resync (see Background_loop.ino)
    digitalWrite(LED_BUILTIN, LOW);
    lastmiclevel = 0;
  }
  long micLevel = micLevelCached;  // from fixed-rate micService in loop()
  if (enableSound && micLevel > micGate) {
    soundmode = true;
    lastime = millis();  // hold while audio above gate
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
    // V: scroll period ~40..8 ms (original was ~50 ms fixed; keep it snappy)
    case 0: soundloop(millis(), map(constrain(animSpeed, 0, 100), 0, 100, 40, 8), false); break;  // Sound Phase
    case 1: soundloop(millis(), map(constrain(animSpeed, 0, 100), 0, 100, 40, 8), true); break;   // Sound Pulse
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
