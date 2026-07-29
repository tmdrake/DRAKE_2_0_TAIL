# TMDrake Companion App – Interface Contract

**Version:** 1.4  
**Date:** July 2026  
**App team brief:** [APP_TEAM.md](APP_TEAM.md) · **Settings:** [SETTINGS.md](SETTINGS.md)

---

## BLE NUS

Device: `TMDrake_tail`  
Service `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`  
RX `…002…` write · TX `…003…` notify  
Plugin: `flutter_blue_plus`

---

## Modes 0–10 (full suit)

Selecting a mode on the app updates **Tail + Head + PAWB**.

| ID | Name | Command |
|----|------|---------|
| 0 | Sound Phase | `M0` |
| 1 | Sound Distinct | `M1` |
| 2 | VU Meter | `M2` |
| 3 | Rainbow Chase | `M3` |
| 4 | Comet | `M4` |
| 5 | Breathing | `M5` |
| 6 | Fire | `M6` |
| 7 | Sparkle | `M7` |
| 8 | Wave | `M8` |
| 9 | Solid | `M9` |
| 10 | Off | `M10` |

Solid color: `C<r>,<g>,<b>` (0–255) → sets color and mode 9 on the suit.

---

## Full command list

| Cmd | Purpose |
|-----|---------|
| `M` `B` `V` | Mode, brightness, speed |
| `S` `G` `A` | Mic sensitivity, gate, gain % |
| `E` / `e` | Sound on / off |
| `C<r>,<g>,<b>` | Solid RGB + mode 9 |
| `L` `R` `Z` | Flash, resync, reboot |
| `F0` `F1` `F2` `FT<n>` | Fan off/on/auto, threshold °F |
| `I<n>` `D<n>` | CDS threshold, eye dim % |
| `?` | Status dump |

---

## STAT line

```text
STAT M:3 B:80 V:50 S:75 G:100 A:100 E:1 Mic:1423 HeadB:512 HeadT:86.2
```

---

## Screens

Connect · Control (modes + B/V + color) · Status · Settings (Sound, Fan, Eyes)

See **APP_TEAM.md** for UI layout and icon list.

---

*v1.4: full mode parity + solid color C.*
