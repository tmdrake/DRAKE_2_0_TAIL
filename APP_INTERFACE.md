# TMDrake Companion App – Interface Contract

**Version:** 2.0  
**Date:** August 2026  
**Audience:** App developers (Flutter / Android first; USB tools share the same command language)  
**Link-service requirements:** [APP_TEAM.md](APP_TEAM.md)

This document is the **wire-format source of truth**. Firmware fans out commands to Head (ESP-NOW/UDP) and PAWB (ASK) as needed — the app only talks to the **Tail**.

---

## 1. Transport

### BLE NUS (phone)

| Item | Value |
|------|--------|
| Device name | `TMDrake_tail` |
| Service | `6E400001-B5A3-F393-E0A9-E50E24DCCA9E` |
| RX (app → Tail) | `6E400002-…` **write** |
| TX (Tail → app) | `6E400003-…` **notify** |
| Framing | UTF-8 text; one command per write (trim OK). Replies are notify strings (often with `\n`). |

### USB serial (same commands)

| Item | Value |
|------|--------|
| Baud | **115200** |
| Protocol | Identical to BLE: write a line / string → `processBLECommand` |
| Use | TUI (`tools/tail_tui.py`), debug, lab tools |

Do **not** assume binary packets on the app link — binary is ESP-NOW only (Tail ↔ Head).

---

## 2. Connection & heartbeat (required)

| Rule | Detail |
|------|--------|
| On notify subscribe / resume | Send **`HB`** once |
| While connected | Send **`HB` every 2–5 s** |
| Healthy | Receive **`HBACK`** |
| Stale | No `HBACK` for **>10 s** → reconnect |
| Android | Foreground service while “Keep suit linked” is on; prefer `autoConnect` after first pair |

Full checklist: [APP_TEAM.md](APP_TEAM.md).

### `HB` exchange

```text
App  →  HB
Tail →  HBACK Seq:<n> U:<uptimeSec>
Tail →  STAT M:… B:… V:… S:… G:… A:… E:… C:r,g,b T:… Mic:… HeadB:… HeadT:… U:… Seq:…
```

- Unsolicited **`STAT` ~2 Hz** while BLE is connected (no need to poll for live mic/temp).
- **`HB`** forces proof-of-link + immediate full snapshot.

---

## 3. STAT line (parse every token)

Example:

```text
STAT M:1 B:65 V:100 S:100 G:300 A:100 E:1 C:255,60,0 T:1 Mic:260 HeadB:730 HeadT:80.9 U:340 Seq:168
```

| Token | Type | Meaning | UI |
|-------|------|---------|-----|
| `M` | int 0–10 | Current mode | Mode grid highlight |
| `B` | int 0–100 | Master brightness % | Brightness slider |
| `V` | int 0–100 | Anim speed % | Speed slider (flood / chase pace) |
| `S` | int 10–400 | Mic **amp %** (post-envelope gain) | Mic settings |
| `G` | int 5–2000 | Mic **gate** (wake threshold for M0/M1) | Gate slider; see §5 |
| `A` | int 50–300 | Mic **preamp %** | Mic settings |
| `E` | 0\|1 | Sound detect master | Toggle |
| `C` | `r,g,b` | Solid color RGB | Color picker |
| `T` | int −1…4 | Theme id (−1 = custom `C`) | Theme chips |
| `Mic` | int ≥0 | **Raw** mic envelope (not 0–100 bar) | Status; gate tuning |
| `HeadB` | int | Head CDS / ambient light reading | Status |
| `HeadT` | float | Head temp °F | Status |
| `U` | int | Tail uptime seconds | Status |
| `Seq` | int | Last HB sequence | Link health |

**Mic display tip:** `Mic` is a raw envelope count (often ~200–400 at quiet with the Adafruit mic). The **on-suit VU bar** uses an internal noise-floor **excess** scale (0…1). For an app “level meter,” either:

1. Show raw `Mic` with a user-tunable floor, or  
2. Show a relative meter vs a short max (EMA peak of `Mic`), similar to firmware.

Gate **G** should be set **above** the quiet `Mic` reading so M0/M1 do not stay permanently awake.

---

## 4. Modes 0–10

Command: **`M0`** … **`M10`** (or `M10` as `MA` on ASK; app should send `M10`).

Applies to **Tail + Head + both PAWBs** (suit fan-out).

| ID | Name (UI label) | Behavior (user-facing) |
|----|-----------------|-------------------------|
| **0** | **Sound Phase** | Mic-gated color flood; continuous hue wheel; idle purple when quiet |
| **1** | **Sound Pulse** | Mic-gated flood; stepped 6-color wheel; idle purple when quiet |
| **2** | **VU Meter** | Continuous mic bar (always on; uses excess scale on suit) |
| **3** | Rainbow | Phase-synced chase |
| **4** | Comet | Phase-synced |
| **5** | Breathe | Phase-synced |
| **6** | Dragonfire | Free-run |
| **7** | Sparkle | Free-run |
| **8** | Wave | Phase-synced |
| **9** | Solid | Solid color (`C` / theme) |
| **10** | Blackout | All off |

### M0 / M1 gating (important for UX copy)

