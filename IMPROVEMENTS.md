# Drake 2.0 Improvements & Roadmap

Status tracker. Detail: **[SYSTEM.md](SYSTEM.md)** · **[ESPNOW.md](ESPNOW.md)** · **[APP_INTERFACE.md](APP_INTERFACE.md)**

## Implemented (July 2026)

| Item | Notes |
|------|-------|
| BLE NUS (NimBLE) | Service UUID advertised |
| Modes 0–10 | Tail + PAWB |
| Brightness `B` + Speed `V` | NVS |
| Binary mic (UDP) | Superseded as primary by ESP-NOW |
| **Encrypted ESP-NOW** | Mic, cmds, light, temp Tail↔Head |
| Head/Tail WiFi no-sleep | Done |
| Live `STAT` to app | ~2 Hz |
| PAWB mode follow | Modes 0–10 via ASK |
| System + ESP-NOW docs | SYSTEM.md, ESPNOW.md |

## Open / next

1. Color / theme commands (`C`, `T`)  
2. Head mode visual parity  
3. Companion app  
4. Prove ESP-NOW on-suit; then optional remove UDP fallback  

---

*Update when features ship.*
