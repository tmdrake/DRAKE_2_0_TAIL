/*
 * New_Modes.ino – Tail modes 3-10 (non-blocking)
 * solidR/G/B are globals (shared with BLE color/theme commands).
 */

static unsigned long modePrevMillis = 0;
static uint16_t modeStep = 0;

// Defined in main so STAT / NVS / BLE can see them
extern uint8_t solidR, solidG, solidB;

unsigned long scaledInterval(unsigned long baseMs) {
  float factor = 0.3f + (animSpeed / 100.0f) * 1.7f;
  unsigned long result = (unsigned long)(baseMs / factor);
  if (result < 8) result = 8;
  return result;
}

void setSolidColor(uint8_t r, uint8_t g, uint8_t b) {
  solidR = r;
  solidG = g;
  solidB = b;
}

void mode_rainbow_chase() {
  if (millis() - modePrevMillis < scaledInterval(40)) return;
  modePrevMillis = millis();
  for (int i = 0; i < spikes.numPixels(); i++) {
    uint16_t pixelHue = modeStep + (i * 65536L / spikes.numPixels());
    spikes.setPixelColor(i, spikes.gamma32(spikes.ColorHSV(pixelHue)));
  }
  spikes.show();
  modeStep += 256;
}

void mode_comet() {
  if (millis() - modePrevMillis < scaledInterval(45)) return;
  modePrevMillis = millis();
  for (int i = 0; i < spikes.numPixels(); i++) {
    uint32_t c = spikes.getPixelColor(i);
    spikes.setPixelColor(i, ((c >> 16) & 0xFF) * 0.7, ((c >> 8) & 0xFF) * 0.7, (c & 0xFF) * 0.7);
  }
  int head = modeStep % spikes.numPixels();
  spikes.setPixelColor(head, 255, 255, 255);
  if (head > 0) spikes.setPixelColor(head - 1, 180, 100, 255);
  if (head > 1) spikes.setPixelColor(head - 2, 80, 0, 150);
  spikes.show();
  modeStep++;
}

void mode_breathing() {
  if (millis() - modePrevMillis < scaledInterval(30)) return;
  modePrevMillis = millis();
  static int breath = 0;
  static int breathDir = 1;
  breath += breathDir * 3;
  if (breath >= 255) { breath = 255; breathDir = -1; }
  if (breath <= 20)  { breath = 20;  breathDir = 1; }
  uint32_t color = spikes.gamma32(spikes.ColorHSV(modeStep, 255, breath));
  for (int i = 0; i < spikes.numPixels(); i++) spikes.setPixelColor(i, color);
  spikes.show();
  modeStep += 20;
}

void mode_fire() {
  if (millis() - modePrevMillis < scaledInterval(40)) return;
  modePrevMillis = millis();
  for (int i = 0; i < spikes.numPixels(); i++) {
    int heat = constrain(random(0, 255) - (i * 12), 0, 255);
    uint8_t r, g, b;
    if (heat < 85) { r = heat * 3; g = 0; b = 0; }
    else if (heat < 170) { r = 255; g = (heat - 85) * 3; b = 0; }
    else { r = 255; g = 255; b = (heat - 170) * 2; }
    spikes.setPixelColor(i, r, g, b);
  }
  spikes.show();
}

void mode_sparkle() {
  if (millis() - modePrevMillis < scaledInterval(50)) return;
  modePrevMillis = millis();
  for (int i = 0; i < spikes.numPixels(); i++) {
    uint32_t c = spikes.getPixelColor(i);
    spikes.setPixelColor(i, ((c >> 16) & 0xFF) * 0.85, ((c >> 8) & 0xFF) * 0.85, (c & 0xFF) * 0.85);
  }
  if (random(0, 100) < 40) {
    int pos = random(0, spikes.numPixels());
    spikes.setPixelColor(pos, random(0, 2) ? 0xFFFFFF : spikes.Color(180, 80, 255));
  }
  spikes.show();
}

void mode_wave() {
  if (millis() - modePrevMillis < scaledInterval(35)) return;
  modePrevMillis = millis();
  for (int i = 0; i < spikes.numPixels(); i++) {
    float phase = (float)(modeStep + i * 30) / 40.0f;
    float wave = (sin(phase) + 1.0f) * 0.5f;
    uint8_t bri = (uint8_t)(wave * 220);
    spikes.setPixelColor(i, (bri * 150) / 255, 0, bri);
  }
  spikes.show();
  modeStep++;
}

void mode_solid() {
  if (millis() - modePrevMillis < 200) return;
  modePrevMillis = millis();
  for (int i = 0; i < spikes.numPixels(); i++)
    spikes.setPixelColor(i, solidR, solidG, solidB);
  spikes.show();
}

void mode_off() {
  if (millis() - modePrevMillis < 300) return;
  modePrevMillis = millis();
  for (int i = 0; i < spikes.numPixels(); i++) spikes.setPixelColor(i, 0);
  spikes.show();
}

void resetModeState() {
  modePrevMillis = 0;
  modeStep = 0;
}
