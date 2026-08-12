# DRAKE_2_0_TAIL

ESP32 (Node32S) firmware for the **dragonsuit tail** — system hub.

## Role
- Analog mic (`A0`, Adafruit ~1.25 V bias on 0–3.3 V) + fixed-rate envelope
- **BLE** remote control (NimBLE NUS)
- **ESP-NOW** ↔ Head (mic excess ~200 Hz, commands, phase sync, sensors)
- **ASK TX** (pin 17) → PAWB: mode/cmd always; **mic pulse only on M0–M2**
- WiFi STA to Head SoftAP (`TMDRAKE` ch2)
- 12× NeoPixels (GPIO 22)
- **USB serial** same command language as BLE (for tools / TUI)

### Mic scale (M0 Sound Phase / M1 Sound Pulse / M2 VU)
Noise-floor track → **excess** → `micNorm01()` (quiet ≈ empty bar / inject).  
Gate **G** wakes M0/M1; VU always paints from excess.

**Hardware RC LPF** is on the suit (values **TBD** until measured). Software sample budget ~**200 Hz** (500 Hz tick). See [MIC_HARDWARE.md](MIC_HARDWARE.md).

## Tail settings TUI (Mac / Linux)

![Drake Tail TUI](docs/tail_tui.svg)

USB serial dashboard for modes, brightness, mic, fan, themes, live STAT.

```bash
cd DRAKE_2_0_TAIL
pip3 install -r tools/requirements.txt
python3 tools/tail_tui.py
# or: python3 tools/tail_tui.py -p /dev/cu.usbserial-0001
```

Full keys & notes: **[tools/README.md](tools/README.md)**

## Docs
| Doc | Contents |
|-----|----------|
| **[SYSTEM.md](SYSTEM.md)** | Full architecture |
| **[MIC_HARDWARE.md](MIC_HARDWARE.md)** | Mic amp + RC LPF (topology; on-suit R/C TBD) |
| **[ESPNOW.md](ESPNOW.md)** | ESP-NOW MACs, packets, gotchas (SoftAP MAC, encrypt) |
| **[APP_INTERFACE.md](APP_INTERFACE.md)** | Phone app contract |
| **[APP_TEAM.md](APP_TEAM.md)** | App requirements (HB, link service) |
| **[REPO.md](REPO.md)** | What belongs in git (text, libs, assets) + agent upload limits |
| **[IMPROVEMENTS.md](IMPROVEMENTS.md)** | Roadmap |
| **[tools/README.md](tools/README.md)** | Tail TUI |

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
