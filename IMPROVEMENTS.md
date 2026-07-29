# Drake 2.0 Improvements & Roadmap

Suggestions compiled for evolving the networkable dragonsuit lighting system (Head ESP8266 + Tail ESP32 + Paw Arduino).

## 1. Switch Remote Serial to BLE (Nordic UART Service)

**Current:** Classic Bluetooth SPP (`BluetoothSerial` "TMDrake_tail").

**Recommended:** Move to BLE Nordic UART Service (NUS) using **NimBLE-Arduino** for lower RAM/flash use and better modern phone compatibility.

### Benefits
- Better battery life
- Works reliably with current Android/iOS
- Easier pairing
- Can keep the exact same command set

### Suggested Libraries
- NimBLE-Arduino + NuS-NimBLE-Serial / Serial_BLE / BLESerial (with `BLESERIAL_USE_NIMBLE=true`)

### Latency Tuning for BLE
- Connection interval: 7.5–15 ms
- Larger MTU (247+)
- Prefer notifications over polling

## 2. Expanded Lighting Modes

| Mode | Name              | Description                              | Notes |
|------|-------------------|------------------------------------------|-------|
| 0    | Sound Phase       | Color-phase sound reactive (current)     | Keep  |
| 1    | Sound Distinct    | Hard color cycle sound reactive          | Keep  |
| 2    | VU Meter          | Classic bar graph                        | Keep  |
| 3    | Rainbow Chase     | Smooth flowing rainbow                   | New   |
| 4    | Comet / Meteor    | Bright head + fading trail               | New   |
| 5    | Breathing Pulse   | Slow sine brightness + color shift       | New   |
| 6    | Fire Flicker      | Orange/red random flicker + upward drift | New   |
| 7    | Sparkle / Twinkle | Random bright sparkles on dark base      | New   |
| 8    | Wave / Undulate   | Soft traveling brightness wave           | New   |
| 9    | Solid / Static    | Single controllable color                | New   |
| 10   | Off / Blackout    | Everything off                           | New   |

All new modes must stay non-blocking (millis-based) so the main loop, WiFi, BLE, and ASK keep running.

## 3. Tuning Parameters (Expose via App)

Live adjustable + saved to NVS/EEPROM:

- Mode (0–10+)
- Master Brightness (0–100%)
- Sound Sensitivity
- Sound Enable/Disable
- Animation Speed (global multiplier)
- Color Theme / Palette (Default purple-blue, Fire, Ice, Rainbow, Custom RGB)
- Mic Threshold / Decay
- Eye brightness / dim override
- Background fade speed
- Flash duration
- User presets (3–4 slots)

## 4. Proposed BLE / Serial Command Protocol

Keep human-readable for easy debugging and app development:

```
M<mode>          // Set mode
B<0-100>         // Master brightness
S<value>         // Sensitivity
E0 / E1          // Sound off / on
V<0-100>         // Animation speed
C<r>,<g>,<b>     // Base / theme color
T<theme>         // Named theme
P<preset>        // Load preset
W                // Write / save settings
R                // Resync / reset animation state
L                // Flash
?                // Status dump
```

## 5. Branded Companion App

**Goal:** Full TMDrake / Drake Dragon branded BLE terminal + control surface.

### Recommended Stack
- **Flutter** (single codebase for Android + iOS)
- Nordic UART Service support
- Dark purple/blue dragon theme, logo, suit-inspired icons

### App Features
- Mode selector (grid or carousel with dragon-themed icons)
- Sliders: Brightness, Sensitivity, Speed
- Color picker / theme buttons
- Quick actions: Flash, Resync, Save Preset
- Live status (mode, mic level, head temp if streamed)
- Optional animated dragon head that reacts to mic level

**Quick start alternative:** Fork SimpleBluetoothLeTerminal (Android) and re-skin, or use nRF Connect temporarily.

## 6. UDP / WiFi Latency Optimizations (for animations & mic stream)

High-impact changes already identified:

1. **Binary unicast mic packets** instead of ASCII + broadcast
2. **Disable WiFi power save** on both boards
   - Tail (ESP32): `WiFi.setSleep(false);` or `esp_wifi_set_ps(WIFI_PS_NONE);`
   - Head (ESP8266): `WiFi.setSleepMode(WIFI_NONE_SLEEP);`
3. Only stream mic data while soundmode is active + change-threshold
4. Fixed clean channel (1 or 6) on SoftAP
5. Consider **ESP-NOW** for the high-rate mic stream (much lower latency than UDP over SoftAP)

## 7. Implementation Priority Suggestion

1. Port Tail serial interface to NimBLE NUS (keep command compatibility)
2. Apply binary UDP + power-save + unicast fixes
3. Expand `mode_selector()` with new non-blocking animations
4. Extend command parser for all new tuning parameters
5. Build / brand the companion app
6. (Optional) Add ESP-NOW mic path + accelerometer mode

---

*Generated from design discussion – July 2026*
*Feel free to turn sections into Issues or implement piece by piece.*
