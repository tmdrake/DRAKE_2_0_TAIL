#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void prepend(char* s, const char* t);

float cR = 0, cG = 0, cB = 0;

// Last value we actually transmitted (for change-threshold)
static long lastSentMic = -9999;

void soundloop(unsigned long millis, long refresh_ms, bool color) {
  static unsigned long soundloop_previousMillis = 0;
  static int k = 0;
  static int col = 0;

  if (millis - soundloop_previousMillis >= refresh_ms) {
    soundloop_previousMillis = millis;

    long squareLevel = sampleaudio();

    for (uint16_t i = spikes.numPixels(); i > 0; i--) {
      spikes.setPixelColor(i, spikes.getPixelColor(i - 1));
    }

    if (squareLevel / 1.0f > 0.5) {
      spikes.setPixelColor(0, spikes.Color(0, 0, 0));
    }

    setRgb(squareLevel / 1.0f);
    spikes.show();

    if (!color) {
      fadeRgb();
    }

    k++;
  }

  if (k >= spikes.numPixels()) {
    col = ++col % 6;
    if (color) {
      cycleRgb(col);
    }
    k = 0;
  }
}

/*
 * Analog mic path (unchanged):
 *   raw = analogRead(MIC)          // ESP32 ADC
 *   level = raw - OFFSET + sensitivity
 *   OFFSET = 1600  (approx mid-rail / bias voltage of the mic preamp)
 *
 * Transport to Head: encrypted ESP-NOW (preferred)
 * Transport to paws: ASK ASCII "m####" (unchanged)
 */
long sampleaudio() {
  long micLevel = analogRead(MIC) - OFFSET + sensitivity;
  if (micLevel < 0) micLevel = 0;

  // ---- ESP-NOW mic stream to Head (low latency, encrypted) ----
  if (abs(micLevel - lastSentMic) >= 8 || micLevel == 0) {
    lastSentMic = micLevel;
    int16_t level16 = (int16_t)constrain(micLevel, 0, 32767);
    espnowSendMic(level16);

    // Optional UDP fallback while bringing ESP-NOW online on the bench:
    // uint8_t buf[2] = { (uint8_t)((level16 >> 8) & 0xFF), (uint8_t)(level16 & 0xFF) };
    // udp.writeTo(buf, 2, IPAddress(192, 168, 4, 1), 1237);
  }

  // ---- ASK to paws (ASCII "m####") ----
  char msg[16];
  ltoa(micLevel, msg, 10);
  prepend(msg, "m");
  Ask_TX.send((uint8_t *)msg, strlen(msg));

#ifdef DEBUG_MIC
  Serial.println("Mic:");
  Serial.println(micLevel);
#endif
  if (lastmiclevel < (unsigned long)micLevel) {
    lastmiclevel = micLevel;
  }

  return micLevel;
}

void prepend(char* s, const char* t) {
  size_t len = strlen(t);
  memmove(s + len, s, strlen(s) + 1);
  memcpy(s, t, len);
}

void cycleRgb(int col) {
  switch (col) {
    case 0: cR = 1;   cG = 0;   cB = 0;   break;
    case 1: cR = 0.5; cG = 0.5; cB = 0;   break;
    case 2: cR = 0;   cG = 1;   cB = 0;   break;
    case 3: cR = 0;   cG = 0.5; cB = 0.5; break;
    case 4: cR = 0;   cG = 0;   cB = 1;   break;
    case 5: cR = 0.5; cG = 0;   cB = 0.5; break;
  }
}

void setRgb(float val) {
  if (val > 1.0f) val = 1.0f;
  spikes.setPixelColor(0, (int)(val * cR * 255), (int)(val * cG * 255), (int)(val * cB * 255));
}

void fadeRgb() {
  const int phaseLength = 100;
  const int period = phaseLength * 5;
  static int iteration = 0;
  int phase = iteration / phaseLength;
  int step = iteration % phaseLength;

  switch (phase) {
    case 0: cR = 1; cG = step / (float)phaseLength; cB = 0; break;
    case 1: cR = (phaseLength - step) / (float)phaseLength; cG = 1; cB = 0; break;
    case 2: cR = 0; cG = 1; cB = step / (float)phaseLength; break;
    case 3: cR = 0; cG = (phaseLength - step) / (float)phaseLength; cB = 1; break;
    case 4: cR = step / (float)phaseLength; cG = 0; cB = 1; break;
    case 5: cR = 1; cG = 0; cB = (phaseLength - step) / (float)phaseLength; break;
  }
  iteration = ++iteration % period;
}
