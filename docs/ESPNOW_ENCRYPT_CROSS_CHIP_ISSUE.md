# Draft: ESP-NOW peer encryption ESP32 ↔ ESP8266 silent failure

**Suggested venues (pick one):**
- https://github.com/espressif/arduino-esp32/issues  
- https://github.com/esp8266/Arduino/issues  
- https://github.com/espressif/esp-idf/issues (if reproducible with IDF + non-OS 8266)

**Title suggestion:**  
`ESP-NOW encrypted peer: ESP32 (Arduino 3.x) ↔ ESP8266 SoftAP fails silently despite matching PMK/LMK`

---

## Description

Encrypted ESP-NOW between **ESP32 (Arduino-ESP32 3.x / IDF 5)** and **ESP8266 (Arduino core SoftAP + ESP-NOW)** does not deliver application payloads even when:

- Both sides report peer add / “ESP-NOW ready”
- PMK and LMK are **identical 16-byte** strings
- Both are on the **same Wi‑Fi channel**
- ESP32 STA has **joined** the ESP8266 SoftAP
- **Unencrypted** peers on the same setup work immediately

Disabling encryption (`encrypt = false` / `add_peer(..., NULL, 0)`) restores CMD/MIC/PHASE delivery without other changes.

This is easy to mis-debug as “wrong MAC” or “app bug.” We hit both: SoftAP vs STA MAC **and** encrypt.

---

## Environment (repro from Drake 2.0 suit)

| Side | Hardware | Framework |
|------|----------|-----------|
| A (STA / sender) | ESP32-D0WDQ6 (Node32S) | Arduino-ESP32 **3.3.x** |
| B (SoftAP / receiver) | ESP8266EX (NodeMCU) | ESP8266 Arduino **3.1.x** |

- SoftAP: open network, **channel 2**, SSID visible  
- ESP32: `WiFi.mode(WIFI_STA)`, `WiFi.begin(ssid, NULL, 2)`, then ESP-NOW  
- ESP8266: `WiFi.mode(WIFI_AP)`, `WiFi.softAP(..., channel=2)`, then ESP-NOW  

### Keys (identical both sides)

```text
PMK: "TMDrakePMK_2026!"   // 16 bytes
LMK: "TMDrakeLMK_2026!"   // 16 bytes
```

### ESP32 (Arduino) sketch shape

```cpp
esp_now_set_pmk((const uint8_t*)PMK);
esp_now_peer_info_t peer = {};
memcpy(peer.peer_addr, head_mac, 6);
peer.channel = 0;           // home channel already 2
peer.encrypt = true;        // FAIL case
peer.ifidx = WIFI_IF_STA;
memcpy(peer.lmk, LMK, 16);
esp_now_add_peer(&peer);
esp_now_send(head_mac, pkt, len);
```

### ESP8266 (Arduino) sketch shape

```cpp
esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
esp_now_set_kok((u8*)PMK, 16);
esp_now_add_peer(tail_mac, ESP_NOW_ROLE_COMBO, 2, (u8*)LMK, 16);  // FAIL case
// recv_cb registered — never called for encrypted frames from ESP32
```

### Working change (only)

```cpp
// ESP32
peer.encrypt = false;

// ESP8266
esp_now_add_peer(tail_mac, ESP_NOW_ROLE_COMBO, 2, NULL, 0);
```

Same MACs, channel, SoftAP, payloads → **recv works**.

---

## Related gotcha (separate, also silent)

ESP8266 **SoftAP MAC ≠ STA MAC**.

Example from one unit:

```text
STA MAC:     9C:9C:1F:46:82:33
SoftAP MAC:  9E:9C:1F:46:82:33   // WiFi.BSSID() from STA client
```

If the ESP32 peers `WiFi.macAddress()` printed while Head is in AP mode (or the wrong interface), ESP-NOW “ready” but no delivery.  
**Fix:** peer SoftAP BSSID (`WiFi.BSSID()` after join / `WiFi.softAPmacAddress()` on AP).

Please document both issues if encrypt is unsupported across these stacks.

---

## Expected

- Document that **encrypted ESP-NOW is only guaranteed same-family / same-stack**, **or**
- Make ESP32 ↔ ESP8266 encrypt interoperable, **or**
- Fail loudly on `add_peer` / `set_kok` / first send when crypto is incompatible

## Actual

- `add_peer` succeeds  
- Send may not surface a clear app-level error  
- **No receive callback** on ESP8266 for encrypted frames from ESP32  
- Unencrypted works  

---

## Workaround (production)

`#define ESPNOW_ENCRYPT 0` on both ends; use SoftAP BSSID as peer; optional UDP unicast fallback.

Reference project notes:  
https://github.com/tmdrake/DRAKE_2_0_TAIL/blob/main/ESPNOW.md  
(section **BLAME: ESP-NOW / ESP8266 SoftAP gotchas**)

---

## Ask to Espressif / core maintainers

1. Is encrypted ESP-NOW **officially supported** between Arduino-ESP32 3.x and Arduino-ESP8266?  
2. If not, can docs state that clearly next to PMK/LMK examples?  
3. If yes, which API pairing is correct (`set_pmk`+`lmk` vs `set_kok`+key on add_peer)?

---

*Drafted from field debugging on a dual-board LED suit (Tail ESP32 + Head ESP8266), August 2026.*
