# Drake 2.0 – System Documentation

**Last updated:** July 2026  
Related: [ESPNOW.md](ESPNOW.md) · [APP_INTERFACE.md](APP_INTERFACE.md)

---

## 1. Hardware roles

| Board | MCU | Role |
|-------|-----|------|
| **Tail** | ESP32 | Mic, BLE, ESP-NOW, ASK TX, modes |
| **Head** | ESP8266 | SoftAP, ESP-NOW, eyes+spikes, **CDS**, **fan**, temp |
| **PAWB** | Pro Mini | Claws, ASK RX |

---

## 2. Head sensors (important)

### Temperature + fan

| Item | Detail |
|------|--------|
| Sensor | DS18B20-style OneWire on **D5** |
| Fan pin | **D4** (HIGH = on) |
| Default AUTO threshold | **85 °F** |
| Modes | `F0` off · `F1` on · `F2` auto |
| Set threshold | `FT<n>` (°F, 50–120) |

In AUTO: fan runs when measured temp **>** threshold.

### CDS photocell → eye brightness

| Item | Detail |
|------|--------|
| Sensor | **CDS** (photocell) voltage divider on Head **A0** |
| ADC range | ~0–1023 |
| Eyes | NeoPixels **0–3** |
| Logic | `dim_eyes = (sensorValue >= cdsThreshold)` |
| Default threshold | **500** (`I500`) |
| When dimmed | `eyesbrightness(eyeDimPercent/100)` — default **10%** (`D10`) |
| When not dimmed | `eyesbrightness(1.0)` |
| Live to app | `STAT` **`HeadB`** |

**Intent:** In brighter ambient light (higher CDS reading with current wiring), eye LEDs automatically dim so the face doesn’t wash out. Calibrate `I` and `D` per suit if the CDS divider is different.

Implemented in:
- `checkLight()` — reads CDS, sets `dim_eyes`, sends `HeadB`
- `soundloop()` / `eyes_led.ino` — applies eye pixel brightness

---

## 3. Links

| Link | Medium |
|------|--------|
| Tail↔Head hot path | Encrypted **ESP-NOW** (mic, cmds, light, temp) |
| Tail→PAWB | **ASK** 2000 baud pin 17 → A0 |
| Phone→Tail | **BLE NUS** |

Head settings commands (`F*`, `I*`, `D*`) go Tail→Head only (not ASK).

---

## 4. BLE commands (summary)

Lighting: `M` `B` `V` `S` `E/e` `L` `R` `Z` `?`  
Head: `F0` `F1` `F2` `FT<n>` `I<n>` `D<n>`

Full app contract: [APP_INTERFACE.md](APP_INTERFACE.md)

---

## 5. Analog mic (Tail)

```text
level = analogRead(A0) - 1600 + sensitivity
```

OFFSET 1600 = mic preamp bias in ADC counts.

---

## 6. Modes 0–10

On Tail + PAWB. Head receives mode byte for sound-loop style.

---

## 7. Bench notes

1. Pair ESP-NOW MACs (see ESPNOW.md).  
2. Test fan: `F1` / `F0` / `F2` + `FT80`.  
3. Test CDS: cover/uncover photocell; watch `HeadB` and eye brightness; tune `I` / `D`.

---

*Keep in sync when Head sensor behaviour changes.*
