# Tail mic – analog path & lowpass filter

**Board:** ESP32 Tail (`A0` / GPIO36)

---

## System note – mic module (current hardware)

| Item | Detail |
|------|--------|
| **Module** | Electret mic + **preamp with built-in AGC** (Adafruit-style / normalizing board) |
| **AGC** | Automatic gain control compresses loud/soft levels toward a usable mid range |
| **Bias** | **~1.25 V DC** mid-rail on a **0–3.3 V** output (ESP32-safe if board is powered at 3.3 V) |
| **Why AGC** | Suit audio levels vary wildly (quiet hall → DJ drop). AGC keeps the ADC from sitting in the weeds or slamming the rail. |
| **Implication** | Envelope modes (0–2) stay usable without constant `A`/`S` babysitting. For a **future FFT / spectrum** path, AGC **flattens dynamics** — great for “something is playing,” less ideal for true SPL or raw bass-impact metering. Design spectrum modes knowing the front end is **already compressed**. |

Do not treat the ADC stream as linear studio audio. Treat it as **AGC’d suit mic**.

---

## Software vs hardware (envelope path – **current**)

| Layer | Fact |
|-------|------|
| **Software** | Envelope tick **500 Hz** → Nyquist **250 Hz**. Design budget **~200 Hz**. Sample-rate side, not the RC label. |
| **Hardware LPF** | On-suit (recalled): **~10 kΩ + ~0.1 µF** → **fc ≈ 159 Hz**. Confirm on bench. |

This LPF is correct for **loudness / envelope**. It is **too low** for a full music spectrum (bass through presence). A future spectrum mode needs a **wider analog path** (higher fc or LPF bypass) — see [SPECTRUM_FUTURE.md](SPECTRUM_FUTURE.md).

---

## 1. Why hardware LPF (today)?

RF hash from WiFi/BLE/ESP-NOW, limit energy above the envelope sample budget, calmer ADC. Firmware: deadband / `A` / `S` / EMA / `G`.

---

## 2. Circuit (on-suit)

```text
                    R_lp
  Mic amp OUT ───/\/\/\/───┬──────── ESP32 A0 (ADC1_CH0 / GPIO36)
   (~1.25 V bias)          │
                        C_lp
                           │
                          GND
```

### On-suit values (recalled — verify on bench)

| Part | Value | Notes |
|------|--------|--------|
| **R_lp** | **~10 kΩ** | Series from amp OUT |
| **C_lp** | **~0.1 µF (100 nF)** | Often marked `104` |
| **fc** | **≈ 159 Hz** | `1 / (2π · 10e3 · 100e-9)` |

### If rebuilding (envelope-oriented)

| R | C | fc |
|---|---|----|
| 10 kΩ | 100 nF | **~159 Hz** (matches recalled build) |
| 8.2 kΩ | 100 nF | **~194 Hz** |

---

## 3. Bias & OFFSET

| Item | Value |
|------|--------|
| Amp bias | **~1.25 V** |
| ADC | 0–3.3 V → 0–4095 |
| `OFFSET` | ≈ **1551** (tune on bench) |

Do not AC-couple into A0 without mid-rail bias; firmware expects stable DC bias.

---

## 4. Bench verify

1. Measure R_lp / C_lp; update table if different.  
2. Silence → low Mic after deadband; clap → reacts.

---

## 5. Wiring

Common GND; C near A0; short leads; amp **3.3 V**; GPIO36 input-only.

---

*AGC mic + ~10k/0.1µF LPF = current envelope system. Spectrum/FFT = future design doc.*
