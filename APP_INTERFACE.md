# TMDrake Companion App – Interface Contract

**Version:** 1.6  
**Date:** July 2026  
**App brief:** [APP_TEAM.md](APP_TEAM.md)

---

## BLE NUS

Device: `TMDrake_tail`  
Service `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`  
RX write · TX notify · Plugin: `flutter_blue_plus`

---

## Heartbeat & data sync (`HB`)

Use for **keepalive** and **full state sync** after connect or when UI may be stale.

### App → Tail

```text
HB
```

(Optional: `HB 123` — payload after space is ignored by firmware for now; reserved for app sequence.)

**Recommended interval:** every **2–5 seconds** while connected.  
Also send **once immediately** after NUS subscribe / on resume from background.

### Tail → App (reply)

1. Compact ACK:
```text
HBACK Seq:42 U:3600
```
2. Immediate full snapshot (same format as live STAT):
```text
STAT M:9 B:80 V:50 S:75 G:100 A:100 E:1 C:157,78,221 T:0 Mic:100 HeadB:512 HeadT:86.2 U:3600 Seq:42
```

| Field | Meaning |
|-------|--------|
| `HBACK` | Heartbeat acknowledged |
| `Seq` | Monotonic counter (increments each HB) |
| `U` | Tail uptime seconds |
| `STAT …` | Full settings + sensors — **apply to UI** |

### App behaviour

| Event | Action |
|-------|--------|
| Connect + notify enabled | Send `HB` once |
| Timer 2–5 s | Send `HB` |
| Receive `HBACK` | Link healthy; reset “stale” timer |
| Receive `STAT` | Parse all tokens; sync sliders, mode, color, meters |
| No `HBACK` for >10 s | Show “link weak / reconnecting” |

Periodic unsolicited `STAT` (~2 Hz) still runs while connected; **HB forces an immediate sync** and proves the uplink is bidirectional.

---

## Modes 0–10

`M0`…`M10` — Tail + Head + PAWB.

---

## Color & themes

| Cmd | Format |
|-----|--------|
| Color | `C<r>,<g>,<b>` → mode 9, NVS, fan-out |
| Theme | `T0`–`T4` or `Tpurple` / `Tfire` / `Tice` / `Tgold` / `Temerald` |

| Id | Name | RGB |
|----|------|-----|
| 0 | purple | 157, 78, 221 |
| 1 | fire | 255, 60, 0 |
| 2 | ice | 80, 180, 255 |
| 3 | gold | 255, 180, 40 |
| 4 | emerald | 20, 200, 100 |

---

## Full commands

```text
M B V S G A E/e C T HB L R Z
F0 F1 F2 FT I D
?
```

---

## STAT fields

```text
STAT M:9 B:80 V:50 S:75 G:100 A:100 E:1 C:157,78,221 T:0 Mic:100 HeadB:512 HeadT:86.2 U:3600 Seq:42
```

| Token | Meaning |
|-------|--------|
| `M B V S G A E` | Mode, brightness, speed, mic, sound |
| `C` `T` | Solid RGB, theme id (-1 = custom) |
| `Mic` `HeadB` `HeadT` | Live meters |
| `U` | Uptime (s) |
| `Seq` | Last HB sequence (also on unsolicited STAT) |

---

*v1.6: Protocol HB + HBACK + STAT U/Seq for data sync.*