```text
Mic raw > G  →  sound “hot”  →  Sound Phase / Pulse runs
Quiet ~2.5 s →  idle purple keep-alive pulse on Tail/Head
E = 0        →  sound detect off; stay idle (no wake)
```

**Brightness of Phase/Pulse inject** tracks the same mic **excess** scale as the VU (not “how far above G”). **G** is wake only.

### Speed `V`

Affects animation / flood scroll rate on Tail (higher = faster). Map **0–100** in UI.

---

## 5. Mic controls

| Cmd | Range | Meaning | Persist |
|-----|-------|---------|---------|
| **`S<n>`** | 10–400 | Amp % on envelope (was historically a DC offset; **now gain % only**) | NVS |
| **`A<n>`** | 50–300 | Preamp % | NVS |
| **`G<n>`** | 5–2000 | Gate threshold vs **raw** `Mic` for M0/M1 wake | NVS |
| **`E`** | — | Sound detect **on** | NVS |
| **`e`** | — | Sound detect **off** | NVS |

Suggested defaults for UI if STAT not yet received: `S=100`, `A=100`, `G=40`, `E=1`.  
If quiet `Mic` ≈ 250–270, set **`G` ≈ 300+** so Phase/Pulse only open on real sound.

---

## 6. Brightness, color, themes

| Cmd | Range / format | Effect |
|-----|----------------|--------|
| **`B<n>`** | 0–100 | Master brightness % |
| **`C<r>,<g>,<b>`** | 0–255 each | Solid RGB → mode **9**, fan-out |
| **`T0`…`T4`** | or names | Theme + mode **9** |

| T id | Name | RGB |
|------|------|-----|
| 0 | purple | 157, 78, 221 |
| 1 | fire | 255, 60, 0 |
| 2 | ice | 80, 180, 255 |
| 3 | gold | 255, 180, 40 |
| 4 | emerald | 20, 200, 100 |

Names (case-insensitive): `Tpurple` `Tfire` `Tice` `Tgold` `Temerald`.

---

## 7. Head-only (via Tail)

Tail forwards to Head; app still sends on the Tail BLE/USB link.

| Cmd | Range | Effect |
|-----|-------|--------|
| **`F0`** | — | Fan off |
| **`F1`** | — | Fan on |
| **`F2`** | — | Fan auto |
| **`FT<n>`** | ~50–120 | Auto threshold °F (default 85) |
| **`I<n>`** | 0–1023 | CDS threshold (eyes dim when light ≥ threshold) |
| **`D<n>`** | 1–100 | Eye dim percent when CDS dimmed |

Live feedback: **`HeadT`**, **`HeadB`** in STAT.

---

## 8. Utility

| Cmd | Effect |
|-----|--------|
| **`L`** | Flash lamp (white blip) + fan-out |
| **`R`** | Resync (background reset + `R0` to suit) |
| **`Z`** | Reboot Tail |
| **`?`** | Help + current STAT |

---

## 9. Command cheat sheet

```text
M0–M10          mode (suit-wide)
B0–100          brightness
V0–100          speed
S10–400         mic amp %
A50–300         mic preamp %
G5–2000         mic gate (M0/M1 wake)
E / e           sound detect on / off
C<r>,<g>,<b>    solid color → M9
T0–T4 | Tname   theme → M9
HB              heartbeat
L R Z           flash / resync / reboot
F0 F1 F2 FT<n>  fan
I<n> D<n>       CDS / eye dim
?
```

---

## 10. Recommended UI structure

1. **Control** — mode grid (labels in §4), B/V, themes + color, Flash, Resync, **link health**  
2. **Status** — `Mic`, `HeadT`, `HeadB`, uptime, Seq, short log  
3. **Settings** — Sound E, S/A/G with help text for gate, Fan, Eyes, **Keep suit linked**, Reboot  

Link states: `OFF → SCANNING → CONNECTING → LINKED → STALE → (retry)`.

---

## 11. What the app does *not* need

| Internal path | App involvement |
|---------------|-----------------|
| ESP-NOW mic packets Tail → Head | None (automatic) |
| ASK mic pulses Tail → PAWB (M0–M2 only) | None |
| Phase sync 25 Hz | None |
| Suit settings re-push ~30 s | None |

If modes diverge briefly after Head reboot, Tail’s periodic suit sync re-aligns; app can also re-send **`M`** after `HB`.

---

## 12. Changelog (app-relevant)

### v2.0 — August 2026

- Mode names: **Sound Phase (0)**, **Sound Pulse (1)**, **VU Meter (2)**  
- **`S`** is mic **gain %** (10–400), not a DC offset  
- **`G`** gate vs raw `Mic`; M0/M1 wake ~2.5 s quiet hold  
- Suit VU / Phase / Pulse use internal **excess** scale (quiet ≈ empty); STAT `Mic` remains raw  
- Suit-wide fan-out of modes; PAWB gets ASK mic **pulses** only on M0–M2  

### v1.6 — July 2026

- `HB` / `HBACK` / `STAT` + foreground link service requirements  

---

*Firmware architecture: [SYSTEM.md](SYSTEM.md). BLE product requirements: [APP_TEAM.md](APP_TEAM.md).*
