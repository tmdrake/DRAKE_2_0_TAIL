#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void prepend(char* s, const char* t);

//#include <esp_task_wdt.h>
/*1-5-2025: Lots of this code was stolen from Shngy's suit project back in 2016 era*/
// void soundloop() {
//   /*Some example procedures showing how to display to the pixels:*/
//   static int col = 0;
//   for (int k = 0; k < spikes.numPixels(); k++) {

//     for (uint16_t i = spikes.numPixels(); i > 0; i--) {
//       spikes.setPixelColor(i, spikes.getPixelColor(i - 1));
//     }



//     long micLevel = analogRead(MIC) - OFFSET + sensitivity;  //adafruit offset
//     if (micLevel < 0)
//       micLevel = 0;

// #ifdef DEBUG_MIC
//     Serial.println("Mic:");
//     Serial.println(micLevel);
// #endif
//     if (lastmiclevel < micLevel) {
//       lastmiclevel = micLevel;
//     }


//     //                        int log4Level = log(micLevel) / log(4) * 12.5;
//     int squareLevel = micLevel * micLevel * micLevel / ((long)4 * 1024 * 1024);
//     //                        float sCurveLevel = (int)(sCurve(micLevel, 4) * 255);
//     //                        float fMicLevel = micLevel / 4.0f / 200.0f;
//     //                        float quadraticLevel = pow(fMicLevel, 2);

//     //Serial.println(squareLevel);
//     if (squareLevel / 1.0f > 0.5) {
//       spikes.setPixelColor(0, spikes.Color(0, 0, 0));  //reset 0 to all
//     }

//     //this ratio needs to be fixed
//     setRgb(squareLevel / 1.0f);

//     spikes.show();  //push the color out to all

//     delay(5);  //***this is needed but will multiply out, needs a asyn clock

//     fadeRgb();  //Makes the color more warmer (optional)
//   }             ///end of the first loop

//   //delay(500); //Not needed anymore 12-31-2024

//   col = ++col % 6;  // col = col modulus 6, Plus 1
//   cycleRgb(col);    // makes them blinky

// }
/*************************The actual code****************************/
float cR = 0, cG = 0, cB = 0;  // Need to use descriptive varables....
void soundloop(unsigned long millis, long refresh_ms, bool color) {
  /*sample and sound was done at <intervals in mS>*/ 
  static unsigned long soundloop_previousMillis = 0;  //persistant varable
  static int k = 0;
  static int col = 0;

  //esp_task_wdt_reset();
  //delay(1); // <------------------------------ Mandatory

  if (millis - soundloop_previousMillis >= refresh_ms/*ms*/) {
    soundloop_previousMillis = millis;  //reset counter

    //get our sound audio
    long squareLevel = sampleaudio();
    
    //Shift everything down 1 spike, leave spike 0 with the last sound
    for (uint16_t i = spikes.numPixels(); i > 0; i--) {
      spikes.setPixelColor(i, spikes.getPixelColor(i - 1));
    }

    if (squareLevel / 1.0f > 0.5) {
      spikes.setPixelColor(0, spikes.Color(0, 0, 0));  //reset 0 to all
    }

    //this ratio needs to be fixed
    setRgb(squareLevel / 1.0f);

    spikes.show();  //push the color out to all

    //delay(5);  //***this is needed but will multiply out, needs a asyn clock

    if (!color){
      fadeRgb();  //Makes the color more warmer (optional)
    }
    

    k++;
  }

  if (k >= spikes.numPixels()) {
    col = ++col % 6;  // col = col modulus 6, Plus 1
    if (color){
      cycleRgb(col);    //More Distinct RGB mode
    }
    
    k = 0;            //reset
  } 
}


