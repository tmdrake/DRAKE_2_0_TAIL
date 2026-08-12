# Tail mic – analog path & lowpass filter

**Board:** ESP32 Tail (`A0` / GPIO36)  
**Design cutoff:** **~200 Hz** (matches firmware / SYSTEM.md §3)  
**Amp:** Adafruit-style electret mic + preamp (AGC / normalizing), **~1.25 V DC bias**, output swing within **0–3.3 V**

---

## 1. Why a hardware lowpass?

| Reason | Detail |
|--------|--------|
| **Anti-alias** | Firmware samples envelope at **500 Hz** (2 ms tick). Nyquist = 250 Hz → design **fc ≈ 200 Hz**. |
| **Lighting use** | Modes care about **loudness envelope**, not full speech band. |
| **RF / digital noise** | WiFi, BLE, ESP-NOW hash on long mic leads; LPF helps. |
| **ADC** | Modest source Z + small C at the pin is kinder to the ESP32 SAR ADC. |

Software EMA still runs in firmware; the **RC is the first line of defence**.

---

## 2. Recommended circuit (1-pole RC)

```text
                    R_lp
  Mic amp OUT ───/\/\/\/───┬──────── ESP32 A0 (ADC1_CH0 / GPIO36)
   (~1.25 V bias)          │
                        C_lp
                           │
                          GND  (common with ESP32 GND)
```

### Preferred values — **fc ≈ 200 Hz**

| Part | Value | Role |
|------|-------|------|
| **R_lp** | **8.2 kΩ** | Series |
| **C_lp** | **100 nF** (0.1 µF) | To GND at ADC pin |

```text
fc = 1 / (2 · π · R · C)
   = 1 / (2 · π · 8.2e3 · 100e-9)
   ≈ 194 Hz   → treat as ~200 Hz design point
```

(1st-order; −3 dB near 194 Hz, gentle roll-off above.)

### Same topology, other E12-ish sets

| R_lp | C_lp | fc (approx) | Notes |
|------|------|-------------|--------|
| **8.2 kΩ** | **100 nF** | **~194 Hz** | **Default (~200 Hz)** |
| 10 kΩ | 82 nF | **~194 Hz** | Same fc if you have 82 nF |
| 7.5 kΩ | 100 nF | **~212 Hz** | Slightly open |
| 10 kΩ | 100 nF | **~159 Hz** | More aggressive / darker |
| 10 kΩ | 68 nF | **~234 Hz** | Near Nyquist — use only if needed |
| 4.7 kΩ | 180 nF | **~188 Hz** | Lower R if amp dislikes 8.2 k load |

**Cap:** X7R/C0G ceramic or film, ≥16 V. Avoid electrolytics here.

---

## 3. Optional 2-pole (steeper RF rejection)

```text
                 R1              R2
  Amp OUT ───/\/\/\/───┬───/\/\/\/───┬──── A0
                       │             │
                      C1            C2
                       │             │
                      GND           GND
```

Example near **~200 Hz**: R1 = R2 = **8.2 kΩ**, C1 = C2 = **100 nF** (approximate; two poles, sharper above fc).

Start with **1-pole**; add the second only if on-suit RF is still ugly.

---

## 4. Bias, levels, and OFFSET

| Item | Value |
|------|--------|
| Amp DC bias | **~1.25 V** |
| ADC full scale (11 dB) | **0–3.3 V** → **0–4095** |
| Firmware `OFFSET` | `1.25/3.3 * 4095 ≈ **1551**` (tune on bench) |

RC **does not strip DC** — bias stays so `|analogRead − OFFSET|` is valid AC.

Do **not** series-cap into A0 unless you add a proper mid-rail bias; firmware expects stable bias.

---

## 5. Wiring

1. Common **GND**: amp, RC, ESP32.  
2. **C_lp close to A0**.  
3. Short mic leads; twist if long.  
4. Amp **VCC = 3.3 V** (not 5 V).  
5. GPIO36 input-only — good for mic.

```text
[Electret + preamp] OUT (~1.25 V bias)
        │
     R_lp 8.2k
        │
        ├──── C_lp 100nF ── GND
        │
     ESP32 A0
```

---

## 6. BOM (default ~200 Hz)

| Qty | Part | Spec |
|-----|------|------|
| 1 | R_lp | **8.2 kΩ**, 1/8–1/4 W |
| 1 | C_lp | **100 nF**, ≥16 V |
| — | Mic amp | Existing Adafruit-style electret board |

---

## 7. Bench check

1. Silence → `Mic` near 0 after deadband.  
2. Speak / clap → Mic up; M0–M2 react.  
3. Always hot → check OFFSET / lower G, S, A.  
4. Optional scope: A0 smoother than raw amp OUT.

---

## 8. Firmware vs hardware

| Stage | What |
|-------|------|
| **RC** | **fc ≈ 200 Hz** |
| Sample tick | 500 Hz |
| Deadband / A / S / EMA / G | Software show tuning |

---

*Default: **8.2 kΩ + 100 nF → fc ≈ 194 Hz (~200 Hz)**. Keep SYSTEM.md in sync.*
