# TMDrake Companion App – Interface Contract

**Version:** 1.2  
**Date:** July 2026  
**Hardware:** ESP32 Tail (`TMDrake_tail`) via BLE Nordic UART Service  
**Architecture:** [SYSTEM.md](SYSTEM.md) · ESP-NOW: [ESPNOW.md](ESPNOW.md)

---

## 1. Branding

| Element | Spec |
|---------|------|
| App Name | **TMDrake** or **Drake Control** |
| Primary | Deep purple `#4A1C6B`–`#7B2D8E` |
| Accent | Cyan `#00C2FF` |
| Background | Near-black `#0D0B14` |

---

## 2. BLE (NUS)

| Role | UUID |
|------|------|
| Service | `6E400001-B5A3-F393-E0A9-E50E24DCCA9E` |
| RX (app→suit) | `6E400002-B5A3-F393-E0A9-E50E24DCCA9E` |
| TX (suit→app) | `6E400003-B5A3-F393-E0A9-E50E24DCCA9E` |

Device name: `TMDrake_tail` · Plugin: `flutter_blue_plus` · Filter by service UUID.

---

## 3. Command protocol (App → Tail RX)

UTF-8 strings, no required terminator.

### Lighting / suit (Tail + forward)

| Command | Format | Description |
|---------|--------|-------------|
| Mode | `M<0-10>` | Lighting mode |
| Brightness | `B<0-100>` | Master brightness % |
| Speed | `V<0-100>` | Animation speed |
| Sensitivity | `S<n>` | Mic sensitivity |
| Sound | `E` / `e` | Sound detect on/off |
| Flash | `L` | Flash |
| Resync | `R` | Reset animations |
| Reboot | `Z` | Reboot Tail |
| Status | `?` | Help + dump |

### Head settings (Settings menu) — **new**

These are forwarded Tail → Head (ESP-NOW / UDP). **Not** sent to PAWB.

| Command | Format | Range | Description |
|---------|--------|-------|-------------|
| Fan mode | `F0` | — | Force fan **OFF** |
| Fan mode | `F1` | — | Force fan **ON** |
| Fan mode | `F2` | — | Fan **AUTO** (by temperature) |
| Fan threshold | `FT<n>` | 50–120 | AUTO fan ON when Head temp **> n °F** (default **85**) |
| CDS threshold | `I<n>` | 0–1023 | When CDS reading **≥ n**, dim eyes (default **500**) |
| Eye dim level | `D<n>` | 1–100 | Eye brightness **%** while dimmed (default **10**) |

#### Settings UI recommendations

- **Fan section**
  - Segmented control / radio: Off · On · Auto
  - Slider or stepper: “Fan on above ___ °F” (only enabled in Auto)
  - Show live `HeadT` from `STAT`
- **Eyes / ambient light section**
  - Slider: “Dim eyes when light sensor ≥ ___” (`I`)
  - Slider: “Dimmed eye brightness %” (`D`)
  - Show live `HeadB` (CDS raw) from `STAT`
  - Short help text: *CDS on the Head dims eye LEDs in bright ambient light.*

---

## 4. CDS sensor & eye brightness (hardware behaviour)

**Document this clearly in the app Settings help / about copy.**

| Item | Detail |
|------|--------|
| Sensor | **CDS photocell** (light-dependent resistor) on **Head A0** |
| Eyes | NeoPixel indices **0–3** on the Head strip |
| Spikes | Indices 4+ (not dimmed by CDS) |
| Rule | `if (CDS_reading >= threshold) dim_eyes = true` |
| Dimmed | Eyes drawn at `D/100` brightness (default 10%) |
| Not dimmed | Eyes at full relative brightness (100%) |
| Live value | `STAT` field **`HeadB`** = latest CDS reading |

Typical use: in bright rooms / outdoor sun, eyes automatically dim so they don’t look blown-out; in darker spaces they stay bright.

Threshold and dim % are tunable so different suits / CDS wiring can be calibrated without reflashing.

---

## 5. Fan behaviour (Head)

| Mode | Command | Behaviour |
|------|---------|-----------|
| Off | `F0` | Fan always off |
| On | `F1` | Fan always on |
| Auto | `F2` | Fan on when `HeadT > FT` threshold °F |

Live temperature: `STAT` field **`HeadT`** (°F).

---

## 6. Status / telemetry (TX notify)

```text
STAT M:3 B:80 V:50 S:75 E:1 Mic:1423 HeadB:512 HeadT:36.5
```

| Token | Meaning |
|-------|--------|
| `M` `B` `V` `S` `E` | Mode, brightness, speed, sensitivity, sound |
| `Mic` | Tail mic level |
| `HeadB` | **CDS light sensor** raw reading |
| `HeadT` | Head temperature °F |

---

## 7. App structure

1. **Connect**
2. **Control** — modes, B/V/S, flash, resync
3. **Status** — Mic, HeadT, HeadB meters
4. **Settings**
   - Fan: mode + threshold
   - Eyes / CDS: threshold + dim %
   - About / branding

---

## 8. Mode icons (0–10)

Unchanged — see prior contract (Sound Phase … Off).

---

## 9. Versioning

- v1.2 adds `F*`, `FT*`, `I*`, `D*`.
- Older firmware ignores unknown commands; app should tolerate missing behaviour.

---

*Firmware is source of truth for protocol.*
