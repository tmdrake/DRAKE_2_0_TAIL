# TMDrake App Team – Modes & Settings Brief

**Date:** July 2026 · **Contract version:** 1.4  
**Read with:** [APP_INTERFACE.md](APP_INTERFACE.md) · [SETTINGS.md](SETTINGS.md)

---

## Suit behaviour you can rely on

One BLE connection to **`TMDrake_tail`**. Tail fans out to **Head** (ESP-NOW) and **PAWB claws** (ASK).

**Modes 0–10 run on the entire suit** (Tail + Head + claws). All non-blocking on firmware.

| ID | Name | UI hint |
|----|------|---------|
| 0 | Sound Phase | Soft color-phase sound reactive |
| 1 | Sound Distinct | Hard color steps sound reactive |
| 2 | VU Meter | Bar graph |
| 3 | Rainbow Chase | Flowing rainbow |
| 4 | Comet | Bright head + trail |
| 5 | Breathing | Pulse + hue |
| 6 | Fire | Heat flicker |
| 7 | Sparkle | Twinkles |
| 8 | Wave | Traveling wave |
| 9 | Solid | Static color (picker + `C`) |
| 10 | Off | Blackout |

- **0–1:** need mic activity (and Sound reactive ON).  
- **2–10:** continuous once selected.  
- Command: `M0` … `M10`.

---

## Commands to implement

### Control screen
| Control | Command |
|---------|---------|
| Mode grid | `M<0-10>` |
| Brightness | `B<0-100>` |
| Speed | `V<0-100>` |
| Flash | `L` |
| Resync | `R` |
| Solid color picker | `C<r>,<g>,<b>` then show mode 9 |

`C150,0,255` sets RGB, switches suit to **Solid (9)**, syncs Head + paws.

### Settings – Sound
| Control | Command | Notes |
|---------|---------|-------|
| Sound reactive | `E` / `e` | |
| Gain % | `A<50-300>` | Default 100 |
| Sensitivity | `S<n>` | Default 75; UI 0–500 |
| Gate | `G<n>` | Wake threshold; default 100 |
| Mic meter | `STAT Mic:` | Live |

### Settings – Fan (Head)
| Control | Command |
|---------|---------|
| Off / On / Auto | `F0` / `F1` / `F2` |
| Threshold °F | `FT<50-120>` |
| Temp meter | `STAT HeadT:` |

### Settings – Eyes / light (Head)
| Control | Command |
|---------|---------|
| Dim when light ≥ | `I<0-1023>` |
| Dimmed eye % | `D<1-100>` |
| Light meter | `STAT HeadB:` |

CDS dims **eye LEDs only** in bright ambient light.

---

## Live telemetry

```text
STAT M:3 B:80 V:50 S:75 G:100 A:100 E:1 Mic:1423 HeadB:512 HeadT:86.2
```

Sync UI from `STAT` after connect (~2 Hz).

---

## Suggested UI structure

1. **Connect** – scan NUS UUID / `TMDrake_tail`  
2. **Control** – mode icons 0–10, B, V, color chip (Solid), L, R  
3. **Status** – Mic, HeadT, HeadB  
4. **Settings** – Sound (E,A,S,G), Fan, Eyes/CDS, About  

### Mode icon set (required)
Eleven mode icons + Flash, Resync, Bluetooth, Settings, dragon mark. Dark purple / cyan theme.

---

## What firmware already does (don’t re-solve in app)

- Mode fan-out to Head + paws  
- Mic gain/gate/EMA  
- Fan + CDS on Head  
- Encrypted ESP-NOW Tail↔Head  

App only talks BLE to Tail.

---

*Firmware source of truth: Tail repo. Questions → SYSTEM.md / FIRMWARE_NOTES.md.*
