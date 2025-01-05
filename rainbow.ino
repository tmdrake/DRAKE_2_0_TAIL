
// Rainbow cycle along whole spikes. Pass delay time (in ms) between frames.
void rainbow(int wait) {
  // Hue of first pixel runs 5 complete loops through the color wheel.
  // Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to 5*65536. Adding 256 to firstPixelHue each time
  // means we'll make 5*65536/256 = 1280 passes through this outer loop:
  for (long firstPixelHue = 0; firstPixelHue < 5 * 65536; firstPixelHue += 256) {
    for (int i = 0; i < spikes.numPixels(); i++) {  // For each pixel in spikes...
      // Offset pixel hue by an amount to make one full revolution of the
      // color wheel (range of 65536) along the length of the spikes
      // (spikes.numPixels() steps):
      static int pixelHue = firstPixelHue + (i * 65536L / spikes.numPixels());
      // spikes.ColorHSV() can take 1 or 3 arguments: a hue (0 to 65535) or
      // optionally add saturation and value (brightness) (each 0 to 255).
      // Here we're using just the single-argument hue variant. The result
      // is passed through spikes.gamma32() to provide 'truer' colors
      // before assigning to each pixel:
      spikes.setPixelColor(i, spikes.gamma32(spikes.ColorHSV(pixelHue)));
    }
    spikes.show();  // Update spikes with new contents
    delay(wait);    // Pause for a moment
  }
}