# TMDrake App – Settings Breakout + Mic Guidance

**For:** App team (Flutter / Android)  
**Firmware source of truth:** [APP_INTERFACE.md](APP_INTERFACE.md)  
**Date:** July 2026

---

## 1. How the mic works today (Tail)

```text
raw   = analogRead(A0)           // ESP32 ADC
level = raw - OFFSET + sensitivity
OFFSET = 1600                    // fixed bias / mid-rail of the mic preamp
sensitivity = user setting (S)   // default 75, stored in NVS
```

- **Sound modes (0–2)** turn “active” when `level > ~100` (gate is currently hard-coded).
- Active sound mode stays on for **10 seconds** after last loud sample, then returns to idle fade.
- Live meter in the app: `STAT` field **`Mic:`** (peak-ish value Tail reports).

### What `S` (sensitivity) actually does

It is an **additive offset** after the bias removal — not a true gain multiplier.

| Higher `S` | Lower `S` |
|------------|-----------|
| Quiet sounds look “louder” | Needs more real volume to move LEDs |
| Easier to trigger sound modes | Fewer false triggers from noise |
| Can floor-clip less if bias is high | Can sit at 0 if bias is underestimated |

**Default 75** is a reasonable starting point for the current OFFSET=1600 board.

---

## 2. Recommendations to improve mic feel

### A. App UX (do these first — no firmware change)

1. **Live Mic meter on Control + Settings**  
   Bind to `STAT Mic:`. User should see the bar jump while talking while moving the sensitivity slider.

2. **Sensitivity slider with sensible range**  
   - UI range: **0–500** for normal use (firmware allows much wider; don’t expose −1500…4000 in the main slider).  
   - Optional “Advanced” expand for full range.  
   - Debounce writes ~150 ms → send `S<n>`.

3. **Presets**  
   - Quiet room / suit on: `S150`–`S250`  
   - Normal con floor: `S50`–`S100`  
   - Loud DJ / parade: `S0`–`S40`  
   Label them in the UI; still write the numeric `S` command.

4. **Sound enable toggle**  
   Keep `E` / `e` obvious. When off, modes 0–2 won’t react; visual modes 3–10 still run.

5. **Help text**  
   > “Sensitivity raises or lowers how strongly the tail mic drives the lights. Watch the Mic meter while you speak.”

### B. Firmware improvements (recommended next, not required for app v1)

| Improvement | Why |
|-------------|-----|
| **Separate gate threshold** (e.g. `G<n>`) | Today gate is fixed ~100 and ignores `sensitivity` on the trigger path in `sound_detect()`. Making gate app-tunable stops “always on” vs “never triggers”. |
| **Use sensitivity on the gate path** | `sound_detect()` currently does `analogRead - OFFSET` **without** `+ sensitivity`. Align it with `sampleaudio()`. |
| **True gain** `level = (raw - OFFSET) * gain / 100` | Additive `S` is crude; a 50–200% gain slider feels more natural. |
| **Noise floor / EMA** | Smooth mic with a short exponential average; subtract a slow noise floor so ambient hum doesn’t light the suit. |
| **Attack / release** | Fast rise, slower fall on the displayed/sent level → smoother Head/PAWB animations. |
| **Auto-calibrate bias** | On boot, sample quiet ADC average → set OFFSET (or an app “Calibrate silence” button). |
| **Clamp + curve** | Soft-knee compression so shouts don’t flatline every pixel. |

**Highest value, smallest change:** fix the gate path to include `sensitivity`, and/or expose gate as `G<n>`.

### C. Hardware (bench)

- Confirm mic preamp bias near mid-rail so OFFSET 1600 is honest.  
- If raw ADC idles far from 1600, either change OFFSET in firmware or run auto-cal.  
- Shield the mic line from the ASK transmitter / LED power to reduce false triggers.

---

## 3. Settings breakout for the app team

Group the Settings screen like this. All commands go to Tail BLE RX as UTF-8.

### 3.1 Sound (Tail mic)

