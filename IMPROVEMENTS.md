# Drake 2.0 Improvements & Roadmap

Status tracker. Full technical detail lives in **[SYSTEM.md](SYSTEM.md)**.

## Implemented (July 2026)

| Item | Notes |
|------|-------|
| BLE NUS (NimBLE) | Replaced Classic SPP; service UUID advertised |
| Modes 0–10 | Tail + PAWB; see SYSTEM.md |
| Brightness `B` + Speed `V` | NVS-backed |
| Binary mic Tail→Head | 2-byte unicast + change threshold |
| Head WiFi no-sleep | `WIFI_NONE_SLEEP` |
| Head→Tail sensor unicast | Light 1235, temp 1236 |
| Live `STAT` to app | ~2 Hz over BLE TX |
| App interface contract | [APP_INTERFACE.md](APP_INTERFACE.md) |
| System documentation | [SYSTEM.md](SYSTEM.md) |

## Open / next

1. Color / theme commands (`C`, `T`) and solid-color sync to PAWB  
2. Head visual parity for modes 3–10  
3. Companion app build (other agent; use APP_INTERFACE.md)  
4. Optional ESP-NOW mic path  
5. Connection interval / MTU BLE tuning  

## Priority order used so far

1. ~~BLE migration~~  
2. ~~Expanded modes~~  
3. ~~Brightness / speed~~  
4. ~~Binary mic + Head feedback + STAT~~  
5. ~~PAWB mode follow~~  
6. Documentation (this pass)  

---

*Update SYSTEM.md when behaviour changes; update this table when features ship.*