long sampleaudio() {
  long micLevel = analogRead(MIC) - OFFSET + sensitivity;  //adafruit offset

  if (micLevel < 0)
    micLevel = 0;

  /*Send Data to head*/
  char msg[16];
  ltoa(micLevel, msg, 10);
  udp.broadcastTo(msg, 1237); //Send Audio Data Over UDP-IP 
  /*need to add a 'm' in front*/
  prepend(msg, "m");
  /*ASK may not support buffering while sending*/
  Ask_TX.send((uint8_t *)msg, strlen(msg));
  //Ask_TX.waitPacketSent(); //May not be neeed if we can check if ASK is busy.
  //Serial.println(msg);
  //Serial.println(strlen(msg)); //Massive debugging

  
  

#ifdef DEBUG_MIC
  Serial.println("Mic:");
  Serial.println(micLevel);
#endif
  if (lastmiclevel < micLevel) {
    lastmiclevel = micLevel;  //update a globable varable
  }


  //                        int log4Level = log(micLevel) / log(4) * 12.5;
  long squareLevel = micLevel * micLevel * micLevel / ((long)4 * 1024 * 1024);
  //                        float sCurveLevel = (int)(sCurve(micLevel, 4) * 255);
  //                        float fMicLevel = micLevel / 4.0f / 200.0f;
  //                        float quadraticLevel = pow(fMicLevel, 2);
  return (micLevel);
}



/* Prepends t into s. Assumes s has enough space allocated
** for the combined string.
*/
void prepend(char* s, const char* t)
{
    size_t len = strlen(t);
    memmove(s + len, s, strlen(s) + 1);
    memcpy(s, t, len);
}


/*Cycle col based on the rotational values in COL*/
void cycleRgb(int col) {
  switch (col) {
    case 0:
      cR = 1;
      cG = 0;
      cB = 0;
      break;

    case 1:
      cR = 0.5;
      cG = 0.5;
      cB = 0;
      break;

    case 2:
      cR = 0;
      cG = 1;
      cB = 0;
      break;

    case 3:
      cR = 0;
      cG = 0.5;
      cB = 0.5;
      break;

    case 4:
      cR = 0;
      cG = 0;
      cB = 1;
      break;

    case 5:
      cR = 0.5;
      cG = 0;
      cB = 0.5;
      break;
  }
}

/*Set output for the first neopixel*/
void setRgb(float val) {
  //Drakes; Drops the first pixel on each set
  if (val > 1.0f) val = 1.0f;
  //        rgb[0].r = (int)(val * cR * 255);
  //        rgb[0].g = (int)(val * cG * 255);
  //        rgb[0].b = (int)(val * cB * 255);
  //        back.setPixelColor(0, back.Color((int)(val * cR * 255), (int)(val * cG * 255), (int)(val * cB * 255)));
  spikes.setPixelColor(0, (int)(val * cR * 255), (int)(val * cG * 255), (int)(val * cB * 255));
}

/*Sets the phase of the cR, cB, cG values & Phase*/
void fadeRgb() {
  const int phaseLength = 100;         // thinking that it dose the color lenght or the linth of the power coming in
  const int period = phaseLength * 5;  //the changes depend pon the delays...probabbly should have it set into a value
  static int iteration = 0;
  // Determine which phase we are in
  int phase = iteration / phaseLength;
  int step = iteration % phaseLength;

  switch (phase) {
    case 0:  // Red on, green increasing, blue off
      cR = 1;
      cG = step / ((float)phaseLength);
      cB = 0;
      break;

    case 1:  // Red decreasing, green on, blue off
      cR = (phaseLength - step) / ((float)phaseLength);
      cG = 1;
      cB = 0;
      break;

    case 2:  // Red off, green on, blue increasing
      cR = 0;
      cG = 1;
      cB = step / ((float)phaseLength);
      break;

    case 3:  // Red off, green decreasing, blue on
      cR = 0;
      cG = (phaseLength - step) / ((float)phaseLength);
      cB = 1;
      break;

    case 4:  // Red increasing, green off, blue on
      cR = step / ((float)phaseLength);
      cG = 0;
      cB = 1;
      break;

    case 5:  // Red on, green off, blue decreasing
      cR = 1;
      cG = 0;
      cB = (phaseLength - step) / ((float)phaseLength);
      break;
  }

  iteration = ++iteration % period;
}
