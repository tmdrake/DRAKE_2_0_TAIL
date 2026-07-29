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
 *
 * Encryption: PMK + per-peer LMK (AES). Change keys if you want a private suit key.
 */

#include <esp_now.h>
#include <esp_wifi.h>

// ---- CHANGE THESE AFTER FIRST BOOT (see Serial) ----
// Head MAC goes here (example format)
uint8_t HEAD_PEER_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};  // REPLACE

// 16-byte keys (must match Head exactly)
static const char ESPNOW_PMK[17] = "TMDrakePMK_2026!";
static const char ESPNOW_LMK[17] = "TMDrakeLMK_2026!";

#define ESPNOW_CH 2   // Must match Head SoftAP channel

enum EspNowType : uint8_t {
  EN_MIC   = 0x01,
  EN_CMD   = 0x02,
  EN_LIGHT = 0x03,
  EN_TEMP  = 0x04
};

bool espnowReady = false;

void onEspNowSent(const uint8_t *mac, esp_now_send_status_t status) {
  // Optional debug:
  // Serial.println(status == ESP_NOW_SEND_SUCCESS ? "EN OK" : "EN FAIL");
}

void onEspNowRecv(const uint8_t *mac, const uint8_t *data, int len) {
  if (len < 1) return;
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

bool setupEspNow() {
  // Channel must match SoftAP (Head uses channel 2)
  esp_wifi_set_channel(ESPNOW_CH, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init FAILED");
    return false;
  }

  esp_now_register_send_cb(onEspNowSent);
  esp_now_register_recv_cb(onEspNowRecv);

  // Primary master key
  if (esp_now_set_pmk((const uint8_t *)ESPNOW_PMK) != ESP_OK) {
    Serial.println("ESP-NOW PMK set failed");
  }

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, HEAD_PEER_MAC, 6);
  peer.channel = ESPNOW_CH;
  peer.encrypt = true;
  memcpy(peer.lmk, ESPNOW_LMK, 16);

  // If MAC still FF:FF:..., peer add will fail until user sets real MAC
  bool broadcastPlaceholder =
      (HEAD_PEER_MAC[0] == 0xFF && HEAD_PEER_MAC[1] == 0xFF &&
       HEAD_PEER_MAC[2] == 0xFF && HEAD_PEER_MAC[3] == 0xFF &&
       HEAD_PEER_MAC[4] == 0xFF && HEAD_PEER_MAC[5] == 0xFF);

  if (broadcastPlaceholder) {
    Serial.println("ESP-NOW: HEAD_PEER_MAC not set yet — set it and reflash");
    Serial.print("This Tail MAC: ");
    uint8_t mac[6];
    WiFi.macAddress(mac);
    printMac(mac);
    // Still init without peer so MAC print is useful
    espnowReady = false;
    return false;
  }

  if (esp_now_add_peer(&peer) != ESP_OK) {
    Serial.println("ESP-NOW add peer FAILED (check MAC + channel)");
    return false;
  }

  espnowReady = true;
  Serial.println("ESP-NOW ready (encrypted) → Head");
  Serial.print("This Tail MAC: ");
  uint8_t mac[6];
  WiFi.macAddress(mac);
  printMac(mac);
  return true;
}

void espnowSendMic(int16_t level) {
  if (!espnowReady) return;
  uint8_t pkt[3];
  pkt[0] = EN_MIC;
  pkt[1] = (level >> 8) & 0xFF;
  pkt[2] = level & 0xFF;
  esp_now_send(HEAD_PEER_MAC, pkt, 3);
}

void espnowSendCmd(const char *cmd) {
  if (!espnowReady || !cmd) return;
  size_t n = strlen(cmd);
  if (n > 30) n = 30;
  uint8_t pkt[32];
  pkt[0] = EN_CMD;
  memcpy(pkt + 1, cmd, n);
  esp_now_send(HEAD_PEER_MAC, pkt, n + 1);
}
