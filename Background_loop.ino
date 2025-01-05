//Global varables to reset
bool _direction;
int brightness;

void fading()
{
  static byte resetdelay = 0; 
  static unsigned long previousMillis;

  
  if(millis() - previousMillis >= 50) //time expired
  {
    
    if (_direction)
      brightness--;
    else
      brightness++;

    if (brightness >= MAXBRIGHTNESS/*75*/)
      _direction = true;
    else if (brightness <=0)
    { 
      _direction = false;
      resetdelay++;
      if (resetdelay >= 10) //Delay about 10 cycles
      {
        sendbackgroundloopReset();
        resetdelay = 0; //reset
        //Serial.println("Sending Reset...");
      }
      
    }
     
  

    set_brightness(brightness);
    //Serial.println(brightness);
    

    previousMillis = millis();



    
  }
  
}

void set_brightness(byte __brightness)
{
  float _brightness = (float)__brightness/100.0;
  
  for(uint16_t i=0; i < spikes.numPixels(); i++)
      spikes.setPixelColor(i, spikes.Color(150 * _brightness , 0, 255 * _brightness ));
  spikes.show();

}

void sendbackgroundloopReset()
{
    //Send out the RESYNC command to both ASK and UDP WIFI (IF connected).
    const char *msg = "R0";
    Ask_TX.send((uint8_t *)msg, strlen(msg));
    Ask_TX.waitPacketSent();
    udp.broadcastTo(msg, 1234);

}

void resetBrightnessandDirection(){
  _direction = false;
  brightness = 0;

}
