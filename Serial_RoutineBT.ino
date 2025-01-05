

void checkSerialBT() {
  //routine to handle serial commands
  if (SerialBT.available() > 0) {

    byte inByte = SerialBT.read();
    SerialBT.println();

    switch (inByte) {
      case 'e':
        {
          SerialBT.println("Disable Sound Detection.");
          enableSound = false;
          ENABLESOUND.put(0, enableSound);
          ENABLESOUND.commit();
          break;
        }
      case 'E':
        {
          SerialBT.println("Enable Sound Detection.");
          enableSound = true;
          ENABLESOUND.put(0, enableSound);
          ENABLESOUND.commit();          
          break;
        }      
      case 'L':
        {
          SerialBT.println("Flash Lamp!");
          flash_lamp();
          const char *msg = "L0";
          udp.broadcastTo(msg, 1234);
          Ask_TX.send((uint8_t *)msg, strlen(msg));
          Ask_TX.waitPacketSent();
          break;
        }

      case 'S':
        {
          SerialBT.print("Sensitivity=");
          int temp = SerialBT.parseInt();
          if (temp > -1500 && temp < 4000)
            sensitivity = (int)temp;
          SerialBT.println(sensitivity);
          SENSITIVITY.put(0, sensitivity);
          SENSITIVITY.commit();
          break;
        }

      case 'R':
        {
          SerialBT.println("Resyc...");
          Serial.println("Resyc...");
          sendbackgroundloopReset();
          resetBrightnessandDirection();
          break;
        }

      case 'Z':
        {
          SerialBT.println("REBOOTING...");
          Serial.println("REBOOTING...");
          delay(1000);
          ESP.restart();
          break;
        }

      case 'M':
        {
          SerialBT.print("Mode=");
          int temp = SerialBT.parseInt();
          if (temp >= 0 && temp <= 10)
            mode = temp; 

          char msg[3] = {'M','0', '\0'};

          switch (mode){ //TODO do a byte conversion
            case 0:
            msg[1] = '0';
            break;
            case 1:
            msg[1] = '1';
            break;
            default:
            msg[1] = '0';
            break;
          }

          udp.broadcastTo(msg, 1234);
          Ask_TX.send((uint8_t *)msg, strlen(msg));
          Ask_TX.waitPacketSent();          

          SerialBT.println(mode);
          Serial.print("Mode=");
          Serial.println(mode);

          /*Save to NVS*/
          MODE.put(0, mode);
          MODE.commit();
          break;
        }

      case '?':
      default:
        {
          SerialBT.println("**************************************");
          SerialBT.println("Available Commands:");
          SerialBT.println("L-Flash LEDs");
          SerialBT.println("R-Resyc LEDS");
          SerialBT.println("E/e-Enable/Disable Sound detection.");
          SerialBT.println("M<mode>-Mode control [0-10]");
          SerialBT.println("Z - REBOOT");
          SerialBT.println("S<sensitivty>-Change sensitivty.");
          SerialBT.println("**************************************");
          SerialBT.print("M:"); SerialBT.println(mode);  
          SerialBT.print("S:"); SerialBT.println(sensitivity);   
          SerialBT.print("E:"); SerialBT.println(enableSound);     
          SerialBT.print("Last Mic:"); SerialBT.println(lastmiclevel);  
          SerialBT.print("Head Brightness:"); SerialBT.println(head_brightness);  
          SerialBT.print("Head Temperature:"); SerialBT.println(head_temperature);  
          SerialBT.println("**************************************");
          SerialBT.println("**************************************");           
          break;
        }
    }
  }
}
