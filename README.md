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
| **[APP_TEAM.md](APP_TEAM.md)** | App requirements (HB, link service) |
| **[REPO.md](REPO.md)** | What belongs in git (text, libs, assets) + agent upload limits |
| **[IMPROVEMENTS.md](IMPROVEMENTS.md)** | Roadmap |

## Repo contents

Mostly `.ino` + markdown today. **Libraries and binary assets required for build or docs are allowed** — see **[REPO.md](REPO.md)**.  
Note: some automation tools can only push text; use local `git` or GitHub Upload for PNG/PDF/etc.

## ESP-NOW quick start
1. Flash Tail + Head, read MACs from Serial.  
2. Set `HEAD_PEER_MAC` in `EspNowCom.ino`.  
3. Set `TAIL_PEER_MAC` on Head.  
4. Keys: `TMDrakePMK_2026!` / `TMDrakeLMK_2026!`  
5. Reflash both.

http://tmdrake.com
