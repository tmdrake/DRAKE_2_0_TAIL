/*
 * Serial_RoutineBT.ino  (NimBLE NUS)
 * Remote command interface over Nordic UART Service
 *
 * Also pushes live status to the app:
 *   STAT M:.. B:.. V:.. S:.. E:.. Mic:.. HeadB:.. HeadT:..
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

// Lightweight live status for the app (single line, easy to parse)
void pushLiveStatus() {
  if (!deviceConnected || !pTxCharacteristic) return;

  static unsigned long lastPush = 0;
  if (millis() - lastPush < 500) return;  // ~2 Hz
  lastPush = millis();

  String s = "STAT M:";
  s += mode;
  s += " B:";
  s += masterBrightness;
  s += " V:";
  s += animSpeed;
  s += " S:";
  s += sensitivity;
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

void processBLECommand(const String& raw) {
  String cmd = raw;
  cmd.trim();
  if (cmd.length() == 0) return;

  char inByte = cmd.charAt(0);

  switch (inByte) {
    case 'e':
      {
        blePrintln("Disable Sound Detection.");
        enableSound = false;
        ENABLESOUND.put(0, enableSound);
        ENABLESOUND.commit();
        break;
      }
    case 'E':
      {
        blePrintln("Enable Sound Detection.");
        enableSound = true;
        ENABLESOUND.put(0, enableSound);
        ENABLESOUND.commit();
        break;
      }
    case 'L':
      {
        blePrintln("Flash Lamp!");
        flash_lamp();
        const char *msg = "L0";
        udp.broadcastTo(msg, 1234);
        Ask_TX.send((uint8_t *)msg, strlen(msg));
        Ask_TX.waitPacketSent();
        break;
      }
    case 'S':
      {
        int temp = cmd.substring(1).toInt();
        if (temp > -1500 && temp < 4000) {
          sensitivity = temp;
          SENSITIVITY.put(0, sensitivity);
          SENSITIVITY.commit();
        }
        blePrintln("Sensitivity=" + String(sensitivity));
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
    case 'R':
      {
        blePrintln("Resync...");
        sendbackgroundloopReset();
        resetBrightnessandDirection();
        break;
      }
    case 'Z':
      {
        blePrintln("REBOOTING...");
        delay(500);
        ESP.restart();
        break;
      }
    case 'M':
      {
        int temp = cmd.substring(1).toInt();
        if (temp >= 0 && temp <= 10)
          mode = temp;

        char msg[3] = {'M', '0', '\0'};
        if (mode >= 0 && mode <= 9)
          msg[1] = '0' + mode;
        else if (mode == 10)
          msg[1] = 'A';

        udp.broadcastTo(msg, 1234);
        Ask_TX.send((uint8_t *)msg, strlen(msg));
        Ask_TX.waitPacketSent();

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
        status += "Commands:\n";
        status += "L - Flash\n";
        status += "R - Resync\n";
        status += "E/e - Sound on/off\n";
        status += "M<0-10> - Mode\n";
        status += "B<0-100> - Brightness\n";
        status += "V<0-100> - Speed\n";
        status += "S<value> - Sensitivity\n";
        status += "Z - REBOOT\n";
        status += "**************************************\n";
        status += "M:" + String(mode) + "  B:" + String(masterBrightness) +
                  "  V:" + String(animSpeed) + "\n";
        status += "S:" + String(sensitivity) + "  E:" + String(enableSound) + "\n";
        status += "Mic:" + String(lastmiclevel) +
                  "  HeadB:" + String(head_brightness) +
                  "  HeadT:" + String(head_temperature) + "\n";
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
    Serial.println("BLE client disconnected - restarting advertising");
    NimBLEDevice::startAdvertising();
  }
};

class RxCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0) {
      processBLECommand(String(value.c_str()));
    }
  }
};

void setupBLE() {
  Serial.println("Starting NimBLE NUS...");

  NimBLEDevice::init("TMDrake_tail");
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);

  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  NimBLEService* pService = pServer->createService(NUS_SERVICE_UUID);

  NimBLECharacteristic* pRxCharacteristic = pService->createCharacteristic(
      NUS_RX_UUID,
      NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
  );
  pRxCharacteristic->setCallbacks(new RxCallbacks());

  pTxCharacteristic = pService->createCharacteristic(
      NUS_TX_UUID,
      NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ
  );

  pService->start();

  NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->setName("TMDrake_tail");
  pAdvertising->addServiceUUID(NUS_SERVICE_UUID);
  pAdvertising->enableScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMaxPreferred(0x12);
  pAdvertising->start();

  Serial.println("NimBLE NUS advertising as TMDrake_tail");
}

void checkSerialBT() {
  // No-op
}
