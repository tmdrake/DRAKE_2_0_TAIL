# DRAKE_2_0_TAIL

ESP32 (Node32S) firmware for the **dragonsuit tail** — system hub.

## Role
- Mic sampling + sound-reactive modes
- **BLE** remote control (NimBLE Nordic UART Service)
- **WiFi STA** to Head SoftAP (`TMDRAKE` @ `192.168.4.10`)
- **ASK TX** (pin 17, 2000 baud) → PAWB claws
- **Binary mic stream** (UDP 1237) → Head
- 12× NeoPixels (GPIO 22)

## Docs (start here)
| Doc | Contents |
|-----|----------|
| **[SYSTEM.md](SYSTEM.md)** | Full architecture, pins, ports, modes, protocols |
| **[APP_INTERFACE.md](APP_INTERFACE.md)** | Contract for the TMDrake Android/Flutter app |
| **[IMPROVEMENTS.md](IMPROVEMENTS.md)** | Roadmap and implementation status |

## Quick BLE
- Name: `TMDrake_tail`
- NUS UUID: `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`
- Library: **NimBLE-Arduino** (h2zero)
- Commands: `M`, `B`, `V`, `S`, `E`/`e`, `L`, `R`, `Z`, `?`
- Live notify: `STAT M:… B:… V:… Mic:… HeadB:… HeadT:…`

## Board notes
- 4MB flash, NO-OTA, Large App partition
- Core 2.0.17 historically used; confirm NimBLE works on your core version

http://tmdrake.com
