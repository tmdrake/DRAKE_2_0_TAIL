# Drake 2.0 – System Documentation

**Last updated:** July 2026  
**Purpose:** Single reference for architecture, protocols, pins, modes, and how the three boards work together. Read this before changing firmware or the companion app.

Related: **[ESPNOW.md](ESPNOW.md)** (encrypted Tail↔Head link) · **[APP_INTERFACE.md](APP_INTERFACE.md)** (phone app contract)

---

## 1. Hardware roles

| Board | Repo | MCU | Role |
|-------|------|-----|------|
| **Tail** | `DRAKE_2_0_TAIL` | ESP32 (Node32S) | Hub: analog mic, BLE, WiFi STA, **ESP-NOW**, ASK TX, NeoPixels |
| **Head** | `DRAKE_2_0_HEAD` | ESP8266 (NodeMCU) | SoftAP ch2, **ESP-NOW**, eyes/spikes, temp + light, fan |
| **PAWB** | `DRAKE_2_0_PAWB` | Arduino Pro Mini 5V/16MHz | Claw NeoPixels (×5), ASK RX from Tail |

```
                    ┌─────────────────┐
                    │  Phone / App    │
                    │  (BLE NUS)      │
                    └────────┬────────┘
                             │ BLE
                             ▼
┌──────────────┐    ┌─────────────────┐  ESP-NOW (encrypted)
│  PAWB (×2)   │◄───│  TAIL (ESP32)   │◄──────────────────►┌─────────────────┐
│  ASK RX      │ASK │  Mic + ASK TX   │   mic, cmd, sensors │  HEAD (ESP8266) │
│  5 NeoPixels │    │  12 NeoPixels   │                     │  SoftAP ch 2    │
└──────────────┘    └─────────────────┘                     │  50 NeoPixels   │
                                                            └─────────────────┘
```

---

## 2. Pin map & radio

### Tail (ESP32)
| Function | Pin / setting |
|----------|----------------|
| NeoPixels (spikes) | GPIO 22 |
| Mic (analog) | **A0**, `OFFSET = 1600` (bias voltage in ADC counts) |
| ASK **transmitter** | GPIO **17**, `RH_ASK(2000, 0, 17, 0)` |
| WiFi | STA to `TMDRAKE`, IP `192.168.4.10` |
| ESP-NOW | Channel **2**, encrypted peer = Head MAC |

### Head (ESP8266)
| Function | Pin / setting |
|----------|----------------|
| NeoPixels | D2, 50 LEDs |
| Fan | D4 |
| Light sensor (CDS) | A0 |
| Temp (OneWire) | D5 |
| SoftAP | SSID `TMDRAKE`, IP `192.168.4.1`, **channel 2**, hidden |
| WiFi sleep | **Disabled** |
| ESP-NOW | Channel **2**, encrypted peer = Tail MAC |

### PAWB (Pro Mini)
| Function | Pin / setting |
|----------|----------------|
| NeoPixels | Pin 8, 5 LEDs |
| ASK **receiver** | A0, `RH_ASK(2000, A0, 0, 0)` |

---

## 3. Inter-board links

| Link | Medium | Payload |
|------|--------|---------|
| Tail → Head mic | **ESP-NOW** type `0x01` | int16 level |
| Tail → Head cmds | **ESP-NOW** type `0x02` (+ UDP fallback) | `M*`, `L*`, `R*` |
| Head → Tail light | **ESP-NOW** type `0x03` (+ UDP 1235) | uint16 |
| Head → Tail temp | **ESP-NOW** type `0x04` (+ UDP 1236) | int16 °F×10 |
| Tail → PAWB | **ASK 2000 baud** | `M*`, `L*`, `R*`, `m####` |
| Phone → Tail | **BLE NUS** | command letters |

Full ESP-NOW detail: **[ESPNOW.md](ESPNOW.md)**

---

## 4. ASK packets (Tail → PAWB)

| Packet | Meaning |
|--------|---------|
| `M0` … `M9` | Mode 0–9 |
| `MA` | Mode 10 |
| `L…` | Flash |
| `R…` | Resync |
| `m####` | Mic level (ASCII) |

---

## 5. Lighting modes (0–10)

On **Tail** and **PAWB**. Head receives mode over ESP-NOW/UDP; visual style there is still primarily 0/1 sound loops + fade.

| ID | Name |
|----|------|
| 0–2 | Sound Phase / Distinct / VU |
| 3–8 | Rainbow, Comet, Breath, Fire, Sparkle, Wave |
| 9–10 | Solid, Off |

---

## 6. BLE control (Tail)

Device: `TMDrake_tail` · NUS UUID `6E400001-…` · NimBLE-Arduino  
Commands: `M`, `B`, `V`, `S`, `E`/`e`, `L`, `R`, `Z`, `?`  
Live: `STAT M:… B:… Mic:… HeadB:… HeadT:…`  
See **[APP_INTERFACE.md](APP_INTERFACE.md)**

---

## 7. Analog mic (Tail)

```text
level = analogRead(A0) - 1600 + sensitivity
```

- Hardware is still an **analog mic** into the ESP32 ADC.  
- `OFFSET 1600` compensates for the preamp bias / mid-rail voltage.  
- Level is sent to Head via ESP-NOW and to paws via ASK.

---

## 8. Key source files

### Tail
`DRAKE_2_0_TAIL.ino`, `EspNowCom.ino`, `Serial_RoutineBT.ino`, `sound_activate.ino`, `New_Modes.ino`, …

### Head
`DRAKE_2_0_HEAD.ino`, `EspNowCom.ino`, `sound_activate.ino`, …

### PAWB
`DRAKE_2_0_PAWB.ino`, `New_Modes.ino`, …

---

## 9. Flash / test order

1. Flash Head, then Tail (set peer MACs after first Serial boot — see ESPNOW.md).  
2. Flash PAWB(s).  
3. Confirm ESP-NOW ready on both Serial logs.  
4. BLE `?` and `STAT`; speak into mic; change modes.

---

## 10. Design decisions

- Arduino + NimBLE on Tail until Drake 3.0.  
- **ESP-NOW + encryption** for Tail↔Head hot path; UDP retained as fallback.  
- ASK retained for paws (separate RF modules).  
- Same SoftAP channel (2) for ESP-NOW coexistence.  

---

## 11. Suggested next work

- Color / theme commands (`C`, `T`)  
- Head visual parity for modes 3–10  
- Companion app against APP_INTERFACE.md  
- Optional: drop UDP fallback once ESP-NOW is proven on the suit  

---

*Update this file and ESPNOW.md when links or pins change.*
