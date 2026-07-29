/*
 * Serial_RoutineBT.ino – NimBLE NUS + Head settings + mic G/A
 */

#include <NimBLEDevice.h>

#define NUS_SERVICE_UUID      "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define NUS_RX_UUID           "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define NUS_TX_UUID           "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

NimBLECharacteristic* pTxCharacteristic = nullptr;
NimBLEServer* pServer = nullptr;
bool deviceConnected = false;

void blePrint(const String& msg) {
  if (pTxCharacteristic && deviceConnected) {
    pTxCharacteristic->setValue(msg.c_str());
    pTxCharacteristic->notify();
  }
  Serial.print("[BLE] ");
  Serial.println(msg);
}

void blePrintln(const String& msg) {
  blePrint(msg + "\n");
}

void pushLiveStatus() {
  if (!deviceConnected || !pTxCharacteristic) return;

  static unsigned long lastPush = 0;
  if (millis() - lastPush < 500) return;
  lastPush = millis();

  String s = "STAT M:";
  s += mode;
  s += " B:";
  s += masterBrightness;
  s += " V:";
  s += animSpeed;
  s += " S:";
  s += sensitivity;
  s += " G:";
  s += micGate;
  s += " A:";
  s += micGain;
  s += " E:";
  s += enableSound ? 1 : 0;
  s += " Mic:";
  s += lastmiclevel;
  s += " HeadB:";
  s += head_brightness;
  s += " HeadT:";
  s += head_temperature;

  pTxCharacteristic->setValue(s.c_str());
  pTxCharacteristic->notify();
}

void forwardCmd(const char *msg) {
  espnowSendCmd(msg);
  udp.broadcastTo(msg, 1234);
  Ask_TX.send((uint8_t *)msg, strlen(msg));
  Ask_TX.waitPacketSent();
}

void forwardHeadCmd(const char *msg) {
  espnowSendCmd(msg);
  udp.broadcastTo(msg, 1234);
}

void processBLECommand(const String& raw) {
  String cmd = raw;
  cmd.trim();
  if (cmd.length() == 0) return;

  char inByte = cmd.charAt(0);

  switch (inByte) {
    case 'e':
      enableSound = false;
      ENABLESOUND.put(0, enableSound);
      ENABLESOUND.commit();
      blePrintln("Sound Detection OFF");
      break;
    case 'E':
      enableSound = true;
      ENABLESOUND.put(0, enableSound);
      ENABLESOUND.commit();
      blePrintln("Sound Detection ON");
      break;
    case 'L':
      blePrintln("Flash Lamp!");
      flash_lamp();
      forwardCmd("L0");
      break;
    case 'S':
      {
        int temp = cmd.substring(1).toInt();
        if (temp >= -500 && temp <= 4000) {
          sensitivity = temp;
          SENSITIVITY.put(0, sensitivity);
          SENSITIVITY.commit();
        }
        blePrintln("Sensitivity=" + String(sensitivity));
        break;
      }
    case 'G':
      {
        int temp = cmd.substring(1).toInt();
        if (temp >= 10 && temp <= 2000) {
          micGate = temp;
          GATE.put(0, micGate);
          GATE.commit();
        }
        blePrintln("Gate=" + String(micGate));
        break;
      }
    case 'A':
      {
        int temp = cmd.substring(1).toInt();
        if (temp >= 50 && temp <= 300) {
          micGain = temp;
          GAIN.put(0, micGain);
          GAIN.commit();
        }
        blePrintln("Gain=" + String(micGain));
        break;
      }
    case 'B':
      {
        int temp = cmd.substring(1).toInt();
        if (temp >= 0 && temp <= 100) {
          masterBrightness = temp;
          applyMasterBrightness();
          BRIGHTNESS.put(0, masterBrightness);
          BRIGHTNESS.commit();
        }
        blePrintln("Brightness=" + String(masterBrightness));
        break;
      }
    case 'V':
      {
        int temp = cmd.substring(1).toInt();
        if (temp >= 0 && temp <= 100) {
          animSpeed = temp;
          SPEED.put(0, animSpeed);
          SPEED.commit();
        }
        blePrintln("Speed=" + String(animSpeed));
        break;
      }
    case 'F':
      forwardHeadCmd(cmd.c_str());
      blePrintln("Fan cmd: " + cmd);
      break;
    case 'I':
      forwardHeadCmd(cmd.c_str());
      blePrintln("CDS threshold: " + cmd);
      break;
    case 'D':
      forwardHeadCmd(cmd.c_str());
      blePrintln("Eye dim: " + cmd);
      break;
    case 'R':
      blePrintln("Resync...");
      sendbackgroundloopReset();
      resetBrightnessandDirection();
      forwardCmd("R0");
      break;
    case 'Z':
      blePrintln("REBOOTING...");
      delay(500);
      ESP.restart();
      break;
    case 'M':
      {
        int temp = cmd.substring(1).toInt();
        if (temp >= 0 && temp <= 10) mode = temp;

        char msg[3] = {'M', '0', '\0'};
        if (mode >= 0 && mode <= 9) msg[1] = '0' + mode;
        else if (mode == 10) msg[1] = 'A';

        forwardCmd(msg);
        blePrintln("Mode=" + String(mode));
        MODE.put(0, mode);
        MODE.commit();
        break;
      }
    case '?':
    default:
      {
        String status;
        status += "**************************************\n";
        status += "M B V S G A E/e L R Z\n";
        status += "S=sensitivity  G=gate  A=gain%\n";
        status += "F0/F1/F2 FT I D (Head)\n";
        status += "**************************************\n";
        status += "M:" + String(mode) + " B:" + String(masterBrightness) +
                  " V:" + String(animSpeed) + "\n";
        status += "S:" + String(sensitivity) + " G:" + String(micGate) +
                  " A:" + String(micGain) + " E:" + String(enableSound) + "\n";
        status += "Mic:" + String(lastmiclevel) +
                  " HeadB:" + String(head_brightness) +
                  " HeadT:" + String(head_temperature) + "\n";
        status += "**************************************\n";
        blePrint(status);
        break;
      }
  }
}

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer) {
    deviceConnected = true;
    Serial.println("BLE client connected");
  }
  void onDisconnect(NimBLEServer* pServer) {
    deviceConnected = false;
    Serial.println("BLE client disconnected");
    NimBLEDevice::startAdvertising();
  }
};

class RxCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0)
      processBLECommand(String(value.c_str()));
  }
};

void setupBLE() {
  NimBLEDevice::init("TMDrake_tail");
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);
  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());
  NimBLEService* pService = pServer->createService(NUS_SERVICE_UUID);
  NimBLECharacteristic* pRx = pService->createCharacteristic(
      NUS_RX_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  pRx->setCallbacks(new RxCallbacks());
  pTxCharacteristic = pService->createCharacteristic(
      NUS_TX_UUID, NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ);
  pService->start();
  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->setName("TMDrake_tail");
  adv->addServiceUUID(NUS_SERVICE_UUID);
  adv->enableScanResponse(true);
  adv->start();
  Serial.println("NimBLE NUS advertising as TMDrake_tail");
}

void checkSerialBT() {}
