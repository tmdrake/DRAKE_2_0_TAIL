# Future design – FFT / 16-band spectrum (not in firmware yet)

**Status:** Design notes for a **later** Drake revision (Florida bench / Drake_3.0-era idea).  
**Current ship path:** AGC mic + RC LPF + envelope (modes 0–2). Do **not** treat this doc as implemented.

Related: [MIC_HARDWARE.md](MIC_HARDWARE.md) · [SYSTEM.md](SYSTEM.md) · [APP_INTERFACE.md](APP_INTERFACE.md)

---

## 1. Goal

Music-aware lighting on the Tail:

- Sample the **AGC mic** at a rate useful for **music** (not CD 44.1 kHz media rate).
- Run **FFT** → fold into a **~16-band** spectrum.
- **Snapshot** bands over **BLE** (app / BT UI).
- **Select which band(s)** drive “fire” (lights react to kick, bass drop, snare region, etc.).

Human hearing top end is often discussed ~15–20 kHz; **music energy that drives a suit** is mostly **bass + low-mids**. You do not need 44.1 kHz capture for that.

---

## 2. Sampling target (design intent)

| Idea | Choice | Rationale |
|------|--------|-----------|
| Media rate | 44.1 kHz | Overkill for lighting FFT on ESP32 |
| **Useful capture** | **~15–20 kHz sample rate** | Nyquist ~7.5–10 kHz content — enough for bass, body, and some presence |
| Alternate | **8–12 kHz** sample rate | Still fine for **deep bass / drops**; cheaper CPU |
| FFT size | **256** (or 512) | Fits ESP32; update spectrum UI at ~20–40 Hz |
| Bands | **16** | Readable in app; maps cleanly to log-spaced groups |

ESP32 continuous **ADC1** is comfortable in the **tens of kHz** when radio is alive — see engineering notes in chat history / IDF continuous ADC docs.

---

## 3. Analog front-end conflict (important)

| Path | Analog bandwidth | Role |
|------|------------------|------|
| **Today** | **~10 kΩ + 0.1 µF → fc ≈ 159 Hz** | Envelope only |
| **Spectrum** | Needs **much higher** fc (or no heavy LPF) | Else bands above ~160 Hz are dead |

**Future hardware options:**

1. **Higher-fc RC** for dual use (e.g. fc ~5–8 kHz anti-alias for 16–20 kHz sample rate), rely more on software filtering for envelope; or  
2. **Two paths** — envelope tap after deep LPF, spectrum tap before / lighter LPF; or  
3. **Mode switch** — spectrum mode changes sample rate + accepts more RF (short leads, shielding).

AGC remains in front either way (same mic module).

---

## 4. AGC + spectrum behaviour

Built-in **AGC** will:

- Help quiet rooms and loud drops both show *something* in the bands.  
- **Squash** true peak dynamics — “band 0 bass” is relative energy after AGC, not calibrated dB SPL.

For “fire on bass drop,” use:

- **Relative** band energy vs recent average (onset / flux), not absolute thresholds alone.  
- Optional slow baseline per band so AGC breathing doesn’t false-trigger.

---

## 5. 16-band layout (example)

Log-ish grouping (tune on music later):

| Band | Approx focus |
|------|----------------|
| 0–1 | Sub / deep bass (drops) |
| 2–3 | Bass body |
| 4–6 | Low mid |
| 7–9 | Mid |
| 10–12 | Upper mid |
| 13–15 | Presence / air (if analog path allows) |

With today’s **~159 Hz** LPF, only the lowest bands would be meaningful until the analog path is widened.

---

## 6. BLE / app interface (future contract sketch)

| Feature | Direction | Sketch |
|---------|-----------|--------|
| Spectrum snapshot | Tail → app | e.g. `SPEC:v0,v1,…,v15` or binary notify | 
| Rate | Tail → app | ~10–20 Hz snapshots while spectrum UI open (not full audio stream) |
| Band select | App → Tail | e.g. `SB0`…`SB15` or bitmask `SM<hex>` — which bands trigger fire |
| Threshold / sensitivity | App → Tail | Per-band or global, reuse spirit of `G`/`S` |
| Mode | App → Tail | New mode id or flag: spectrum-driven pattern vs classic envelope |

Keep **phone optional**: last band-select and mode stay in **NVS**; suit still runs if BLE drops.

Exact command letters TBD when implemented — update [APP_INTERFACE.md](APP_INTERFACE.md) then.

---

## 7. Firmware architecture sketch

```text
ADC1 continuous @ ~16–20 kHz (or 8–12 kHz)
    → ring buffer
    → FFT task (prefer core 1; leave radio quieter on core 0)
    → 16 band magnitudes
    → onset vs baseline → “fire” events
    → existing LED modes / new spectrum modes
    → optional SPEC snapshot over BLE when app subscribed
```

Envelope path can remain the default for modes 0–2 with little CPU cost.

---

## 8. Why this is “future”

- Needs **analog rethink** vs current ~160 Hz LPF.  
- CPU + BLE bandwidth budgeting on a hub that already does ESP-NOW + LEDs + NimBLE.  
- App UI for 16 bars + band picks.  
- AGC-aware triggering logic.

Worth doing when the suit is on the bench for a deliberate audio upgrade — not a drive-by patch on the current envelope ship.

---

## 9. Success criteria (when built)

- [ ] 16 bands visible in BT/app snapshot  
- [ ] User can select which band(s) fire lights  
- [ ] Bass/drop content clearly drives selected low bands on real music  
- [ ] Suit still runs without phone  
- [ ] Documented commands in APP_INTERFACE + AGC notes unchanged in MIC_HARDWARE  

---

*Dream feature for music-reactive Drake — capture here so Florida-you and future-you remember the intent.*
