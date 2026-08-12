# Tail mic – analog path & lowpass filter

**Board:** ESP32 Tail (`A0` / GPIO36)  
**Amp:** Adafruit-style electret mic + preamp (AGC / normalizing), **~1.25 V DC bias**, output within **0–3.3 V**

---

## Software vs hardware

| Layer | Fact |
|-------|------|
| **Software** | Envelope tick **500 Hz** → Nyquist **250 Hz**. Design budget keeps useful content **around / under ~200 Hz**. That is **sample-rate side**, not the RC label. |
| **Hardware** | RC lowpass **is on the suit**. Recalled parts: **~10 kΩ** and **~0.1 µF (100 nF)**. Confirm with a meter when the board is open. |

---

## 1. Why hardware LPF?

RF hash from WiFi/BLE/ESP-NOW, limit energy above the software sample budget, calmer ADC source. Firmware still does deadband / `A` / `S` / EMA / `G`.

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
| **C_lp** | **~0.1 µF (100 nF)** | To GND near A0 |
| **fc** | **≈ 159 Hz** | `1 / (2π · 10e3 · 100e-9)` |

```text
fc = 1 / (2 · π · R · C)
   ≈ 1 / (2 · π · 10 kΩ · 0.1 µF)
   ≈ 159 Hz
```

Slightly under the ~200 Hz software budget — fine for anti-alias vs a 500 Hz tick (Nyquist 250 Hz). If the cap is marked `104` that is 100 nF; if it is a different 0.1-class part, recompute fc.

### If rebuilding

Same topology. Common equivalents:

| R | C | fc |
|---|---|----|
| 10 kΩ | 100 nF (0.1 µF) | **~159 Hz** (matches recalled build) |
| 8.2 kΩ | 100 nF | **~194 Hz** (closer to 200 Hz budget) |
| 10 kΩ | 82 nF | **~194 Hz** |

---

## 3. Bias & OFFSET

| Item | Value |
|------|--------|
| Amp bias | **~1.25 V** |
| ADC | 0–3.3 V → 0–4095 |
| `OFFSET` | ≈ **1551** (tune on bench) |

Do not AC-couple into A0 without a mid-rail bias; firmware expects stable DC bias.

---

## 4. Bench verify

1. Power off; measure **R_lp** and read/measure **C_lp**.  
2. Update this table if numbers differ.  
3. Optional: tone sweep, find −3 dB on A0 vs amp OUT.

---

## 5. Wiring

Common GND; **C close to A0**; short mic leads; amp on **3.3 V**; GPIO36 input-only.

---

*On-suit (recalled): **10 kΩ + 0.1 µF → fc ≈ 159 Hz**. Software ~200 Hz = Nyquist budget only.*
