/*
 * Mode 2 – VU meter (Tail local display + stream mic to Head/PAWB).
 * Must call sampleaudio() so espnowSendMic / ASK "m####" keep Head in sync.
 */
void soundcheck() {
  // sampleaudio(): readMicLevel + ESP-NOW MIC + ASK to paws
  long analogbuffer = sampleaudio();
  if (analogbuffer < 0) analogbuffer = 0;

  // Peak hold with slow decay so the scale doesn't stick at max forever
  if ((unsigned long)analogbuffer > lastmiclevel) {
    lastmiclevel = analogbuffer;
  } else {
    static unsigned long lastDecay = 0;
    if (millis() - lastDecay >= 40) {
      lastDecay = millis();
      if (lastmiclevel > 20)
        lastmiclevel = (lastmiclevel * 94) / 100;
    }
  }
  if (lastmiclevel < 20) lastmiclevel = 20;

#ifdef DEBUG_MIC
  Serial.print("VU:");
  Serial.println(analogbuffer);
#endif

  int pct = map(analogbuffer, 0, (long)lastmiclevel, 0, 100);
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  vugraph(pct);
}

void vugraph(int percentage) {
  int numberleds = map(percentage, 0, 100, 0, spikes.numPixels());

  for (int i = 0; i < spikes.numPixels(); i++) {
    if (i < numberleds)
      spikes.setPixelColor(i, spikes.Color(150, 0, 255));
    else
      spikes.setPixelColor(i, spikes.Color(0, 0, 0));
  }
  spikes.show();
}