| UI control | Command | Range (UI) | Default | Notes |
|------------|---------|------------|---------|-------|
| Sound reactive | `E` / `e` | on/off | on | Disables mic-driven modes 0–2 |
| Sensitivity | `S<n>` | 0–500 (adv: wider) | 75 | Additive offset after bias |
| Mic meter | — | read-only | — | From `STAT Mic:` |

**Future (not in firmware yet — reserve UI space):**
| Gate threshold | `G<n>` | 20–500 | 100 | When mic exceeds gate → enter sound mode |
| Mic gain % | `SG<n>` | 50–200 | 100 | True multiplier (if implemented) |

### 3.2 Lighting (Tail / whole suit)

| UI control | Command | Range | Default | Notes |
|------------|---------|-------|---------|-------|
| Mode | `M<0-10>` | 0–10 | last saved | Main Control screen |
| Master brightness | `B<n>` | 0–100 | 80 | Tail NeoPixels |
| Animation speed | `V<n>` | 0–100 | 50 | 50 = normal |
| Flash | `L` | — | — | Quick action |
| Resync | `R` | — | — | Reset anim state |

### 3.3 Head – Fan

| UI control | Command | Range | Default | Notes |
|------------|---------|-------|---------|-------|
| Fan mode | `F0` / `F1` / `F2` | Off / On / Auto | Auto (`F2`) | Segmented control |
| Fan on above °F | `FT<n>` | 50–120 | 85 | Only enabled in Auto |
| Temp meter | — | read-only | — | `STAT HeadT:` |

### 3.4 Head – Eyes / ambient light (CDS)

| UI control | Command | Range | Default | Notes |
|------------|---------|-------|---------|-------|
| Dim when light ≥ | `I<n>` | 0–1023 | 500 | CDS threshold |
| Dimmed eye brightness % | `D<n>` | 1–100 | 10 | Eyes only (pixels 0–3) |
| Light meter | — | read-only | — | `STAT HeadB:` |

**Help copy for app:**
> The Head has a CDS light sensor. In bright environments the eye LEDs automatically dim so they don’t look washed out. Adjust the threshold and dim level to match your suit.

### 3.5 System

| UI control | Command | Notes |
|------------|---------|-------|
| Reboot Tail | `Z` | Confirm dialog |
| Full status | `?` | Debug / support |
| Connection | — | BLE device `TMDrake_tail` |

---

## 4. Live telemetry (`STAT` line)

Pushed ~2 Hz while connected:

```text
STAT M:3 B:80 V:50 S:75 E:1 Mic:1423 HeadB:512 HeadT:86.2
```

| Field | Use in UI |
|-------|-----------|
| `Mic` | Mic meter (Sound settings + Control) |
| `HeadT` | Fan section |
| `HeadB` | CDS / eyes section |
| `M B V S E` | Sync sliders if app reconnected |

---

## 5. Suggested Settings screen layout

```text
Settings
├── Sound
│   ├── [x] Sound reactive     E/e
│   ├── Sensitivity  ====o===  S
│   └── Mic  [########----]    STAT Mic
├── Fan (Head)
│   ├── ( ) Off  ( ) On  (•) Auto   F0/F1/F2
│   ├── On above [ 85 ] °F          FT
│   └── Temp  86.2 °F               STAT HeadT
├── Eyes / ambient light (Head)
│   ├── Dim when sensor ≥ [ 500 ]   I
│   ├── Dimmed brightness [ 10 ] %  D
│   └── Light sensor [###-----]     STAT HeadB
└── System
    ├── Reboot tail                 Z
    └── About / version
```

---

## 6. Mic tuning quick guide (for users in-app)

1. Put suit on / mic in normal position.  
2. Open Settings → Sound; watch **Mic** meter.  
3. Stay silent — meter should stay low.  
4. Speak at normal volume — meter should move clearly.  
5. If always high → lower Sensitivity.  
6. If never moves → raise Sensitivity (or check mic wiring / OFFSET on firmware).  
7. On a loud con floor, lower Sensitivity so ambient music doesn’t own the LEDs.

---

*App team: implement against this table + APP_INTERFACE.md. Firmware changes for gain/gate are backlog unless requested.*
