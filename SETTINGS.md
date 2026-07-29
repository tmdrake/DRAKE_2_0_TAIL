# TMDrake App – Settings Breakout (App Team)

**Firmware:** Tail with mic gate/gain/EMA (July 2026)  
**Also read:** [APP_INTERFACE.md](APP_INTERFACE.md)

Hand this document to the app team as the settings + mic UX contract.

---

## Mic pipeline (firmware)

```text
delta  = max(0, analogRead(A0) - 1600)
level  = delta * (Gain%/100) + Sensitivity
smooth = EMA(level)              // reduces jitter
STAT Mic: = smooth (peak-tracked)

Sound modes wake when: smooth > Gate  (and Sound Reactive is ON)
```

| Control | Cmd | Default | UI range | Role |
|---------|-----|---------|----------|------|
| Sensitivity | `S<n>` | 75 | 0–500 (adv wider) | Additive boost after gain |
| **Gate** | `G<n>` | 100 | 20–800 | How loud before sound modes wake |
| **Gain %** | `A<n>` | 100 | 50–300 | Multiplier on (raw − bias) |
| Sound on/off | `E` / `e` | on | toggle | Master enable for mic modes |

**Tuning tips for UI copy**
- Meter not moving → raise **Gain** or **Sensitivity**
- Always reacting to room noise → raise **Gate** or lower Gain
- Quiet speech should wake LEDs → lower Gate slightly, raise Gain
- Con floor / loud DJ → higher Gate, lower Gain

**Presets (suggested)**
| Preset | S | G | A |
|--------|---|---|---|
| Quiet / suit on | 100 | 60 | 120 |
| Normal | 75 | 100 | 100 |
| Loud floor | 40 | 180 | 80 |

---

## Settings screen layout

```text
Settings
├── Sound
│   ├── [x] Sound reactive          E / e
│   ├── Gain          ====o====     A  (50–300%)
│   ├── Sensitivity   ====o====     S
│   ├── Gate          ====o====     G  (“wake threshold”)
│   └── Mic meter     [########]    STAT Mic:
├── Fan (Head)
│   ├── Off | On | Auto             F0 / F1 / F2
│   ├── On above [ 85 ] °F          FT
│   └── Temp                        STAT HeadT:
├── Eyes / ambient (Head)
│   ├── Dim when light ≥ [ 500 ]    I
│   ├── Dimmed eye brightness %     D
│   └── Light sensor                STAT HeadB:
└── System
    ├── Reboot Tail                 Z
    └── About
```

Main **Control** screen still owns: Mode `M`, Brightness `B`, Speed `V`, Flash `L`, Resync `R`.

---

## Full command cheat sheet

### Sound / lighting (Tail)
| Cmd | Meaning |
|-----|---------|
| `M0`–`M10` | Mode |
| `B0`–`B100` | Master brightness |
| `V0`–`V100` | Anim speed |
| `S<n>` | Sensitivity |
| `G<n>` | Gate threshold |
| `A<n>` | Mic gain % |
| `E` / `e` | Sound on / off |
| `L` `R` `Z` | Flash / Resync / Reboot |
| `?` | Status dump |

### Head (via Tail)
| Cmd | Meaning |
|-----|---------|
| `F0` `F1` `F2` | Fan off / on / auto |
| `FT<n>` | Fan threshold °F |
| `I<n>` | CDS dim threshold |
| `D<n>` | Eye dim % when CDS active |

---

## Live `STAT` line (~2 Hz)

```text
STAT M:3 B:80 V:50 S:75 G:100 A:100 E:1 Mic:1423 HeadB:512 HeadT:86.2
```

Parse `G:` and `A:` to sync Settings sliders after reconnect.

---

## App UX requirements (Sound)

1. Show **Mic meter** while adjusting S / G / A.  
2. Debounce slider writes ~100–150 ms.  
3. Labels: Gain = “how hard the mic pushes”, Gate = “how loud to wake the lights”.  
4. Optional preset chips: Quiet / Normal / Loud.  
5. If firmware older and ignores `G`/`A`, still send `S`; degrade gracefully.

---

*Firmware is source of truth. Update this file when commands change.*
