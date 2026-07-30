# TMDrake Companion App – Interface Contract

**Version:** 1.6  
**Date:** July 2026  
**App requirements brief:** [APP_TEAM.md](APP_TEAM.md) ← **read this for BLE service + HB mandates**

---

## BLE NUS

| Item | Value |
|------|--------|
| Device name | `TMDrake_tail` |
| Service | `6E400001-B5A3-F393-E0A9-E50E24DCCA9E` |
| RX (app→device) | `6E400002-B5A3-F393-E0A9-E50E24DCCA9E` write |
| TX (device→app) | `6E400003-B5A3-F393-E0A9-E50E24DCCA9E` notify |
| Stack | Flutter + `flutter_blue_plus` (Android first) |

### Connection requirements (app)

- Prefer connect with **`autoConnect: true`** after first successful discovery  
- Run a **foreground service** while the user has “Keep suit linked” enabled  
- On notify subscribe and on resume: send **`HB`**  
- While connected: **`HB` every 2–5 s**  
- No **`HBACK` for >10 s**: treat link as stale and reconnect  

Full checklist: [APP_TEAM.md](APP_TEAM.md).

---

## Heartbeat & data sync (`HB`)

### App → Tail

```text
HB
```

### Tail → App

```text
HBACK Seq:42 U:3600
STAT M:9 B:80 V:50 S:75 G:100 A:100 E:1 C:157,78,221 T:0 Mic:100 HeadB:512 HeadT:86.2 U:3600 Seq:42
```

| Field | Meaning |
|-------|--------|
| `HBACK` | Heartbeat OK (bidirectional link) |
| `Seq` | Monotonic HB counter |
| `U` | Tail uptime (seconds) |
| `STAT` | Full settings + sensors — **apply to all UI** |

Unsolicited `STAT` ~2 Hz also flows while connected. **HB** is required for health + forced resync.

---

## Modes 0–10

`M0` … `M10` — applies to Tail + Head + PAWB.

---

## Color & themes

| Cmd | Effect |
|-----|--------|
| `C<r>,<g>,<b>` | Solid RGB, mode 9, NVS, fan-out |
| `T0`–`T4` or named | Preset theme + mode 9 |

| Id | Name | RGB |
|----|------|-----|
| 0 | purple | 157, 78, 221 |
| 1 | fire | 255, 60, 0 |
| 2 | ice | 80, 180, 255 |
| 3 | gold | 255, 180, 40 |
| 4 | emerald | 20, 200, 100 |

Names: `Tpurple` `Tfire` `Tice` `Tgold` `Temerald` (case-insensitive).

---

## Command summary

```text
M B V S G A E/e C T HB L R Z
F0 F1 F2 FT I D
?
```

---

## STAT tokens

`M B V S G A E C T Mic HeadB HeadT U Seq`

---

*v1.6: HB sync + app link-service requirements documented.*
