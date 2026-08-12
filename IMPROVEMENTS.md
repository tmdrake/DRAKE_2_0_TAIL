# Drake 2.0 Improvements & Roadmap

**Firmware team status – August 2026**

Detail: [SYSTEM.md](SYSTEM.md) · [ESPNOW.md](ESPNOW.md) · [SETTINGS.md](SETTINGS.md) · [APP_INTERFACE.md](APP_INTERFACE.md) · [MIC_HARDWARE.md](MIC_HARDWARE.md) · [SPECTRUM_FUTURE.md](SPECTRUM_FUTURE.md)

## Implemented

| Item | Boards | Notes |
|------|--------|-------|
| BLE NUS (NimBLE) | Tail | Service UUID advertised |
| Modes **0–10 non-blocking** | **Tail + Head + PAWB** | Full parity |
| Brightness `B` + Speed `V` | Tail | NVS |
| Mic `S` + **Gate `G`** + **Gain `A`** + EMA | Tail | **AGC mic module** + envelope |
| Hardware RC LPF | Tail | ~10 kΩ + 0.1 µF (recalled) |
| Encrypted ESP-NOW | Tail↔Head | Mic, cmds, sensors |
| Fan `F*` / `FT*` + CDS `I*` / `D*` | Head via Tail | App Settings |
| Live `STAT` | Tail→app | Includes G, A, HeadB, HeadT |
| Docs for app + firmware | Tail repo | SETTINGS, SYSTEM, ESPNOW, MIC_HARDWARE |

## Open / next (priority)

1. On-suit ESP-NOW validation → then trim UDP fallback  
2. Mic auto-calibrate / noise floor (optional)  
3. Companion app polish (app team)  
4. Meter-confirm RC markings when hardware open  
5. Remove unused blocking demos in Head `Other_modes.ino`  

## Future design (not scheduled)

| Idea | Doc |
|------|-----|
| **FFT → 16-band spectrum**, BLE snapshot, **band-select fire** (bass drops / music) | **[SPECTRUM_FUTURE.md](SPECTRUM_FUTURE.md)** |
| Needs wider analog bandwidth than current ~159 Hz LPF; AGC mic stays |
| Sample ~15–20 kHz (or 8–12 kHz) — not 44.1 kHz media rate |

## Mode parity checklist

- [x] Tail 0–10 non-blocking  
- [x] PAWB 0–10 non-blocking  
- [x] Head 0–10 non-blocking + CDS eyes  
- [x] Mode broadcast M via ESP-NOW / UDP / ASK  

---

*Firmware team: treat SYSTEM.md as the current contract; SPECTRUM_FUTURE is aspirational.*
