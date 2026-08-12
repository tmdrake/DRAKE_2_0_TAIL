# Tail mic – analog path & lowpass filter

**Board:** ESP32 Tail (`A0` / GPIO36)  
**Amp:** Adafruit-style electret mic + preamp (AGC / normalizing), **~1.25 V DC bias**, output within **0–3.3 V**

---

## Software vs hardware (do not mix these up)

| Layer | What we know |
|-------|----------------|
| **Software** | Envelope path ticks at **500 Hz** (2 ms). Nyquist = **250 Hz** → useful content should stay **under ~200 Hz** so the ADC isn’t fighting aliases. That **~200 Hz** number is a **sample-rate / design budget**, not a measured RC part number. |
| **Hardware** | An **RC lowpass was built** on the suit between mic amp and A0. **Exact R and C values are not recorded in this repo** (not remembered off the top of the head). Treat the table below as **recommended / rebuild** values, not as “what is soldered today.” |

When the physical board is on the bench: read the markings or measure R with a meter and C with an LCR / known-R time-constant check, then **write the real values into §2** of this file.

---

## 1. Why hardware LPF at all?

- Soften RF hash from WiFi / BLE / ESP-NOW on mic leads  
- Limit energy above the software sample budget (~200–250 Hz)  
- Give the ESP32 ADC a calmer source  

Firmware still does deadband, gain `A`, sensitivity `S`, EMA, and gate `G`. Hardware is the first stage only.

---

## 2. Circuit topology (what was / should be on the board)

Passive 1-pole between **mic amp OUT** and **ESP32 A0**:

```text
                    R_lp
  Mic amp OUT ───/\/\/\/───┬──────── ESP32 A0 (ADC1_CH0 / GPIO36)
   (~1.25 V bias)          │
                        C_lp
                           │
                          GND  (common with ESP32 GND)
```

### On-suit values (fill in when known)

| Part | On-suit (actual) | Notes |
|------|------------------|--------|
| **R_lp** | **TBD — measure** | Series from amp OUT |
| **C_lp** | **TBD — measure** | To GND near A0 |
| **fc** | **TBD** | `1 / (2π R C)` once R,C known |

### Recommended if rebuilding or verifying (targets ~200 Hz software budget)

| R_lp | C_lp | fc (approx) |
|------|------|-------------|
| **8.2 kΩ** | **100 nF** | **~194 Hz** |
| 10 kΩ | 82 nF | **~194 Hz** |
| 10 kΩ | 100 nF | **~159 Hz** (more aggressive) |
| 7.5 kΩ | 100 nF | **~212 Hz** |

```text
fc = 1 / (2 · π · R · C)
```

These are **suggestions** aligned with the firmware sample rate — **not** a claim that the suit was built with 8.2k/100n.

**Cap type:** ceramic (X7R/C0G) or film. Amp **3.3 V** supply so OUT cannot exceed ADC max.

---

## 3. Optional 2-pole

Only if a single pole still leaves RF junk:

```text
  Amp OUT ── R1 ──┬── R2 ──┬── A0
                  C1       C2
                  GND      GND
```

Same rule: document real values here when used.

---

## 4. Bias & OFFSET (firmware)

| Item | Value |
|------|--------|
| Typical amp bias | **~1.25 V** |
| ADC 0–3.3 V → 12-bit | **0–4095** |
| `OFFSET` | `1.25/3.3 * 4095 ≈ **1551**` (tune on bench) |

RC does **not** remove DC bias. Do not AC-couple into A0 without a mid-rail bias network.

---

## 5. How to recover the real RC on the bench

1. Power **off**.  
2. Read resistor code / measure **R_lp** in-circuit or lifted.  
3. Read capacitor marking or measure **C_lp**.  
4. Compute `fc = 1/(2πRC)` and paste into the **On-suit** table above.  
5. Optional: scope amp OUT vs A0 while playing a tone sweep — −3 dB point ≈ hardware fc.

---

## 6. Wiring reminders

- Common GND: amp, RC, ESP32  
- **C_lp close to A0**  
- Short mic leads  
- GPIO36 input-only — correct for mic  

---

## 7. Firmware relationship

| Stage | Rate / role |
|-------|-------------|
| Hardware RC | Unknown until measured; topology as above |
| Sample tick | **500 Hz** → software Nyquist **250 Hz**, design interest **~200 Hz** |
| Deadband / A / S / EMA / G | Show tuning in software |

---

*Hardware filter exists on the suit; part values TBD. Software ~200 Hz is Nyquist-side budget only.*
