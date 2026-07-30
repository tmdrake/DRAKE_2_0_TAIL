# DRAKE_2_0_TAIL

ESP32 **Tail** firmware — mic, BLE NUS, ESP-NOW, ASK, modes 0–10.

## Repo policy

**Source + docs only.** Do **not** commit binary artifacts (PDF, PNG/JPG, `.bin`/`.hex`, APK, fonts, zip). Keep datasheets and screenshots outside git (local drive, wiki, or release assets). Wire protocols may be *binary on the radio*; that logic stays as **source** (`.ino` / `.md`), not as binary files.

## Docs

| Doc | Purpose |
|------|--------|
| [SYSTEM.md](SYSTEM.md) | Architecture |
| [ESPNOW.md](ESPNOW.md) | Tail↔Head packets |
| [APP_INTERFACE.md](APP_INTERFACE.md) | BLE contract v1.6 |
| [APP_TEAM.md](APP_TEAM.md) | App requirements (HB, FGS) |
| [FIRMWARE_NOTES.md](FIRMWARE_NOTES.md) | Modes / team notes |

## Flash

Arduino + NimBLE until Drake_3.0. Set `HEAD_PEER_MAC` before ESP-NOW bench test.

http://tmdrake.com
