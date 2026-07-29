# Drake 2.0 – System Documentation

**Last updated:** July 2026  
**Purpose:** Single reference for architecture, protocols, pins, modes, and how the three boards work together. Read this before changing firmware or the companion app.

---

## 1. Hardware roles

| Board | Repo | MCU | Role |
|-------|------|-----|------|
| **Tail** | `DRAKE_2_0_TAIL` | ESP32 (Node32S) | Hub: mic, BLE control, WiFi client, ASK TX to paws, local NeoPixels |
| **Head** | `DRAKE_2_0_HEAD` | ESP8266 (NodeMCU) | SoftAP, UDP command/mic receive, eyes + spike disks, temp + light sensors, fan |
| **PAWB** | `DRAKE_2_0_PAWB` | Arduino Pro Mini 5V/16MHz | Claw NeoPixels (×5), ASK RX from Tail |

```
                    ┌─────────────────┐
                    │  Phone / App    │
                    │  (BLE NUS)      │
                    └────────┬────────┘
                             │ BLE
                             ▼
┌──────────────┐    ┌─────────────────┐    UDP binary mic
│  PAWB (×2)   │◄───│  TAIL (ESP32)   │──────────────►┌─────────────────┐
│  ASK RX      │ASK │  Mic + ASK TX   │               │  HEAD (ESP8266) │
│  5 NeoPixels │    │  12 NeoPixels   │◄── temp/light─┤  SoftAP         │
└──────────────┘    └─────────────────┘               │  50 NeoPixels   │
                                                      └─────────────────┘
```

---

## 2. Pin map & radio

### Tail (ESP32)
| Function | Pin / setting |
|----------|----------------|
| NeoPixels (spikes) | GPIO 22 |
| Mic (analog) | A0, offset 1600 |
| ASK **transmitter** | GPIO **17** |
| RadioHead ASK | `RH_ASK(2000, 0, 17, 0)` — **2000 baud**, TX only |
| WiFi | STA to SSID `TMDRAKE`, static IP `192.168.4.10` |

### Head (ESP8266)
| Function | Pin / setting |
|----------|----------------|
| NeoPixels | D2, 50 LEDs |
| Fan | D4 |
| Light sensor (CDS) | A0 |
| Temp (OneWire) | D5 |
| SoftAP | SSID `TMDRAKE`, IP `192.168.4.1`, channel 2, hidden |
| WiFi sleep | **Disabled** (`WIFI_NONE_SLEEP`) |

### PAWB (Pro Mini)
| Function | Pin / setting |
|----------|----------------|
| NeoPixels (claws) | Pin 8, 5 LEDs |
| ASK **receiver** | A0 |
| RadioHead ASK | `RH_ASK(2000, A0, 0, 0)` — **same 2000 baud**, RX only |

**ASK “channel”:** same bit rate (2000) + matching 433/315 MHz modules. No software channel ID.

---

## 3. Network ports

| Port | Direction | Payload |
|------|-----------|---------|
| **1234** | Tail → Head (UDP) | Commands: `M*`, `L*`, `R*` |
| **1237** | Tail → Head (UDP) | **Mic stream** — preferred: 2-byte big-endian int16; ASCII still accepted |
| **1235** | Head → Tail (UDP) | Light sensor value (int) |
| **1236** | Head → Tail (UDP) | Head temperature °F (float) |

Tail WiFi power-save is **off** (`WiFi.setSleep(false)`).

---

## 4. ASK packets (Tail → PAWB)

| Packet | Meaning |
|--------|---------|
| `M0` … `M9` | Set mode 0–9 |
| `MA` | Set mode 10 (Off) |
| `L0` (or `L…`) | Flash |
| `R0` (or `R…`) | Resync / reset fade |
| `m####` | Mic level (ASCII, for sound-reactive modes on paws) |

---

## 5. Lighting modes (0–10)

Implemented on **Tail** and **PAWB**. Head still primarily uses 0/1 style sound loops + fade (mode byte is received).

| ID | Name | Behaviour |
|----|------|-----------|
| 0 | Sound Phase | Color-phase sound reactive |
| 1 | Sound Distinct | Hard color cycle sound reactive |
| 2 | VU Meter | Bar graph |
| 3 | Rainbow Chase | Flowing rainbow |
| 4 | Comet / Meteor | Bright head + trail |
| 5 | Breathing Pulse | Brightness pulse + hue |
| 6 | Fire Flicker | Heat-style flicker |
| 7 | Sparkle / Twinkle | Random sparkles |
| 8 | Wave / Undulate | Traveling brightness wave |
| 9 | Solid / Static | Solid color (default purple-blue) |
| 10 | Off / Blackout | All off |

- Modes **0–2**: sound-reactive (need mic activity).
- Modes **3–10**: run continuously on Tail and PAWB.

---

## 6. BLE control (Tail only)

