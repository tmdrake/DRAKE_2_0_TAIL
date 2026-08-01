/*
 * Mode 2 – VU meter (Tail local display).
 *
 * Uses shared micNorm01() (true-zero envelope + adaptive peak).
 * Ballistics: fast attack / medium release on the bar, slower peak-hold marker.
 */
void soundcheck() {
  static unsigned long lastVuMs = 0;
  static float bar = 0.0f;   // main level 0..1
  static float hold = 0.0f;  // peak marker 0..1

  if (millis() - lastVuMs < (unsigned long)MIC_STREAM_MS) return;
  lastVuMs = millis();

  float n = micNorm01();

  // Ballistics @ ~200 Hz — instant attack, short release (keep it readable)
  if (n > bar)
    bar = n;
  else
    bar = bar * 0.70f + n * 0.30f;

  if (n > hold)
    hold = n;
  else {
    hold -= 0.012f;                   // peak tick fall ~0.4 s from full
    if (hold < bar) hold = bar;
  }
  if (hold < 0.0f) hold = 0.0f;

  int pct = (int)(bar * 100.0f + 0.5f);
  int peakPct = (int)(hold * 100.0f + 0.5f);
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  if (peakPct < 0) peakPct = 0;
  if (peakPct > 100) peakPct = 100;

#ifdef DEBUG_MIC
  Serial.print("VU bar=");
  Serial.print(pct);
  Serial.print(" hold=");
  Serial.print(peakPct);
  Serial.print(" lvl=");
  Serial.print(micLevelCached);
  Serial.print(" peak=");
  Serial.println(micScalePeak);
#endif

  vugraph(pct, peakPct);
}

/** Fill LEDs 0..N by level; peak marker as a brighter tip pixel. */
void vugraph(int percentage, int peakPercentage) {
  const int nPix = (int)spikes.numPixels();
  int lit = map(percentage, 0, 100, 0, nPix);
  if (lit < 0) lit = 0;
  if (lit > nPix) lit = nPix;

  int peakIdx = map(peakPercentage, 0, 100, 0, nPix - 1);
  if (peakIdx < 0) peakIdx = 0;
  if (peakIdx > nPix - 1) peakIdx = nPix - 1;

  for (int i = 0; i < nPix; i++) {
    if (i < lit) {
      // Gradient: purple base → pink tip of the bar
      uint8_t t = (lit <= 1) ? 0 : (uint8_t)((i * 255) / (lit - 1));
      spikes.setPixelColor(i, spikes.Color(150 + (t / 4), 0, 255 - (t / 3)));
    } else {
      spikes.setPixelColor(i, 0);
    }
  }
  // Peak hold tick (white-ish) so you can see crest above the bar
  if (peakPercentage >= 3 && peakIdx >= lit - 1) {
    spikes.setPixelColor(peakIdx, spikes.Color(255, 200, 255));
  }
  spikes.show();
}
