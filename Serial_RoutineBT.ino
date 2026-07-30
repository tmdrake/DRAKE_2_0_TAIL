/*
 * Serial_RoutineBT.ino – NimBLE NUS
 * Includes C (RGB) + T (themes) for app color UI
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

void blePrintln(const String& msg) { blePrint(msg + "\n"); }

void pushLiveStatus() {
  if (!deviceConnected || !pTxCharacteristic) return;
  static unsigned long lastPush = 0;
  if (millis() - lastPush < 500) return;
  lastPush = millis();

  String s = "STAT M:";
  s += mode;
  s += " B:"; s += masterBrightness;
  s += " V:"; s += animSpeed;
  s += " S:"; s += sensitivity;
  s += " G:"; s += micGate;
  s += " A:"; s += micGain;
  s += " E:"; s += enableSound ? 1 : 0;
  s += " C:"; s += solidR; s += ","; s += solidG; s += ","; s += solidB;
  s += " T:"; s += themeId;
  s += " Mic:"; s += lastmiclevel;
  s += " HeadB:"; s += head_brightness;
  s += " HeadT:"; s += head_temperature;

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

// Apply RGB, mode 9, persist, fan-out to Head/PAWB
void applySolidAndBroadcast(uint8_t r, uint8_t g, uint8_t b) {
  setSolidColor(r, g, b);
  saveSolidColor();
  mode = 9;
  MODE.put(0, mode);
  MODE.commit();
  char msg[20];
  snprintf(msg, sizeof(msg), "C%d,%d,%d", r, g, b);
  forwardCmd(msg);
  forwardCmd("M9");
}

// Themes: numeric T0-T4 or named Tpurple/Tfire/Tice/Tgold/Temerald
bool applyTheme(const String& arg) {
  String a = arg;
  a.toLowerCase();
  uint8_t r = solidR, g = solidG, b = solidB;
  int id = -1;

  if (a == "0" || a == "purple")      { r = 157; g = 78;  b = 221; id = 0; }
  else if (a == "1" || a == "fire")   { r = 255; g = 60;  b = 0;   id = 1; }
  else if (a == "2" || a == "ice")    { r = 80;  g = 180; b = 255; id = 2; }
  else if (a == "3" || a == "gold")   { r = 255; g = 180; b = 40;  id = 3; }
  else if (a == "4" || a == "emerald"){ r = 20;  g = 200; b = 100; id = 4; }
  else return false;

  themeId = id;
  if (THEME.begin(8)) { THEME.put(0, themeId); THEME.commit(); }
  applySolidAndBroadcast(r, g, b);
  return true;
}

void processBLECommand(const String& raw) {
  String cmd = raw;
  cmd.trim();
  if (cmd.length() == 0) return;
  char inByte = cmd.charAt(0);

  switch (inByte) {
    case 'e':
      enableSound = false;
      ENABLESOUND.put(0, enableSound); ENABLESOUND.commit();
      blePrintln("Sound Detection OFF");
      break;
    case 'E':
      enableSound = true;
      ENABLESOUND.put(0, enableSound); ENABLESOUND.commit();
      blePrintln("Sound Detection ON");
      break;
    case 'L':
      blePrintln("Flash Lamp!");
      flash_lamp();
      forwardCmd("L0");
      break;
    case 'S': {
      int temp = cmd.substring(1).toInt();
      if (temp >= -500 && temp <= 4000) {
        sensitivity = temp;
        SENSITIVITY.put(0, sensitivity); SENSITIVITY.commit();
      }
      blePrintln("Sensitivity=" + String(sensitivity));
      break;
    }
    case 'G': {
      int temp = cmd.substring(1).toInt();
      if (temp >= 10 && temp <= 2000) {
        micGate = temp;
        GATE.put(0, micGate); GATE.commit();
      }
      blePrintln("Gate=" + String(micGate));
      break;
    }
    case 'A': {
      int temp = cmd.substring(1).toInt();
      if (temp >= 50 && temp <= 300) {
        micGain = temp;
        GAIN.put(0, micGain); GAIN.commit();
      }
      blePrintln("Gain=" + String(micGain));
      break;
    }
    case 'B': {
      int temp = cmd.substring(1).toInt();
      if (temp >= 0 && temp <= 100) {
        masterBrightness = temp;
        applyMasterBrightness();
        BRIGHTNESS.put(0, masterBrightness); BRIGHTNESS.commit();
      }
      blePrintln("Brightness=" + String(masterBrightness));
      break;
    }
    case 'V': {
      int temp = cmd.substring(1).toInt();
      if (temp >= 0 && temp <= 100) {
        animSpeed = temp;
        SPEED.put(0, animSpeed); SPEED.commit();
      }
      blePrintln("Speed=" + String(animSpeed));
      break;
    }
    case 'C': {
      int r = 0, g = 0, b = 0;
      if (sscanf(cmd.c_str() + 1, "%d,%d,%d", &r, &g, &b) == 3) {
        r = constrain(r, 0, 255);
        g = constrain(g, 0, 255);
        b = constrain(b, 0, 255);
        themeId = -1; // custom
        applySolidAndBroadcast((uint8_t)r, (uint8_t)g, (uint8_t)b);
        blePrintln("Color=" + String(r) + "," + String(g) + "," + String(b) + " Mode=9");
      } else {
        blePrintln("Color format: C<r>,<g>,<b>");
      }
      break;
    }
    case 'T': {
      String arg = cmd.substring(1);
      arg.trim();
      if (applyTheme(arg)) {
        blePrintln("Theme=" + arg + " Mode=9");
      } else {
        blePrintln("Themes: T0/Tpurple T1/Tfire T2/Tice T3/Tgold T4/Temerald");
      }
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
    case 'M': {
      int temp = cmd.substring(1).toInt();
      if (temp >= 0 && temp <= 10) mode = temp;
      char msg[3] = {'M', '0', '\0'};
      if (mode >= 0 && mode <= 9) msg[1] = '0' + mode;
      else if (mode == 10) msg[1] = 'A';
      forwardCmd(msg);
      blePrintln("Mode=" + String(mode));
      MODE.put(0, mode); MODE.commit();
      break;
    }
    case '?':
    default: {
      String status;
      status += "M0-10 B V S G A E/e C r,g,b T0-4 L R Z\n";
      status += "F0/F1/F2 FT I D (Head)\n";
      status += "M:" + String(mode) + " B:" + String(masterBrightness) +
                " V:" + String(animSpeed) + "\n";
      status += "C:" + String(solidR) + "," + String(solidG) + "," + String(solidB) +
                " T:" + String(themeId) + "\n";
      status += "Mic:" + String(lastmiclevel) +
                " HeadB:" + String(head_brightness) +
                " HeadT:" + String(head_temperature) + "\n";
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
    NimBLEDevice::startAdvertising();
  }
};

class RxCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0) processBLECommand(String(value.c_str()));
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
