# App Team – Requirements & What You Can Implement

**Firmware contract:** [APP_INTERFACE.md](APP_INTERFACE.md) **v1.6**  
**Date:** July 2026  
**Audience:** Companion app (Flutter / Android first)

---

## REQUIRED: BLE Link Service + Heartbeat

The suit must stay linked without the user hammering Connect. Treat this as a **product requirement**, not optional polish.

### Architecture

```text
Android foreground service (while “Keep suit linked” is ON)
  → autoConnect to TMDrake_tail (NUS UUID)
  → subscribe TX notify
  → HB every 2–5 s + HB once on subscribe / resume
  → HBACK = link healthy
  → STAT = full UI sync
  → no HBACK >10 s → disconnect + autoConnect / rescan
```

### Must implement

| # | Requirement | Detail |
|---|-------------|--------|
| 1 | **Foreground service** | Android notification while link mode is enabled (“TMDrake linked” / “Searching…”) so BLE is not killed in background |
| 2 | **autoConnect** | After first successful connect, store device id; reconnect with `autoConnect: true` (flutter_blue_plus) |
| 3 | **Discovery** | Filter by NUS service UUID `6E400001-B5A3-F393-E0A9-E50E24DCCA9E` and/or name `TMDrake_tail` |
| 4 | **HB on link up** | Send `HB` immediately after notify subscription |
| 5 | **HB timer** | Send `HB` every **2–5 seconds** while connected |
| 6 | **HBACK handling** | Reset stale timer; mark link healthy |
| 7 | **STAT handling** | Parse **all** tokens and sync UI (mode, B/V, mic, color, theme, HeadT/B, U, Seq) |
| 8 | **Stale / reconnect** | No `HBACK` for **>10 s** → show weak-link UI, disconnect, then autoConnect or rescan |
| 9 | **Resume** | App/service resume from background → send `HB` once for full resync |
| 10 | **User off switch** | “Keep suit linked” OFF → stop timer, disconnect, stop foreground service |

### Protocol (firmware already live)

```text
App  --HB-->  Tail
App  <--HBACK Seq:n U:uptimeSec--
App  <--STAT M:… B:… C:r,g,b T:… Mic:… HeadB:… HeadT:… U:… Seq:…--
```

- Unsolicited `STAT` ~2 Hz continues while connected; **HB forces bidirectional proof + immediate snapshot**.
- `Seq` increments each HB — use to detect gaps if desired.

### Suggested link states

```text
OFF → SCANNING → CONNECTING → LINKED → STALE → (retry CONNECTING)
```

### Permissions (Android)

Bluetooth connect/scan (and legacy location only if required by target SDK), **notifications** for the foreground service. Document for the user why the persistent notification exists.

iOS: background BLE is more limited; Android-first is the target.

---

## Fully supported commands (wire to UI)

| Feature | Command |
|---------|---------|
| Modes 0–10 | `M0`–`M10` |
| Brightness / Speed | `B0-100` / `V0-100` |
| Mic gain / sensitivity / gate | `A` / `S` / `G` |
| Sound on/off | `E` / `e` |
| Solid color | `C<r>,<g>,<b>` → mode 9 |
| Themes | `T0`–`T4` or `Tpurple` `Tfire` `Tice` `Tgold` `Temerald` |
| **Heartbeat** | **`HB`** |
| Flash / Resync / Reboot | `L` / `R` / `Z` |
| Fan | `F0` `F1` `F2` `FT<n>` |
| Eyes / CDS | `I<n>` `D<n>` |
| Status dump | `?` |

### Theme RGB map

| Id | Name | RGB |
|----|------|-----|
| 0 | purple | 157, 78, 221 |
| 1 | fire | 255, 60, 0 |
| 2 | ice | 80, 180, 255 |
| 3 | gold | 255, 180, 40 |
| 4 | emerald | 20, 200, 100 |

### STAT tokens to parse

`M B V S G A E C T Mic HeadB HeadT U Seq`

---

## Screens (unchanged intent)

1. **Control** — modes, B/V, theme circles + color, Flash, Resync, link indicator  
2. **Status** — Mic, HeadT, HeadB, Seq/uptime, log  
3. **Settings** — Sound, Fan, Eyes, **Keep suit linked** toggle, Reboot  

Link health should be visible on Control (dot / banner).

---

## Acceptance checklist (app)

- [ ] Foreground service while link enabled  
- [ ] autoConnect after first successful pair  
- [ ] HB on subscribe + every 2–5 s  
- [ ] HBACK clears stale; STAT syncs all controls  
- [ ] >10 s without HBACK triggers reconnect path  
- [ ] Modes, color/themes, mic, fan, CDS all live  
- [ ] TMDrake branding  

---

*Source of truth for wire format: APP_INTERFACE.md. Firmware questions → SYSTEM.md / FIRMWARE_NOTES.md.*
