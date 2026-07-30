# TMDrake Companion App – Interface Contract

**Version:** 1.5  
**Date:** July 2026  
**App brief:** [APP_TEAM.md](APP_TEAM.md)

---

## BLE NUS

Device: `TMDrake_tail`  
Service `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`  
RX write · TX notify · Plugin: `flutter_blue_plus`

---

## Modes 0–10 (full suit)

`M0`…`M10` — Tail + Head + PAWB.

---

## Color & themes (**live in firmware**)

| Cmd | Format | Effect |
|-----|--------|--------|
| **Color** | `C<r>,<g>,<b>` | RGB 0–255 → solid color, **mode 9**, fan-out to Head/PAWB, NVS |
| **Theme** | `T0`…`T4` or named | Sets preset RGB + mode 9 |

### Theme map

| Id | Name | RGB |
|----|------|-----|
| 0 | purple | 157, 78, 221 |
| 1 | fire | 255, 60, 0 |
| 2 | ice | 80, 180, 255 |
| 3 | gold | 255, 180, 40 |
| 4 | emerald | 20, 200, 100 |

Accepts: `T0`, `Tpurple`, `Tfire`, `Tice`, `Tgold`, `Temerald` (case-insensitive).

App theme circles + HSV picker can be **fully live** — no longer optimistic-only.

---

## Full commands

`M B V S G A E/e C T L R Z F0/F1/F2 FT I D ?`

---

## STAT (~2 Hz)

```text
STAT M:9 B:80 V:50 S:75 G:100 A:100 E:1 C:157,78,221 T:0 Mic:100 HeadB:512 HeadT:86.2
```

| Token | Meaning |
|-------|--------|
| `C` | Current solid RGB |
| `T` | Theme id 0–4, or -1 if custom `C` |
| `Mic` `HeadB` `HeadT` | Live meters |

---

*v1.5: C + T implemented + STAT color fields.*
