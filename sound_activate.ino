#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void prepend(char* s, const char* t);

// Current wheel color (0..1) for Sound Phase / Sound Pulse inject
float cR = 1.0f, cG = 0.0f, cB = 0.0f;

static unsigned long soundloop_previousMillis = 0;
static int soundloop_k = 0;
static int soundloop_col = 0;
static uint16_t soundloop_hue = 0;
static bool soundloop_inited = false;

void resetSoundloopState() {
  soundloop_previousMillis = 0;
  soundloop_k = 0;
  soundloop_col = 0;
  soundloop_hue = 0;
  soundloop_inited = false;
  cR = 1.0f;
  cG = 0.0f;
  cB = 0.0f;
}

static void hueToRgb(uint16_t h, float *r, float *g, float *b) {
  uint8_t sextant = h / 10923;
  uint16_t rem = h % 10923;
  float t = rem / 10923.0f;
  switch (sextant % 6) {
    case 0: *r = 1;   *g = t;   *b = 0;   break;
    case 1: *r = 1-t; *g = 1;   *b = 0;   break;
    case 2: *r = 0;   *g = 1;   *b = t;   break;
    case 3: *r = 0;   *g = 1-t; *b = 1;   break;
    case 4: *r = t;   *g = 0;   *b = 1;   break;
    default:*r = 1;   *g = 0;   *b = 1-t; break;
  }
}

/*
 * M0 Sound Phase / M1 Sound Pulse — gated elsewhere (soundmode).
 *
 * Original feel: mostly idle purple until sound hits, then fast flood sampling.
 * Mic envelope is already ~200 Hz; this only advances the strip (fast shift).
 *
 *   color=false (M0): continuous HSV wheel
 *   color=true  (M1): stepped 6-color every strip length
 */
void soundloop(unsigned long nowMs, long refresh_ms, bool color) {
  // Allow snappy floods (VU is 5 ms; wheel can be 8–40 ms)
  if (refresh_ms < 8) refresh_ms = 8;
  if (refresh_ms > 80) refresh_ms = 80;

  if (nowMs - soundloop_previousMillis < (unsigned long)refresh_ms) return;
  soundloop_previousMillis = nowMs;

  if (!soundloop_inited) {
    if (color)
      cycleRgb(0);
    else
      hueToRgb(soundloop_hue, &cR, &cG, &cB);
    soundloop_inited = true;
  }

  // Same scale as VU (M2): excess above noise floor → 0..1 (quiet ≈ off)
  float bright = micNorm01();

  const uint16_t n = spikes.numPixels();

  // Fast memory flood: shift only (no heavy per-pixel fade — keeps rate up)
  for (uint16_t i = n - 1; i > 0; i--) {
    spikes.setPixelColor(i, spikes.getPixelColor(i - 1));
  }

  if (bright < 0.04f)
    spikes.setPixelColor(0, 0);  // quiet inject → trail empties as it scrolls
  else
    setRgb(bright);  // wheel color × VU-matched intensity

  spikes.show();

  // Phase: spin hue every step (faster when loud)
  if (!color) {
    uint16_t step = 400 + (uint16_t)(bright * 1400.0f);
    soundloop_hue = (uint16_t)(soundloop_hue + step);
    hueToRgb(soundloop_hue, &cR, &cG, &cB);
  }

  soundloop_k++;
  if (soundloop_k >= (int)n) {
    soundloop_col = ++soundloop_col % 6;
    if (color)
      cycleRgb(soundloop_col);
    soundloop_k = 0;
  }
}

long sampleaudio() {
  return micLevelCached;
}

void prepend(char* s, const char* t) {
  size_t len = strlen(t);
  memmove(s + len, s, strlen(s) + 1);
  memcpy(s, t, len);
}

void cycleRgb(int col) {
  switch (col) {
    case 0: cR = 1;   cG = 0;   cB = 0;   break;
    case 1: cR = 1;   cG = 0.55f; cB = 0; break;
    case 2: cR = 0;   cG = 1;   cB = 0;   break;
    case 3: cR = 0;   cG = 0.7f; cB = 1;  break;
    case 4: cR = 0.15f; cG = 0; cB = 1;   break;
    case 5: cR = 0.85f; cG = 0; cB = 0.7f; break;
    default: cR = 1;  cG = 0;   cB = 0;   break;
  }
}

void setRgb(float val) {
  if (val < 0.0f) val = 0.0f;
  if (val > 1.0f) val = 1.0f;
  spikes.setPixelColor(0,
    (int)(val * cR * 255.0f + 0.5f),
    (int)(val * cG * 255.0f + 0.5f),
    (int)(val * cB * 255.0f + 0.5f));
}

void fadeRgb() {
  soundloop_hue = (uint16_t)(soundloop_hue + 800);
  hueToRgb(soundloop_hue, &cR, &cG, &cB);
}
