
void soundcheck() {
  long analogbuffer = analogRead(MIC) - OFFSET + sensitivity;

  //char msg[10];
  //ltoa(analogbuffer, msg, 10);
  //udp.broadcastTo(msg, 1237);
  //        Ask_TX.send((uint8_t *)msg, strlen(msg));
  //        Ask_TX.waitPacketSent();

  if (analogbuffer < 0)
    analogbuffer = 0;

#ifdef DEBUG_MIC
  Serial.println("Mic:");
  Serial.println(analogbuffer);
#endif
  if (lastmiclevel < analogbuffer) {
    lastmiclevel = analogbuffer;  //update a globable varable
  }


  vugraph(map(analogbuffer, 0, lastmiclevel, 0, 100));  //using the highest to for the upper end
}

void vugraph(int percentage) {

  int numberleds = map(percentage, 25, 100, 0, spikes.numPixels());

  for (int i = 0; i < spikes.numPixels(); i++) {
    if (i <= numberleds)
      spikes.setPixelColor(i, spikes.Color(150, 0, 255));
    else
      spikes.setPixelColor(i, spikes.Color(0, 0, 0));
  }

  spikes.show();
}
