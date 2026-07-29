# Drake 2.0 Improvements & Roadmap

**Firmware team status – July 2026**

Detail: [SYSTEM.md](SYSTEM.md) · [ESPNOW.md](ESPNOW.md) · [SETTINGS.md](SETTINGS.md) · [APP_INTERFACE.md](APP_INTERFACE.md)

## Implemented

| Item | Boards | Notes |
|------|--------|-------|
| BLE NUS (NimBLE) | Tail | Service UUID advertised |
| Modes **0–10 non-blocking** | **Tail + Head + PAWB** | Full parity |
| Brightness `B` + Speed `V` | Tail | NVS |
| Mic `S` + **Gate `G`** + **Gain `A`** + EMA | Tail | Shared `readMicLevel()` |
| Encrypted ESP-NOW | Tail↔Head | Mic, cmds, sensors |
| Fan `F*` / `FT*` + CDS `I*` / `D*` | Head via Tail | App Settings |
| Live `STAT` | Tail→app | Includes G, A, HeadB, HeadT |
| Docs for app + firmware | Tail repo | SETTINGS, SYSTEM, ESPNOW |

## Open / next (priority)

1. **Color command** `C` for Solid (and PAWB/Head sync)  
2. Mic auto-calibrate / noise floor (optional)  
3. Companion app build (app team; SETTINGS.md)  
4. On-suit ESP-NOW validation → then trim UDP fallback  
5. Remove unused blocking demos in Head `Other_modes.ino`  

## Mode parity checklist

- [x] Tail 0–10 non-blocking  
- [x] PAWB 0–10 non-blocking  
- [x] Head 0–10 non-blocking + CDS eyes  
- [x] Mode broadcast M via ESP-NOW / UDP / ASK  

---

*Firmware team: treat SYSTEM.md §2 and §7 as the current contract.*
