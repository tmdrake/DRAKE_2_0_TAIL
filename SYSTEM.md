# Drake 2.0 – System Documentation

**Last updated:** August 2026  
**Audience:** Firmware team + app team  
Related: [ESPNOW.md](ESPNOW.md) · [APP_INTERFACE.md](APP_INTERFACE.md) · [SETTINGS.md](SETTINGS.md) · [MIC_HARDWARE.md](MIC_HARDWARE.md)

---

## 1. Hardware roles

| Board | Repo | MCU | Role |
|-------|------|-----|------|
| **Tail** | `DRAKE_2_0_TAIL` | ESP32 | Mic, BLE, ESP-NOW, ASK TX, modes 0–10 |
| **Head** | `DRAKE_2_0_HEAD` | ESP8266 | SoftAP ch2, ESP-NOW, modes 0–10, CDS eyes, fan, temp |
| **PAWB** | `DRAKE_2_0_PAWB` | Pro Mini | Claws, ASK RX, modes 0–10 |

### Watchdog (WDT)

| Board | WDT | Notes |
|-------|-----|--------|
| **Tail** | Loop WDT **30 s** (`enableLoopWDT` + `esp_task_wdt_reset` in loop / after ASK) | Avoid long `delay()` in loop; `rainbow.ino` blocking demos unused |
| **Head** | ESP8266 SW WDT (~**3 s**) — `wdt_reset()` each loop + around DS18B20 re-probe | Never call blocking `Other_modes.ino` delays from loop |
| **PAWB** | AVR WDT not enabled in firmware | |

### Mode persistence (all regions)

| Board | Storage | When saved | Boot |
|-------|---------|------------|------|
| **Tail** | NVS (`MODE`) | BLE `M*` / solid / theme | Restores last mode |
| **Head** | EEPROM addr 0 | ESP-NOW/UDP `M*` / `C` / phase mode | Restores last mode |
| **PAWB** | EEPROM addr 0 | ASK `M*` / `C` | Restores last mode |

Tail remains the hub and re-pushes mode on suit sync; each region also keeps its own last mode if powered alone.

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
| 1 | Sound Pulse | ✅ | ✅ | ✅ |
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

- Modes **0–1** (Sound Phase / Sound Pulse): mic-reactive while gated; after ~10 s quiet (or sound detect off) → idle **purple pulse** (`fading()`). That pulse was the original keep-alive so power-save would not shut the Arduino down; it still breaths purple and periodically sends **R0** resync (ASK + UDP).
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

Adafruit electret amp (AGC/normalizing), **DC bias ~1.25 V**, rail **0–3.3 V max**.

**Hardware LPF:** series **R + C to GND** at A0, design cutoff **~200 Hz**.  
**Default parts: 8.2 kΩ + 100 nF → fc ≈ 194 Hz.**  
Full schematic, BOM, alternates, bench notes: **[MIC_HARDWARE.md](MIC_HARDWARE.md)**.

```text
// 12-bit ADC, ADC_11db → 0..3.3 V full scale
OFFSET = 1.25/3.3 * 4095 ≈ 1551
tick   every 2 ms  (500 Hz)   // non-blocking; ≥2× ~200 Hz
delta  = |analogRead(A0) - OFFSET|   // full-wave; deadband kills noise
if delta < MIC_DEADBAND → 0          // true silence (no DC offset add)
level  = delta × (A%/100) × (S%/100)
smooth = fast-attack/slow-release EMA → micLevelCached @ 50 Hz
wake   = smooth > micGate            // modes 0–1
```

| Setting | Cmd | Default | NVS |
|---------|-----|---------|-----|
| Sensitivity (gain %) | `S` | 100 | yes |
| Gate | `G` | 40 | yes |
| Gain % | `A` | 100 | yes |
| Sound enable | `E`/`e` | on | yes |

Stream to Head @ **~200 Hz** (`MIC_STREAM_MS=5`, ADC 1 kHz): ESP-NOW type `0x01` (int16 excess).  

**ASK mic pulse (M0/M1/M2 only):** `m####` at most ~25 Hz, only on real hits / release — no `waitPacketSent`.  
Modes **3–10**: ASK carries **M# / C / R0 / L** only (no mic stream).

**Shared scale** (`micNorm01()` / `micNormPct()`):

```text
micScalePeak = adaptive max of envelope (slow decay, min MIC_SCALE_MIN)
intensity    = micLevelCached / micScalePeak   // silence → 0
```

| Mode | Effect |
|------|--------|
| **0** | Color flood: shift + inject × intensity; smooth hue wheel |
| **1** | Same flood; stepped 6-color wheel every strip length |
| **2** | VU bar + peak-hold marker (same `micNorm01`) |

Head M0–M2 use the same `micNorm01()` on remote mic (ESP-NOW).

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

### Suit settings heartbeat (Tail → Head / PAWB)

- **When:** ~**2 s after Tail boot**, then every **~30 s**
- **What:** re-broadcast current **mode** (`M*`), **solid color** if mode 9 (`C*`), and last Head **fan/CDS** (`F*` `FT*` `I*` `D*`)
- **Why:** Head or paws rebooted / missed a packet still rejoin the same look without touching the app
- **Not:** animation phase lock (Breathe still free-runs per board)

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
- Mic analog LPF documented ([MIC_HARDWARE.md](MIC_HARDWARE.md)) — **fc ≈ 200 Hz**

### Backlog (ideas, not blocking ship)
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
