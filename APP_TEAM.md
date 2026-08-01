# App Team – Requirements & Implementation Guide

**Firmware contract:** [APP_INTERFACE.md](APP_INTERFACE.md) **v2.0** (August 2026)  
**Audience:** Companion app (Flutter / Android first)

Wire format and ranges live in **APP_INTERFACE.md**. This file is the **product / engineering checklist**.

---

## REQUIRED: BLE link service + heartbeat

Treat “stay linked” as a **product requirement**.

```text
Android foreground service (“Keep suit linked” ON)
  → discover / autoConnect TMDrake_tail (NUS UUID)
  → subscribe TX notify
  → HB on subscribe + every 2–5 s
  → HBACK = healthy; STAT = full UI sync
  → no HBACK >10 s → STALE → reconnect
```

| # | Requirement |
|---|-------------|
| 1 | Foreground service + notification while link enabled |
| 2 | `autoConnect: true` after first successful pair (store device id) |
| 3 | Filter by NUS UUID and/or name `TMDrake_tail` |
| 4 | Send **`HB`** immediately after notify subscribe |
| 5 | **`HB` every 2–5 s** while connected |
| 6 | **`HBACK`** resets stale timer |
| 7 | Parse **all** `STAT` tokens → sync UI |
| 8 | **>10 s** without `HBACK` → reconnect path |
| 9 | Resume from background → one **`HB`** |
| 10 | User turns link OFF → stop timer, disconnect, stop service |

---

## Modes (UI labels)

| ID | Label | Notes for UI |
|----|-------|----------------|
| 0 | Sound Phase | Mic-gated rainbow flood |
| 1 | Sound Pulse | Mic-gated stepped colors |
| 2 | VU Meter | Always-on mic bar |
| 3 | Rainbow | |
| 4 | Comet | |
| 5 | Breathe | |
| 6 | Dragonfire | |
| 7 | Sparkle | |
| 8 | Wave | |
| 9 | Solid | Uses `C` / themes |
| 10 | Blackout | |

Send `M0`…`M10`. Suit fans out to Head + PAWBs.

### Mic settings UX

| Control | Cmd | Hint for user |
|---------|-----|----------------|
| Sound detect | `E` / `e` | Master wake for Phase/Pulse |
| Gate | `G` | Must sit **above** quiet `Mic` (e.g. quiet 260 → G 300) |
| Amp % | `S` | 10–400 overall mic gain |
| Preamp % | `A` | 50–300 |

Show live **`Mic`** on Status so users can tune **G**.

---

## Commands to wire

| Feature | Command |
|---------|---------|
| Modes | `M0`–`M10` |
| Brightness / speed | `B0-100` / `V0-100` |
| Mic | `S` `A` `G` `E`/`e` |
| Color / themes | `C<r>,<g>,<b>` · `T0`–`T4` / names |
| Heartbeat | **`HB`** |
| Flash / Resync / Reboot | `L` / `R` / `Z` |
| Fan | `F0` `F1` `F2` `FT<n>` |
| Eyes / CDS | `I<n>` `D<n>` |
| Dump | `?` |

Themes: purple / fire / ice / gold / emerald (see APP_INTERFACE).

---

## STAT tokens (must parse)

`M B V S G A E C T Mic HeadB HeadT U Seq`

---

## Screens

1. **Control** — modes, B/V, themes + color, Flash, Resync, link indicator  
2. **Status** — Mic, HeadT, HeadB, Seq/uptime, log  
3. **Settings** — Sound, S/A/G, Fan, Eyes, Keep suit linked, Reboot  

---

## Acceptance checklist

- [ ] Foreground service while link enabled  
- [ ] autoConnect after first pair  
- [ ] HB on subscribe + every 2–5 s  
- [ ] HBACK clears stale; STAT drives all controls  
- [ ] >10 s no HBACK → reconnect  
- [ ] Mode labels match table above  
- [ ] Gate UX explains relationship to live Mic  
- [ ] Fan + CDS live  
- [ ] TMDrake branding  

---

## Lab / USB

Same command language over USB **115200** (see `tools/tail_tui.py`). Useful for QA without BLE.

---

*Source of truth for wire format: [APP_INTERFACE.md](APP_INTERFACE.md). Architecture: [SYSTEM.md](SYSTEM.md).*
