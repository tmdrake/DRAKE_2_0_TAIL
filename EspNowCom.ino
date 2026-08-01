/*
 * EspNowCom.ino – Encrypted ESP-NOW link (Tail ESP32 ↔ Head ESP8266)
 *
 * BENCH SETUP:
 *  1. Flash Head and Tail once.
 *  2. Open Serial at 115200 on BOTH boards — each prints its MAC.
 *  3. Copy Head MAC into HEAD_PEER_MAC below.
 *  4. Copy Tail MAC into TAIL_PEER_MAC on the Head firmware.
 *  5. Reflash both. SoftAP stays on channel 2; ESP-NOW uses the same channel.
 *
 * Packet format (first byte = type):
 *  0x01 MIC   + int16 BE level
 *  0x02 CMD   + ASCII (e.g. "M6", "L0", "R0")
 *  0x03 LIGHT + uint16 BE
 *  0x04 TEMP  + int16 BE (degrees F × 10)
 *  0x05 PHASE + uint16 BE phase + uint8 mode  (~25 Hz anim sync)
 *
 * Encryption: PMK + per-peer LMK (AES). Change keys if you want a private suit key.
 */

// esp_now.h / esp_wifi.h included from DRAKE_2_0_TAIL.ino (Arduino prototype order)

// ---- Peer MAC ----
// Hardcoded fallback from Head flash log; after STA joins SoftAP we prefer WiFi.BSSID()
// (ESP8266 SoftAP MAC can differ from STA MAC — wrong peer = silent no-sync)
uint8_t HEAD_PEER_MAC[6] = {0x9C, 0x9C, 0x1F, 0x46, 0x82, 0x33};

// Keys kept for future; ESP32↔ESP8266 encrypted ESP-NOW is unreliable across stacks
static const char ESPNOW_PMK[17] = "TMDrakePMK_2026!";
static const char ESPNOW_LMK[17] = "TMDrakeLMK_2026!";

#define ESPNOW_CH 2   // Must match Head SoftAP channel
// Cross-chip: disable AES peer encrypt (still channel-locked SoftAP link)
#define ESPNOW_ENCRYPT 0

enum EspNowType : uint8_t {
  EN_MIC   = 0x01,
  EN_CMD   = 0x02,
  EN_LIGHT = 0x03,
  EN_TEMP  = 0x04,
  EN_PHASE = 0x05
};

bool espnowReady = false;

/** Force radio onto Head SoftAP channel so peer channel matches home channel. */
void ensureEspNowChannel() {
  uint8_t primary = 0;
  wifi_second_chan_t second = WIFI_SECOND_CHAN_NONE;
  if (esp_wifi_get_channel(&primary, &second) == ESP_OK && primary == ESPNOW_CH) {
    return;
  }
  // STA on a different AP/channel breaks ESP-NOW to Head — drop and lock ch2
  if (WiFi.status() == WL_CONNECTED && WiFi.channel() != (int)ESPNOW_CH) {
    WiFi.disconnect(false, false);
    delay(30);
    yield();
  }
  esp_err_t err = esp_wifi_set_channel(ESPNOW_CH, WIFI_SECOND_CHAN_NONE);
  if (err != ESP_OK) {
    // Fallback Arduino API
    WiFi.setChannel(ESPNOW_CH, WIFI_SECOND_CHAN_NONE);
  }
}

// ESP32 Arduino 3.x / IDF 5.x callback signatures
void onEspNowSent(const esp_now_send_info_t *tx_info, esp_now_send_status_t status) {
  (void)tx_info;
  // Optional debug:
  // Serial.println(status == ESP_NOW_SEND_SUCCESS ? "EN OK" : "EN FAIL");
  (void)status;
}

void onEspNowRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  (void)info;
  if (len < 1 || data == nullptr) return;
  uint8_t type = data[0];

  if (type == EN_LIGHT && len >= 3) {
    uint16_t v = ((uint16_t)data[1] << 8) | data[2];
    head_brightness = (int)v;
  } else if (type == EN_TEMP && len >= 3) {
    int16_t t10 = ((int16_t)data[1] << 8) | data[2];
    head_temperature = t10 / 10.0f;
  }
}

void printMac(const uint8_t *m) {
  char buf[24];
  sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X", m[0], m[1], m[2], m[3], m[4], m[5]);
  Serial.println(buf);
}

bool addOrUpdateHeadPeer(const uint8_t *mac) {
  if (!mac) return false;
  esp_now_del_peer(HEAD_PEER_MAC);
  memcpy(HEAD_PEER_MAC, mac, 6);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, HEAD_PEER_MAC, 6);
  peer.channel = 0;  // use current home channel (we lock ch 2)
  peer.ifidx = WIFI_IF_STA;
