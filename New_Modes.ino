/*
 * New_Modes.ino
 * Non-blocking lighting modes for Drake 2.0 Tail (modes 3-10)
 *
 * All animations use millis() timing so WiFi / BLE / ASK stay responsive.
 */

// Shared state for the new modes
static unsigned long modePrevMillis = 0;
static uint16_t modeStep = 0;
static uint8_t solidR = 150, solidG = 0, solidB = 255;  // default purple-blue

// ---------- Mode 3: Rainbow Chase ----------
void mode_rainbow_chase() {
  const unsigned long interval = 40;  // ms per frame
  if (millis() - modePrevMillis < interval) return;
  modePrevMillis = millis();

  for (int i = 0; i < spikes.numPixels(); i++) {
    uint16_t pixelHue = modeStep + (i * 65536L / spikes.numPixels());
    spikes.setPixelColor(i, spikes.gamma32(spikes.ColorHSV(pixelHue)));
  }
  spikes.show();
  modeStep += 256;  // advance hue
}

// ---------- Mode 4: Comet / Meteor ----------
void mode_comet() {
  const unsigned long interval = 45;
  if (millis() - modePrevMillis < interval) return;
  modePrevMillis = millis();

  // Fade all pixels slightly
  for (int i = 0; i < spikes.numPixels(); i++) {
    uint32_t c = spikes.getPixelColor(i);
    uint8_t r = (c >> 16) & 0xFF;
    uint8_t g = (c >> 8) & 0xFF;
    uint8_t b = c & 0xFF;
    spikes.setPixelColor(i, r * 0.7, g * 0.7, b * 0.7);
  }

  // Bright head
  int head = modeStep % spikes.numPixels();
  spikes.setPixelColor(head, 255, 255, 255);
  // Short trail
  if (head > 0) spikes.setPixelColor(head - 1, 180, 100, 255);
  if (head > 1) spikes.setPixelColor(head - 2, 80, 0, 150);

  spikes.show();
  modeStep++;
}

// ---------- Mode 5: Breathing Pulse ----------
void mode_breathing() {
  const unsigned long interval = 30;
  if (millis() - modePrevMillis < interval) return;
  modePrevMillis = millis();

  // Simple triangle wave 0..255..0
  static int breath = 0;
  static int breathDir = 1;
  breath += breathDir * 3;
  if (breath >= 255) { breath = 255; breathDir = -1; }
  if (breath <= 20)  { breath = 20;  breathDir = 1; }

  // Slow hue drift
  uint16_t hue = modeStep;
  uint32_t color = spikes.gamma32(spikes.ColorHSV(hue, 255, breath));
  for (int i = 0; i < spikes.numPixels(); i++) {
    spikes.setPixelColor(i, color);
  }
  spikes.show();
  modeStep += 20;
}

// ---------- Mode 6: Fire Flicker ----------
void mode_fire() {
  const unsigned long interval = 40;
  if (millis() - modePrevMillis < interval) return;
  modePrevMillis = millis();

  for (int i = 0; i < spikes.numPixels(); i++) {
    // Hotter near the base (index 0), cooler toward the tip
    int heat = random(0, 255);
    // Bias lower pixels hotter
    heat = constrain(heat - (i * 12), 0, 255);

    // Map heat to fire colors (black -> red -> orange -> yellow)
    uint8_t r, g, b;
    if (heat < 85) {
      r = heat * 3;
      g = 0;
      b = 0;
    } else if (heat < 170) {
      r = 255;
      g = (heat - 85) * 3;
      b = 0;
    } else {
      r = 255;
      g = 255;
      b = (heat - 170) * 2;
    }
    spikes.setPixelColor(i, r, g, b);
  }
  spikes.show();
}

// ---------- Mode 7: Sparkle / Twinkle ----------
void mode_sparkle() {
  const unsigned long interval = 50;
  if (millis() - modePrevMillis < interval) return;
  modePrevMillis = millis();

  // Dim everything a little
  for (int i = 0; i < spikes.numPixels(); i++) {
    uint32_t c = spikes.getPixelColor(i);
    uint8_t r = ((c >> 16) & 0xFF) * 0.85;
    uint8_t g = ((c >> 8) & 0xFF) * 0.85;
    uint8_t b = (c & 0xFF) * 0.85;
    spikes.setPixelColor(i, r, g, b);
  }

  // Random new sparkles
  if (random(0, 100) < 40) {
    int pos = random(0, spikes.numPixels());
    // White or soft purple sparkle
    if (random(0, 2) == 0)
      spikes.setPixelColor(pos, 255, 255, 255);
    else
      spikes.setPixelColor(pos, 180, 80, 255);
  }
  spikes.show();
}

// ---------- Mode 8: Wave / Undulate ----------
void mode_wave() {
  const unsigned long interval = 35;
  if (millis() - modePrevMillis < interval) return;
  modePrevMillis = millis();

  for (int i = 0; i < spikes.numPixels(); i++) {
    // Traveling sine-like brightness
    float phase = (float)(modeStep + i * 30) / 40.0f;
    float wave = (sin(phase) + 1.0f) * 0.5f;  // 0..1
    uint8_t bri = (uint8_t)(wave * 220);
    // Soft purple-blue base
    spikes.setPixelColor(i, (bri * 150) / 255, 0, bri);
  }
  spikes.show();
  modeStep++;
}

// ---------- Mode 9: Solid / Static ----------
void mode_solid() {
  // Only update occasionally to save cycles
  if (millis() - modePrevMillis < 200) return;
  modePrevMillis = millis();

  for (int i = 0; i < spikes.numPixels(); i++) {
    spikes.setPixelColor(i, solidR, solidG, solidB);
  }
  spikes.show();
}

// Optional helper to change the solid color later via BLE command
void setSolidColor(uint8_t r, uint8_t g, uint8_t b) {
  solidR = r;
  solidG = g;
  solidB = b;
}

// ---------- Mode 10: Off / Blackout ----------
void mode_off() {
  if (millis() - modePrevMillis < 300) return;
  modePrevMillis = millis();

  for (int i = 0; i < spikes.numPixels(); i++) {
    spikes.setPixelColor(i, 0, 0, 0);
  }
  spikes.show();
}

// Call this when switching modes so animations restart cleanly
void resetModeState() {
  modePrevMillis = 0;
  modeStep = 0;
}
