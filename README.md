# DRAKE_2_0_TAIL

ESP32 (Node32S) firmware for the **dragonsuit tail** — system hub.

## Role
- Analog mic (`A0`, offset 1600) + sound-reactive modes
- **BLE** remote control (NimBLE NUS)
- **Encrypted ESP-NOW** ↔ Head (mic, commands, sensor feedback)
- **ASK TX** (pin 17) → PAWB claws
- WiFi STA to Head SoftAP
- 12× NeoPixels (GPIO 22)

## Docs
| Doc | Contents |
|-----|----------|
| **[SYSTEM.md](SYSTEM.md)** | Full architecture |
| **[ESPNOW.md](ESPNOW.md)** | ESP-NOW keys, MACs, packets, bench test |
| **[APP_INTERFACE.md](APP_INTERFACE.md)** | Phone app contract |
| **[IMPROVEMENTS.md](IMPROVEMENTS.md)** | Roadmap |

## ESP-NOW quick start
1. Flash Tail + Head, read MACs from Serial.  
2. Set `HEAD_PEER_MAC` in `EspNowCom.ino`.  
3. Set `TAIL_PEER_MAC` on Head.  
4. Keys: `TMDrakePMK_2026!` / `TMDrakeLMK_2026!`  
5. Reflash both.

http://tmdrake.com
