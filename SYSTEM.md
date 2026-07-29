# Drake 2.0 – System Documentation

**Last updated:** July 2026  
**Audience:** Firmware team + app team  
Related: [ESPNOW.md](ESPNOW.md) · [APP_INTERFACE.md](APP_INTERFACE.md) · [SETTINGS.md](SETTINGS.md)

---

## 1. Hardware roles

| Board | Repo | MCU | Role |
|-------|------|-----|------|
| **Tail** | `DRAKE_2_0_TAIL` | ESP32 | Mic, BLE, ESP-NOW, ASK TX, modes 0–10 |
| **Head** | `DRAKE_2_0_HEAD` | ESP8266 | SoftAP ch2, ESP-NOW, modes 0–10, CDS eyes, fan, temp |
| **PAWB** | `DRAKE_2_0_PAWB` | Pro Mini | Claws, ASK RX, modes 0–10 |

```
Phone (BLE) → Tail → ESP-NOW ↔ Head
                ↓ ASK
              PAWB claws
```

---

## 2. Lighting modes (0–10) — **full suit parity**

All active paths are **non-blocking** (`millis()` gates, no `delay()` on the hot path).

| ID | Name | Tail | Head | PAWB |
|----|------|:----:|:----:|:----:|
| 0 | Sound Phase | ✅ | ✅ | ✅ |
| 1 | Sound Distinct | ✅ | ✅ | ✅ |
| 2 | VU Meter | ✅ | ✅ | ✅ |
| 3 | Rainbow Chase | ✅ | ✅ | ✅ |
| 4 | Comet | ✅ | ✅ | ✅ |
| 5 | Breathing | ✅ | ✅ | ✅ |
| 6 | Fire | ✅ | ✅ | ✅ |
| 7 | Sparkle | ✅ | ✅ | ✅ |
| 8 | Wave | ✅ | ✅ | ✅ |
| 9 | Solid | ✅ | ✅ | ✅ |
| 10 | Off | ✅ | ✅ | ✅ |

### Behaviour rules

- Modes **0–1** (and Tail/PAWB sound path): mic-reactive; idle fade when quiet (after timeout).
- Modes **2–10**: run **continuously** once selected.
- Mode change: Tail broadcasts `M0`…`M9` / `MA` over **ESP-NOW + UDP + ASK**.
- Head eyes (pixels **0–3**): after each frame, CDS dim is re-applied (`I` threshold, `D` percent).

### Source files

| Board | Mode code |
|-------|-----------|
| Tail | `New_Modes.ino` (3–10), `sound_activate.ino` (0–1), `SoundCheck.ino` (2), `mode_selector` in main |
| Head | `New_Modes.ino` (2–10 + selector), `sound_activate.ino` (0–1), `Background_loop.ino` (idle) |
| PAWB | `New_Modes.ino` (full selector 0–10), `sound_activate.ino` (0–1) |

**Legacy:** Head `Other_modes.ino` still has old blocking demos — **not** used by `mode_selector`. Do not call them from the main loop.

---

## 3. Mic path (Tail)

```text
delta  = max(0, analogRead(A0) - OFFSET)   // OFFSET = 1600
level  = delta * (micGain/100) + sensitivity
smooth = EMA(level)
wake   = smooth > micGate   // modes 0–1
```

| Setting | Cmd | Default | NVS |
|---------|-----|---------|-----|
| Sensitivity | `S` | 75 | yes |
| Gate | `G` | 100 | yes |
| Gain % | `A` | 100 | yes |
| Sound enable | `E`/`e` | on | yes |

Stream to Head: ESP-NOW type `0x01` (int16). Paws: ASK `m####`.

---

## 4. Head sensors

### CDS → eyes
- A0 photocell; `dim_eyes = (reading >= cdsThreshold)` (default 500).
- Eyes use `eyeDimPercent` (default 10%) when dimmed.
- App: `I<n>`, `D<n>`; live `STAT HeadB`.

### Fan + temp
- OneWire temp D5; fan D4.
- `F0`/`F1`/`F2` + `FT<n>` (°F, default 85). Live `STAT HeadT`.

---

## 5. Links

| Link | Medium |
|------|--------|
| Tail↔Head | Encrypted ESP-NOW (mic, cmds, light, temp); UDP fallback |
| Tail→PAWB | ASK 2000 baud, pin 17 → A0 |
| Phone→Tail | BLE NUS |

ESP-NOW details: [ESPNOW.md](ESPNOW.md) (peer MACs required on bench).

---

## 6. BLE command summary

`M B V S G A E/e L R Z ?` + Head `F0/F1/F2 FT I D`  
Full contract: [APP_INTERFACE.md](APP_INTERFACE.md) · [SETTINGS.md](SETTINGS.md)

---

## 7. Firmware team – done vs backlog

### Done
- Modes 0–10 non-blocking on **Tail, Head, PAWB**
- ESP-NOW encrypted Tail↔Head
- Mic gain/gate/EMA on Tail
- Fan + CDS app settings on Head
- BLE STAT telemetry

### Backlog (ideas, not blocking ship)
- Color command `C<r>,<g>,<b>` for Solid mode sync
- Auto-calibrate mic OFFSET / “silence” button
- Drop UDP fallback once ESP-NOW proven on-suit
- Delete or archive Head `Other_modes.ino` blocking demos
- Optional ESP-NOW for more Head telemetry fields

---

## 8. Flash order

1. Head (modes + ESP-NOW)  
2. Tail (set `HEAD_PEER_MAC`)  
3. PAWB(s)  
4. Confirm ESP-NOW ready; test `M3`–`M10` on all three

---

*Update this file when mode behaviour or board roles change.*
