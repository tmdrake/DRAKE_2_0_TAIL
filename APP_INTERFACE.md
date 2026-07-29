# TMDrake Companion App – Interface Contract

**Version:** 1.3  
**Date:** July 2026  
**Settings detail:** [SETTINGS.md](SETTINGS.md)  
**Architecture:** [SYSTEM.md](SYSTEM.md)

---

## BLE NUS

| Role | UUID |
|------|------|
| Service | `6E400001-B5A3-F393-E0A9-E50E24DCCA9E` |
| RX | `6E400002-B5A3-F393-E0A9-E50E24DCCA9E` |
| TX | `6E400003-B5A3-F393-E0A9-E50E24DCCA9E` |

Device: `TMDrake_tail` · Plugin: `flutter_blue_plus`

---

## Commands (App → RX)

### Core
| Cmd | Format | Notes |
|-----|--------|-------|
| Mode | `M<0-10>` | |
| Brightness | `B<0-100>` | |
| Speed | `V<0-100>` | |
| Sensitivity | `S<n>` | Additive mic offset |
| **Gate** | `G<n>` | Sound-mode wake threshold (default 100) |
| **Gain** | `A<n>` | Mic gain % 50–300 (default 100) |
| Sound | `E` / `e` | On / off |
| Flash / Resync / Reboot | `L` / `R` / `Z` | |
| Status | `?` | |

### Head settings
| Cmd | Format | Notes |
|-----|--------|-------|
| Fan | `F0` `F1` `F2` | Off / On / Auto |
| Fan °F | `FT<n>` | AUTO threshold |
| CDS threshold | `I<n>` | Dim eyes when light ≥ n |
| Eye dim % | `D<n>` | 1–100 when dimmed |

---

## Mic behaviour (for app copy)

```text
level = max(0, ADC - 1600) * Gain/100 + Sensitivity
→ EMA smooth → compare to Gate → drive modes / stream to Head
```

Live meter: `STAT Mic:`

---

## Telemetry

```text
STAT M:3 B:80 V:50 S:75 G:100 A:100 E:1 Mic:1423 HeadB:512 HeadT:86.2
```

| Token | Meaning |
|-------|--------|
| `S` | Sensitivity |
| `G` | Gate |
| `A` | Gain % |
| `Mic` | Smoothed mic level |
| `HeadB` | CDS light |
| `HeadT` | Temp °F |

---

## Screens

1. Connect  
2. Control — modes, B, V, L, R  
3. Status — Mic, HeadT, HeadB  
4. Settings — Sound (E, A, S, G), Fan, Eyes/CDS, System  

See **SETTINGS.md** for full breakout and presets.

---

## CDS / eyes (Head)

CDS on Head A0; when reading ≥ `I` threshold, eyes (pixels 0–3) use brightness `D`%. Document in Settings help.

---

*v1.3 adds G (gate) and A (gain).*