#if ESPNOW_ENCRYPT
  peer.encrypt = true;
  memcpy(peer.lmk, ESPNOW_LMK, 16);
#else
  peer.encrypt = false;
#endif
  if (esp_now_add_peer(&peer) != ESP_OK) {
    Serial.print("ESP-NOW add peer FAILED for ");
    printMac(HEAD_PEER_MAC);
    return false;
  }
  Serial.print("ESP-NOW peer Head MAC ");
  printMac(HEAD_PEER_MAC);
  return true;
}

/** After SoftAP join, use BSSID (Head SoftAP MAC) — correct peer for ESP8266 AP mode. */
void refreshHeadPeerFromSoftAP() {
  if (WiFi.status() != WL_CONNECTED) return;
  const uint8_t *bssid = WiFi.BSSID();
  if (!bssid) return;
  bool same = true;
  for (int i = 0; i < 6; i++) {
    if (bssid[i] != HEAD_PEER_MAC[i]) { same = false; break; }
  }
  if (same && espnowReady) return;
  Serial.print("SoftAP BSSID ");
  printMac(bssid);
  if (addOrUpdateHeadPeer(bssid)) {
    espnowReady = true;
  }
}

bool setupEspNow() {
  ensureEspNowChannel();

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init FAILED");
    return false;
  }

  esp_now_register_send_cb(onEspNowSent);
  esp_now_register_recv_cb(onEspNowRecv);

#if ESPNOW_ENCRYPT
  if (esp_now_set_pmk((const uint8_t *)ESPNOW_PMK) != ESP_OK) {
    Serial.println("ESP-NOW PMK set failed");
  }
#endif

  ensureEspNowChannel();

  // Prefer live SoftAP BSSID when STA is up; else hardcoded fallback
  if (WiFi.status() == WL_CONNECTED && WiFi.BSSID()) {
    if (!addOrUpdateHeadPeer(WiFi.BSSID())) {
      espnowReady = false;
      return false;
    }
  } else {
    if (!addOrUpdateHeadPeer(HEAD_PEER_MAC)) {
      espnowReady = false;
      return false;
    }
  }

  espnowReady = true;
  ensureEspNowChannel();
  uint8_t ch = 0;
  wifi_second_chan_t sec = WIFI_SECOND_CHAN_NONE;
  esp_wifi_get_channel(&ch, &sec);
  Serial.print("ESP-NOW ready → Head  ch=");
  Serial.print(ch);
#if ESPNOW_ENCRYPT
  Serial.println(" encrypt=ON");
#else
  Serial.println(" encrypt=OFF (ESP32↔ESP8266 reliable)");
#endif
  Serial.print("This Tail MAC: ");
  uint8_t mac[6];
  WiFi.macAddress(mac);
  printMac(mac);
  return true;
}

void espnowSendMic(int16_t level) {
  if (!espnowReady) return;
  ensureEspNowChannel();
  uint8_t pkt[3];
  pkt[0] = EN_MIC;
  pkt[1] = (level >> 8) & 0xFF;
  pkt[2] = level & 0xFF;
  esp_err_t e = esp_now_send(HEAD_PEER_MAC, pkt, 3);
  (void)e;
}

void espnowSendCmd(const char *cmd) {
  if (!espnowReady || !cmd) return;
  ensureEspNowChannel();
  size_t n = strlen(cmd);
  if (n > 30) n = 30;
  uint8_t pkt[32];
  pkt[0] = EN_CMD;
  memcpy(pkt + 1, cmd, n);
  esp_err_t e = esp_now_send(HEAD_PEER_MAC, pkt, n + 1);
  if (e != ESP_OK) {
    static unsigned long lastLog = 0;
    if (millis() - lastLog > 3000) {
      lastLog = millis();
      Serial.print("ESP-NOW CMD send fail err=");
      Serial.println((int)e);
    }
  }
}

/** Animation phase master → Head (~4 bytes, call ~25 Hz). */
void espnowSendPhase(uint16_t phase, uint8_t modeId) {
  if (!espnowReady) return;
  ensureEspNowChannel();
  uint8_t pkt[4];
  pkt[0] = EN_PHASE;
  pkt[1] = (phase >> 8) & 0xFF;
  pkt[2] = phase & 0xFF;
  pkt[3] = modeId;
  esp_now_send(HEAD_PEER_MAC, pkt, 4);
}
