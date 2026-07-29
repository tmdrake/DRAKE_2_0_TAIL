# DRAKE_2_0_TAIL

Based On the Node32S - 4MB flash/NO-OTA! LARGE APP NEEDED. FLASH-ROM at 40Mhz.
Board version 2.0.17.

Driver firmware running on suit tail.

1. WIFI + ASK transmitter -> Sends information on mode and options
2. WIFI sends mic data
3. **BLE Nordic UART Service (NimBLE)** for remote control (replaced Classic SPP)

### BLE Notes (July 2026)
- Device name: `TMDrake_tail`
- Service UUID: `6E400001-B5A3-F393-E0A9-E50E24DCCA9E` (Nordic UART)
- Requires **NimBLE-Arduino** library (install via Library Manager)
- Commands remain the same: `M`, `S`, `E/e`, `L`, `R`, `Z`, `?`
- Use nRF Connect, Serial Bluetooth Terminal (BLE mode), or a future Flutter app

Planned to add: Accelerometer data. More other light patterns after converting them to non-blocking code.

**See [IMPROVEMENTS.md](IMPROVEMENTS.md) for the full roadmap** (expanded modes, app tuning, latency optimizations, branded companion app).

http://tmdrake.com
