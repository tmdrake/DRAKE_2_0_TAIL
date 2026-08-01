/*
 * Serial_RoutineBT.ino – NimBLE NUS
 * C/T color + HB heartbeat for app data sync
 * Suit settings heartbeat: re-push mode/color/head settings every ~30 s
 */

#include <NimBLEDevice.h>

#define NUS_SERVICE_UUID      "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define NUS_RX_UUID           "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define NUS_TX_UUID           "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

/** How often Tail re-broadcasts current suit settings to Head (+ PAWB mode). */
#define SUIT_SYNC_INTERVAL_MS   30000UL
/** First sync shortly after boot so Head/PAWB catch NVS mode after power-on. */
#define SUIT_SYNC_BOOT_DELAY_MS  2000UL

NimBLECharacteristic* pTxCharacteristic = nullptr;
NimBLEServer* pServer = nullptr;
bool deviceConnected = false;

unsigned long lastAppHbMs = 0;   // last HB from app
uint32_t hbSeq = 0;              // increments each HB reply

// Shadow of last Head-only settings (app may change these via Tail)
int headFanMode = 2;             // 0 off, 1 on, 2 auto
float headFanThreshF = 85.0f;
int headCdsThresh = 500;
int headEyeDimPct = 10;

void blePrint(const String& msg) {
  if (pTxCharacteristic && deviceConnected) {
    pTxCharacteristic->setValue(msg.c_str());
    pTxCharacteristic->notify();
  }
  Serial.print("[BLE] ");
  Serial.println(msg);
}

void blePrintln(const String& msg) { blePrint(msg + "\n"); }

/* Shared STAT builder — used by periodic push and HB sync */
String buildStatLine() {
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
  s += " U:"; s += (millis() / 1000UL);   // uptime seconds
  s += " Seq:"; s += hbSeq;
  return s;
}

void pushLiveStatus() {
  if (!deviceConnected || !pTxCharacteristic) return;
  static unsigned long lastPush = 0;
  if (millis() - lastPush < 500) return;
  lastPush = millis();

  String s = buildStatLine();
  pTxCharacteristic->setValue(s.c_str());
  pTxCharacteristic->notify();
}

/* Immediate full sync for app heartbeat */
void replyHeartbeat() {
  lastAppHbMs = millis();
  hbSeq++;
  // Compact ACK then full snapshot (app can key off either)
  blePrintln("HBACK Seq:" + String(hbSeq) + " U:" + String(millis() / 1000UL));
  blePrint(buildStatLine());
}

/** Head SoftAP is always 192.168.4.1 — unicast is more reliable than 255.255.255.255 */
static void udpToHead(const char *msg) {
  if (!msg) return;
  size_t n = strlen(msg);
  // Primary: unicast to Head SoftAP
  udp.writeTo((const uint8_t *)msg, n, IPAddress(192, 168, 4, 1), 1234);
  // Also broadcast on the SoftAP subnet
  udp.writeTo((const uint8_t *)msg, n, IPAddress(192, 168, 4, 255), 1234);
}

void forwardCmd(const char *msg) {
  espnowSendCmd(msg);
  udpToHead(msg);
  Ask_TX.send((uint8_t *)msg, strlen(msg));
  Ask_TX.waitPacketSent();
}

void forwardHeadCmd(const char *msg) {
  espnowSendCmd(msg);
  udpToHead(msg);
}

/* Remember F/FT/I/D so the 30 s suit sync can re-apply them */
void rememberHeadCmd(const char *cmd) {
  if (!cmd || !cmd[0]) return;
  if (cmd[0] == 'F') {
    if (cmd[1] == 'T' || cmd[1] == 't') {
      float tf = atof(cmd + 2);
      if (tf >= 50.0f && tf <= 120.0f) headFanThreshF = tf;
    } else {
      int m = atoi(cmd + 1);
      if (m >= 0 && m <= 2) headFanMode = m;
    }
  } else if (cmd[0] == 'I') {
    int v = atoi(cmd + 1);
    if (v >= 0 && v <= 1023) headCdsThresh = v;
  } else if (cmd[0] == 'D') {
    int v = atoi(cmd + 1);
    if (v >= 1 && v <= 100) headEyeDimPct = v;
  }
}

/**
 * Re-push current settings to Head (ESP-NOW + UDP) and mode to PAWB (ASK).
 * Safe if Head rebooted or missed a packet — does not force solid unless mode is 9.
 */
