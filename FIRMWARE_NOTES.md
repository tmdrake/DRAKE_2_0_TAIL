# Firmware Team Notes – Modes & Architecture

**Date:** July 2026  
**Status:** Modes 0–10 are implemented on **all three boards**, all **non-blocking**.

---

## What shipped

### Mode implementation

Every board uses the same conceptual map:

| Mode | Implementation idea |
|------|---------------------|
| 0–1 | Sound loops driven by mic (Tail local ADC; Head/PAWB remote mic) |
| 2 | VU bar from mic level |
| 3 | Rainbow HSV chase |
| 4 | Comet head + fade trail |
| 5 | Breathing brightness + hue drift |
| 6 | Fire / heat flicker |
| 7 | Sparkle decay + random sparks |
| 8 | Traveling wave (triangle wave on Head/PAWB to save cycles) |
| 9 | Solid color (default purple-blue) |
| 10 | All off |

**Non-blocking rule:** each mode function returns immediately unless `millis() - prev >= interval`. Never call `delay()` inside a mode used from `loop()`.

### Head-specific

- File: `DRAKE_2_0_HEAD/New_Modes.ino`
- `mode_selector()` in that file; main `sound_detect()` routes:
  - mode 2–10 → continuous `mode_selector`
  - mode 0–1 → sound-reactive or idle `fading()`
- After each visual frame: `applyEyeDim()` so CDS still controls pixels 0–3
- Do **not** use `Other_modes.ino` from the main loop (legacy blocking code)

### Tail-specific

- `New_Modes.ino` + `animSpeed` via `scaledInterval()`
- Mic: `readMicLevel()` shared by gate and `sampleaudio()`

### PAWB-specific

- 5-pixel simplified versions; triangle wave instead of `sin()` where helpful
- Modes arrive as ASK `M0`–`M9` / `MA`

---

## Ideas already in code

| Idea | Where |
|------|--------|
| Encrypted ESP-NOW mic/cmds/sensors | Tail + Head `EspNowCom.ino` |
| Mic gain + gate + EMA | Tail main + BLE `G`/`A` |
| Fan auto/on/off + threshold | Head + BLE `F*`/`FT*` |
| CDS eye dim threshold + % | Head + BLE `I*`/`D*` |
| Live STAT to app | Tail `pushLiveStatus()` |

---

## Ideas not yet coded (backlog)

1. `C<r>,<g>,<b>` solid color set from app → Tail + Head + PAWB  
2. Mic silence calibration (replace fixed OFFSET 1600)  
3. Per-mode speed on Head (Head currently fixed intervals)  
4. Archive `Other_modes.ino`  

---

## Test plan (firmware)

1. Flash Head, Tail (MACs set), PAWB.  
2. BLE or serial: `M0`…`M10` — all three regions should match style.  
3. Cover/uncover Head CDS — eyes dim only, spikes keep mode.  
4. Speak into Tail mic on M0/M1 — Head + paws react.  
5. `F1` / `F0` / `FT80` — fan behaviour.  
6. `G` / `A` / `S` — mic meter and wake behaviour.

---

*Primary references: SYSTEM.md, ESPNOW.md, SETTINGS.md*
