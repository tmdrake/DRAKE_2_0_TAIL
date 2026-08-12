# Tail mic – analog path & lowpass filter

**Board:** ESP32 Tail (`A0` / GPIO36)  
**Firmware target:** hardware bandwidth **≤ ~200 Hz** (see [SYSTEM.md](SYSTEM.md) §3)  
**Amp:** Adafruit-style electret mic + preamp (AGC / normalizing), **~1.25 V DC bias**, output swing within **0–3.3 V**

---

## 1. Why a hardware lowpass?

| Reason | Detail |
|--------|--------|
| **Anti-alias** | Firmware samples envelope path at **500 Hz** (`tick` every 2 ms). Nyquist = 250 Hz → keep content **under ~200 Hz**. |
| **Lighting use** | Sound-reactive modes care about **loudness envelope**, not speech bandwidth. |
| **RF / digital noise** | ESP32 WiFi, BLE, ESP-NOW inject hash into long mic wires; LPF + short leads help. |
| **ADC friendliness** | ESP32 SAR ADC likes a modest source impedance and a small hold capacitance at the pin. |

Software EMA still runs in firmware; the **RC is the first line of defence**, not a replacement for gate/gain tuning.

---

## 2. Recommended circuit (1-pole RC)

Passive lowpass between **mic amp OUT** and **ESP32 A0**:

```text
                    R_lp
  Mic amp OUT ───/\/\/\/───┬──────── ESP32 A0 (ADC1_CH0 / GPIO36)
   (~1.25 V bias)          │
                        C_lp
                           │
                          GND  (common with ESP32 GND)
```

### Preferred values (fc ≈ 159 Hz)

| Part | Value | Role |
|------|-------|------|
| **R_lp** | **10 kΩ** | Series resistance |
| **C_lp** | **100 nF** (0.1 µF) | To GND at ADC pin |

**Cutoff (1st order):**

```text
fc = 1 / (2 · π · R · C)
   = 1 / (2 · π · 10e3 · 100e-9)
   ≈ 159 Hz
```

Comfortably under 200 Hz; still passes kick / voice energy useful for VU and modes 0–2.

### Alternate value sets (same topology)

| R_lp | C_lp | fc (approx) | Use when |
|------|------|-------------|----------|
| 10 kΩ | 100 nF | **159 Hz** | **Default** |
| 8.2 kΩ | 100 nF | **194 Hz** | Slightly brighter response |
| 10 kΩ | 68 nF | **234 Hz** | Near Nyquist — only if you need more high end |
| 4.7 kΩ | 220 nF | **154 Hz** | Lower R if amp dislikes 10 k load |
| 22 kΩ | 47 nF | **154 Hz** | Higher R; keep ≤ ~20–30 kΩ into ESP32 ADC |

**Cap type:** film or X7R/C0G ceramic; avoid huge electrolytics here (leakage / polarity). 100 nF ceramic is fine.

---

## 3. Optional 2-pole (steeper RF rejection)

If WiFi/BLE hash still rides the mic:

```text
                 R1              R2
  Amp OUT ───/\/\/\/───┬───/\/\/\/───┬──── A0
                       │             │
                      C1            C2
                       │             │
                      GND           GND
```

Example **~fc 150–180 Hz** (approximate, equal R):/R1 = R2 = 10 kΩ, C1 = C2 = 100 nF.

More parts, more roll-off above cutoff. Start with **1-pole**; add second pole only if needed on-suit.

---

## 4. Bias, levels, and OFFSET

| Item | Value |
|------|--------|
| Amp DC bias (typical Adafruit electret) | **~1.25 V** |
| ESP32 ADC full scale (11 dB atten) | **0–3.3 V** → 12-bit **0–4095** |
| Firmware `OFFSET` | `1.25/3.3 * 4095 ≈ **1551**` (see SYSTEM.md; tune on bench) |

The RC **does not remove DC bias** — it only rolls off AC above fc. Bias still sits at ~1.25 V so `analogRead - OFFSET` is a proper AC magnitude.

**Do not** AC-couple (series capacitor only) into A0 unless you add a separate mid-rail bias network; the current firmware assumes a **stable mid bias**.

---

## 5. Wiring notes

1. **GND** of amp, RC, and ESP32 must be common.  
2. Place **C_lp within a few mm of A0** if possible (star to GND pour).  
3. Keep mic leads short; twisted pair amp→ESP helps in the suit.  
4. Amp **VCC = 3.3 V** (not 5 V) so the output cannot exceed the ESP32 ADC absolute max.  
5. GPIO36 is **input-only** (ADC1) — good for mic; avoids output conflicts.

```text
[Electret + Adafruit-style preamp]
        │ OUT (~1.25 V bias, ≤3.3 Vpp region)
        │
     R_lp 10k
        │
        ├──── C_lp 100nF ── GND
        │
     ESP32 A0
```

---

## 6. BOM (default)

| Qty | Part | Spec |
|-----|------|------|
| 1 | R_lp | 10 kΩ, 1/8 W or 1/4 W, 1% or 5% |
| 1 | C_lp | 100 nF, ≥16 V, X7R or film |
| — | Mic amp | Existing Adafruit-style electret board |

---

## 7. Bench check

1. Power Tail, Serial monitor mic / `STAT Mic:`.  
2. Silence: level near **0** after deadband (not a large constant).  
3. Speak / clap: Mic climbs; modes 0–2 react.  
4. If always “hot”: lower `G` / `S` / `A`, or verify OFFSET (~1551).  
5. If thin / no bass thump: try slightly higher fc table row — but stay **≤ ~200 Hz** for the 500 Hz tick.  
6. Scope (optional): amp OUT vs A0; A0 should look smoother, less RF hash.

---

## 8. Firmware relationship (do not double-count)

| Stage | Where | What |
|-------|--------|------|
| Hardware LPF | RC at A0 | ≤ ~200 Hz |
| Sample | `readMicLevel` path, 2 ms tick | 500 Hz |
| Deadband | firmware | kills residual noise |
| Gain `A` / Sensitivity `S` | firmware + BLE | scale |
| EMA / attack-release | firmware | envelope |
| Gate `G` | firmware | wake modes 0–1 |

Hardware filter ≠ software gate. Tune **RC for noise/aliasing**, tune **G/A/S for show behaviour**.

---

## 9. Out of scope

- Op-amp active filters (unnecessary for envelope lighting)  
- Digital IIR in place of RC (possible later; RC still recommended for RF)  
- Head/PAWB mic hardware (mic lives on **Tail only**)

---

*Default: **10 kΩ + 100 nF** → **fc ≈ 159 Hz**. Match SYSTEM.md mic section when either changes.*