void syncSuitSettings() {
  char mmsg[4];
  if (mode >= 0 && mode <= 9) {
    mmsg[0] = 'M';
    mmsg[1] = (char)('0' + mode);
    mmsg[2] = '\0';
  } else {
    mmsg[0] = 'M';
    mmsg[1] = 'A';  // mode 10
    mmsg[2] = '\0';
  }

  // Head treats C as solid + mode 9 — only send color when we are in solid
  if (mode == 9) {
    char cmsg[24];
    snprintf(cmsg, sizeof(cmsg), "C%d,%d,%d", solidR, solidG, solidB);
    forwardCmd(cmsg);
  }
  forwardCmd(mmsg);

  char h[20];
  snprintf(h, sizeof(h), "F%d", headFanMode);
  forwardHeadCmd(h);
  snprintf(h, sizeof(h), "FT%.0f", headFanThreshF);
  forwardHeadCmd(h);
  snprintf(h, sizeof(h), "I%d", headCdsThresh);
  forwardHeadCmd(h);
  snprintf(h, sizeof(h), "D%d", headEyeDimPct);
  forwardHeadCmd(h);

  Serial.print("Suit sync → ");
  Serial.print(mmsg);
  Serial.print(" F");
  Serial.print(headFanMode);
  Serial.print(" FT");
  Serial.println(headFanThreshF, 0);
}

/** Periodic / boot suit settings heartbeat (call from loop). */
void pushSuitSync() {
  static unsigned long lastSyncMs = 0;
  static bool didBootSync = false;
  unsigned long now = millis();

  if (!didBootSync) {
    if (now < SUIT_SYNC_BOOT_DELAY_MS) return;
    didBootSync = true;
    lastSyncMs = now;
    syncSuitSettings();
    return;
  }

  if (now - lastSyncMs < SUIT_SYNC_INTERVAL_MS) return;
  lastSyncMs = now;
  syncSuitSettings();
}

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

bool applyTheme(const String& arg) {
  String a = arg;
  a.toLowerCase();
  uint8_t r = solidR, g = solidG, b = solidB;
  int id = -1;

  if (a == "0" || a == "purple")       { r = 157; g = 78;  b = 221; id = 0; }
  else if (a == "1" || a == "fire")    { r = 255; g = 60;  b = 0;   id = 1; }
  else if (a == "2" || a == "ice")     { r = 80;  g = 180; b = 255; id = 2; }
  else if (a == "3" || a == "gold")    { r = 255; g = 180; b = 40;  id = 3; }
  else if (a == "4" || a == "emerald") { r = 20;  g = 200; b = 100; id = 4; }
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

  // Multi-char commands first
  String upper = cmd;
  upper.toUpperCase();
  if (upper == "HB" || upper.startsWith("HB ") || upper.startsWith("HB:")) {
    replyHeartbeat();
    return;
  }

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
        themeId = -1;
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
      if (applyTheme(arg))
        blePrintln("Theme=" + arg + " Mode=9");
      else
        blePrintln("Themes: T0/Tpurple T1/Tfire T2/Tice T3/Tgold T4/Temerald");
      break;
    }
    case 'F':
      rememberHeadCmd(cmd.c_str());
      forwardHeadCmd(cmd.c_str());
      blePrintln("Fan cmd: " + cmd);
      break;
    case 'I':
      rememberHeadCmd(cmd.c_str());
      forwardHeadCmd(cmd.c_str());
      blePrintln("CDS threshold: " + cmd);
      break;
    case 'D':
      rememberHeadCmd(cmd.c_str());
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
      status += "Cmds: M B V S G A E/e C T HB L R Z\n";
      status += "F0/F1/F2 FT I D (Head)\n";
      status += buildStatLine() + "\n";
      blePrint(status);
      break;
    }
  }
}

// NimBLE-Arduino 2.x: callbacks take NimBLEConnInfo (old 1-arg forms never ran)
class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* server, NimBLEConnInfo& connInfo) override {
    (void)server;
    (void)connInfo;
    deviceConnected = true;
    lastAppHbMs = millis();
    Serial.println("BLE client connected");
  }
  void onDisconnect(NimBLEServer* server, NimBLEConnInfo& connInfo, int reason) override {
    (void)server;
    (void)connInfo;
    (void)reason;
    deviceConnected = false;
    Serial.println("BLE client disconnected — re-advertising");
    NimBLEDevice::startAdvertising();
  }
};

class RxCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
    (void)connInfo;
    NimBLEAttValue value = pCharacteristic->getValue();
    if (value.length() > 0) {
      processBLECommand(String(value.c_str()));
    }
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
  // NimBLE 2.x: services start with the GATT server when advertising begins
  // (NimBLEService::start() is a no-op / deprecated)
  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->setName("TMDrake_tail");
  adv->addServiceUUID(NUS_SERVICE_UUID);
  adv->enableScanResponse(true);
  adv->start();  // also starts the GATT server + all services
  Serial.println("NimBLE NUS advertising as TMDrake_tail");
}

/**
 * USB serial console uses the same command language as BLE NUS
 * (for the Tail TUI / screen / Arduino Serial Monitor).
 * Lines ending in \\n or \\r; max 64 chars.
 */
void checkSerialBT() {
  static char line[72];
  static uint8_t len = 0;
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\r' || c == '\n') {
      if (len > 0) {
        line[len] = '\0';
        processBLECommand(String(line));
        len = 0;
      }
    } else if (len < sizeof(line) - 1) {
      line[len++] = c;
    } else {
      len = 0;  // overflow — drop line
    }
  }
}