### Nordic UART Service
| Role | UUID |
|------|------|
| Service | `6E400001-B5A3-F393-E0A9-E50E24DCCA9E` |
| RX (app → suit) | `6E400002-B5A3-F393-E0A9-E50E24DCCA9E` |
| TX (suit → app) | `6E400003-B5A3-F393-E0A9-E50E24DCCA9E` |

**Device name:** `TMDrake_tail`  
**Stack:** Arduino + **NimBLE-Arduino** (locked until Drake 3.0)

### Commands (write UTF-8 to RX)

| Cmd | Format | Notes |
|-----|--------|-------|
| Mode | `M<0-10>` | Also forwarded over UDP + ASK |
| Brightness | `B<0-100>` | Master brightness %, NVS |
| Speed | `V<0-100>` | Anim speed (50 = normal), NVS |
| Sensitivity | `S<n>` | Mic sensitivity, NVS |
| Sound on/off | `E` / `e` | NVS |
| Flash | `L` | |
| Resync | `R` | |
| Reboot | `Z` | |
| Status | `?` | Full help + state dump |

### Live telemetry (TX notify, ~2 Hz when connected)

```text
STAT M:3 B:80 V:50 S:75 E:1 Mic:1423 HeadB:512 HeadT:36.5
```

App contract details: **[APP_INTERFACE.md](APP_INTERFACE.md)**

---

## 7. Binary mic stream (Tail → Head)

**Why:** Lower latency than ASCII broadcast.

**Format:** 2 bytes, big-endian `int16`, value clamped 0…32767  
**Destination:** unicast `192.168.4.1:1237`  
**Throttle:** only send when `|new - last| >= 8` (or zero)

Head `checkUDP_sound()`:
- `n == 2` → binary parse  
- else → ASCII `atol` fallback  

ASK to paws still uses ASCII `m####` (unchanged).

---

## 8. Key source files

### Tail
| File | Role |
|------|------|
| `DRAKE_2_0_TAIL.ino` | setup/loop, WiFi, EEPROM, mode_selector, sound_detect |
| `Serial_RoutineBT.ino` | NimBLE NUS, commands, `STAT` push |
| `sound_activate.ino` | soundloop, **binary mic TX**, ASK mic |
| `New_Modes.ino` | Modes 3–10 (non-blocking) |
| `Background_loop.ino` | Idle fading |
| `SoundCheck.ino` | VU mode |
| `APP_INTERFACE.md` | App contract |
| `SYSTEM.md` | This document |
| `IMPROVEMENTS.md` | Roadmap / status |

### Head
| File | Role |
|------|------|
| `DRAKE_2_0_HEAD.ino` | SoftAP, UDP cmds, **binary mic RX**, sensors → Tail |
| `sound_activate.ino` | Head soundloop using remote micLevel |
| `Background_loop.ino` | Idle fade |
| `eyes_led.ino` | Eye brightness |

### PAWB
| File | Role |
|------|------|
| `DRAKE_2_0_PAWB.ino` | ASK RX, mode 0–10 routing |
| `New_Modes.ino` | Modes 2–10 for 5 pixels |
| `sound_activate.ino` | Modes 0–1 soundloop |
| `Background_loop.ino` | Idle fade |

---

## 9. Settings persistence (Tail NVS)

| Key class | Variable | Range |
|-----------|----------|-------|
| `MODE` | `mode` | 0–10 |
| `SENSITIVITY` | `sensitivity` | ~−1500…4000 |
| `ENABLESOUND` | `enableSound` | 0/1 |
| `BRIGHTNESS` | `masterBrightness` | 0–100 |
| `SPEED` | `animSpeed` | 0–100 |

PAWB stores last mode in EEPROM address 0.

---

## 10. Flash / test checklist

1. Flash **Head**, then **Tail**, then **PAWB**(s).  
2. Confirm SoftAP `TMDRAKE` and Tail joins as `.10`.  
3. BLE: nRF Connect → `TMDrake_tail` → NUS → subscribe TX → write `?`.  
4. `M6` / `M3` — Tail + paws should match; Head reacts to mode on 1234.  
5. Speak near Tail mic — Head spikes should animate; `STAT` Mic: value moves.  
6. Head temp/light should appear in `STAT` as `HeadT` / `HeadB`.

---

## 11. Design decisions (don’t casually reverse)

- **Arduino + NimBLE** on Tail until Drake 3.0 (not pure ESP-IDF).  
- **Service UUID** advertising for reliable phone discovery.  
- **Binary mic** is Tail→Head only; paws stay on ASK ASCII.  
- **Modes 3–10** continuous; 0–2 gated by sound.  
- Human-readable command letters kept for serial/BLE debugging.  
- App work is contracted in `APP_INTERFACE.md`; firmware is source of truth for protocol.

---

## 12. Suggested next work

- Color / theme commands (`C`, `T`) on Tail (+ solid color on PAWB)  
- Head mode parity (more than 0/1 visual styles)  
- Optional ESP-NOW mic path  
- Companion app implementation against `APP_INTERFACE.md`  

---

*Keep this file updated when ports, pins, commands, or board roles change.*
