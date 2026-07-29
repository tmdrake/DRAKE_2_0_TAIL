/*
 * Serial_RoutineBT.ino  (now BLE / NimBLE NUS)
 * Remote command interface over Nordic UART Service
 *
 * Service UUID : 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
 * RX  (write)  : 6E400002-B5A3-F393-E0A9-E50E24DCCA9E
 * TX  (notify) : 6E400003-B5A3-F393-E0A9-E50E24DCCA9E
 *
 * Device name  : TMDrake_tail
 *
 * Requires: NimBLE-Arduino library
 *   Arduino Library Manager -> "NimBLE-Arduino" by h2zero
 */

#include <NimBLEDevice.h>

// Nordic UART Service UUIDs
#define NUS_SERVICE_UUID      "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define NUS_RX_UUID           "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"  // Phone writes here
#define NUS_TX_UUID           "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"  // We notify here

NimBLECharacteristic* pTxCharacteristic = nullptr;
NimBLEServer* pServer = nullptr;
bool deviceConnected = false;

// Helper: send a string back to the connected phone via TX notifications
void blePrint(const String& msg) {
  if (pTxCharacteristic && deviceConnected) {
    pTxCharacteristic->setValue(msg.c_str());
    pTxCharacteristic->notify();
  }
  // Also mirror to USB serial for debugging
  Serial.print("[BLE] ");
  Serial.println(msg);
}

void blePrintln(const String& msg) {
  blePrint(msg + "\n");
}

// ---------- Command parser (same behaviour as old SerialBT version) ----------
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
    case 'R':
      {
        blePrintln("Resync...");
        Serial.println("Resync...");
        sendbackgroundloopReset();
        resetBrightnessandDirection();
        break;
      }
    case 'Z':
      {
        blePrintln("REBOOTING...");
        Serial.println("REBOOTING...");
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

        udp.broadcastTo(msg, 1234);
        Ask_TX.send((uint8_t *)msg, strlen(msg));
        Ask_TX.waitPacketSent();

        blePrintln("Mode=" + String(mode));
        Serial.print("Mode=");
        Serial.println(mode);

        MODE.put(0, mode);
        MODE.commit();
        break;
      }
    case '?':
    default:
      {
        String status;
        status += "**************************************\n";
        status += "Available Commands:\n";
        status += "L-Flash LEDs\n";
        status += "R-Resync LEDs\n";
        status += "E/e-Enable/Disable Sound detection\n";
        status += "M<mode>-Mode control [0-10]\n";
        status += "S<value>-Change sensitivity\n";
        status += "Z-REBOOT\n";
        status += "**************************************\n";
        status += "M:" + String(mode) + "\n";
        status += "S:" + String(sensitivity) + "\n";
        status += "E:" + String(enableSound) + "\n";
        status += "Last Mic:" + String(lastmiclevel) + "\n";
        status += "Head Brightness:" + String(head_brightness) + "\n";
        status += "Head Temperature:" + String(head_temperature) + "\n";
        status += "**************************************\n";
        blePrint(status);
        break;
      }
  }
}

// ---------- NimBLE Callbacks ----------
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
      String cmd = String(value.c_str());
      processBLECommand(cmd);
    }
  }
};

// ---------- Setup BLE (call once from setup()) ----------
void setupBLE() {
  Serial.println("Starting NimBLE NUS...");

  NimBLEDevice::init("TMDrake_tail");
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);   // Stronger TX if needed

  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  NimBLEService* pService = pServer->createService(NUS_SERVICE_UUID);

  // RX Characteristic - phone writes commands here
  NimBLECharacteristic* pRxCharacteristic = pService->createCharacteristic(
      NUS_RX_UUID,
      NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
  );
  pRxCharacteristic->setCallbacks(new RxCallbacks());

  // TX Characteristic - we send responses / status here
  pTxCharacteristic = pService->createCharacteristic(
      NUS_TX_UUID,
      NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ
  );

  pService->start();

  // Advertising - put NUS UUID in scan response for reliable discovery
  NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->setName("TMDrake_tail");
  pAdvertising->addServiceUUID(NUS_SERVICE_UUID);
  pAdvertising->enableScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMaxPreferred(0x12);
  pAdvertising->start();

  Serial.println("NimBLE NUS advertising as TMDrake_tail");
  Serial.println("Service UUID: " NUS_SERVICE_UUID);
}

// Compatibility stub so the old call site still compiles
// (commands are now handled inside the onWrite callback)
void checkSerialBT() {
  // No-op - BLE is event-driven via callbacks
}
