# TMDrake Companion App – Interface Contract

**Version:** 1.1  
**Date:** July 2026  
**Target:** Sleek Android app (Flutter preferred) branded TMDrake / Drake Dragon  
**Hardware:** ESP32 Tail (`TMDrake_tail`) via BLE Nordic UART Service

This document is the formal contract between the suit firmware and the mobile app.  
Any agent implementing the app should treat this as the source of truth.

Full suit architecture (pins, UDP ports, ASK, modes): **[SYSTEM.md](SYSTEM.md)**

---

## 1. Branding

| Element | Spec |
|---------|------|
| App Name | **TMDrake** or **Drake Control** |
| Tagline | Networkable Lighting for the Dragonsuit |
| Primary Color | Deep purple / indigo (`#4A1C6B` – `#7B2D8E`) |
| Accent | Electric blue / cyan (`#00C2FF`) |
| Background | Near-black / dark navy (`#0D0B14`) |
| Text | Clean sans (e.g. Inter / Roboto), light on dark |
| Logo | TMDrake wordmark + stylized dragon head / suit silhouette |
| Tone | Sleek, modern, slightly aggressive-elegant (dragon energy) |

All icons should feel cohesive with a dark-purple + cyan palette. Prefer filled / dual-tone style over thin outlines.

---

## 2. BLE Connection Spec

### Discovery
- **Primary filter:** Service UUID  
  `6E400001-B5A3-F393-E0A9-E50E24DCCA9E` (Nordic UART Service)
- **Secondary:** Device name starts with `TMDrake` (currently `TMDrake_tail`)
- Prefer service-UUID filtering for reliability across Android versions.

### Nordic UART Service (NUS)
| Role | UUID | Properties |
|------|------|------------|
| Service | `6E400001-B5A3-F393-E0A9-E50E24DCCA9E` | — |
| RX (App → Suit) | `6E400002-B5A3-F393-E0A9-E50E24DCCA9E` | Write / Write Without Response |
| TX (Suit → App) | `6E400003-B5A3-F393-E0A9-E50E24DCCA9E` | Notify + Read |

### Recommended Flutter plugin
- `flutter_blue_plus` (primary recommendation)

### Connection flow
1. Scan with `withServices: [Guid(NUS_SERVICE_UUID)]`
2. Connect to chosen device
3. Discover services / characteristics
4. Subscribe to TX notifications
5. Write commands to RX characteristic as UTF-8 strings
6. On disconnect → auto-restart scan or show reconnect button

---

## 3. Command Protocol (App → Suit)

All commands are **plain ASCII / UTF-8 strings** written to the RX characteristic.  
No null terminator required. Trim whitespace on the firmware side.

| Command | Format | Range / Values | Description |
|---------|--------|----------------|-------------|
| Mode | `M<n>` | 0–10 | Set lighting mode |
| Brightness | `B<n>` | 0–100 | Master brightness % |
| Speed | `V<n>` | 0–100 | Animation speed (50 = normal) |
| Sensitivity | `S<n>` | roughly -1500…4000 | Mic sensitivity |
| Sound Enable | `E` | — | Enable sound detection |
| Sound Disable | `e` | — | Disable sound detection |
| Flash | `L` | — | Trigger short flash |
| Resync | `R` | — | Reset animation state / sync |
| Reboot | `Z` | — | Reboot the Tail |
| Status | `?` | — | Request full status dump |

### Mode map (must match firmware)
| ID | Name | Icon suggestion |
|----|------|-----------------|
| 0 | Sound Phase | waveform + color gradient |
| 1 | Sound Distinct | hard color blocks / equalizer |
| 2 | VU Meter | classic bar meter |
| 3 | Rainbow Chase | rainbow arc / chase arrows |
| 4 | Comet / Meteor | comet with trail |
| 5 | Breathing Pulse | expanding circle / lungs |
| 6 | Fire Flicker | flame |
| 7 | Sparkle / Twinkle | stars / sparkles |
| 8 | Wave / Undulate | sine wave |
| 9 | Solid / Static | solid filled square / droplet |
| 10 | Off / Blackout | power / moon |

### Future commands (not yet in firmware – reserve space in UI)
| Command | Format | Notes |
|---------|--------|-------|
| Color | `C<r>,<g>,<b>` | 0–255 each |
| Theme | `T<name>` | e.g. fire, ice, rainbow |
| Preset | `P<n>` | Load user preset |
| Save | `W` | Write current settings |

---

## 4. Status / Response Protocol (Suit → App)

Responses arrive as **notifications** on the TX characteristic (UTF-8 text).

### Live telemetry (preferred for UI meters)
While connected, the Tail pushes ~2 times per second:

```text
STAT M:3 B:80 V:50 S:75 E:1 Mic:1423 HeadB:512 HeadT:36.5
```

Parse any line starting with `STAT` (space-separated `Key:Value` tokens).

| Token | Meaning |
|-------|--------|
| `M` | Current mode |
| `B` | Master brightness 0–100 |
| `V` | Animation speed 0–100 |
| `S` | Sensitivity |
| `E` | Sound enable 0/1 |
| `Mic` | Last peak mic level |
| `HeadB` | Head light sensor |
| `HeadT` | Head temperature °F |

### Full status dump (`?`)
Multi-line help + same key fields. Still useful for debug.

Also accept short confirmations such as `Mode=3`, `Brightness=80`, `Speed=50`, `Flash Lamp!`, `Resync...`.

---

## 5. Recommended App Structure (UI)

### Screens / Sections
1. **Connect** – scan + device list + connection status
2. **Control (main)**
   - Large mode selector (grid or horizontal carousel with icons)
   - Brightness slider (`B`)
   - Speed slider (`V`)
   - Sensitivity slider (`S`)
   - Sound enable toggle (`E` / `e`)
   - Quick actions: Flash (`L`), Resync (`R`)
3. **Status** – live mic level, head temp/brightness from `STAT`, connection RSSI
4. **Settings** – branding, about, reconnect behaviour, (future presets)

### Interaction rules
- Every slider / toggle writes the corresponding command immediately (or debounced 100–150 ms).
- Prefer parsing continuous `STAT` lines over polling `?`.
- Show a subtle activity indicator when `Mic` is high.
- Offline / disconnected state must be obvious (greyed controls + reconnect CTA).

---

## 6. Icon Set Required

Produce a consistent icon set (preferably SVG or high-res PNG, dark-theme friendly):

**Core actions**
- Connect / Bluetooth
- Flash
- Resync / Sync
- Power / Off
- Settings
- Dragon / TMDrake logo mark

**Modes (11 icons)**
- Sound Phase, Sound Distinct, VU Meter, Rainbow Chase, Comet, Breathing, Fire, Sparkle, Wave, Solid, Off / Blackout

**Sliders / status**
- Brightness, Speed, Mic / Sensitivity, Temperature (optional)

Style: rounded, slightly aggressive geometry, purple/cyan dual-tone, good contrast on near-black.

---

## 7. Non-Goals (for v1)

- Controlling Head or PAWB directly over BLE (everything goes through the Tail)
- Custom firmware OTA
- Multiple simultaneous suits (single connection is fine for v1)
- iOS-specific optimisations beyond basic Flutter compatibility

---

## 8. Versioning & Compatibility

- Firmware that implements this contract reports modes 0–10, accepts `B` / `V`, and emits `STAT` lines.
- App should degrade gracefully if an older firmware lacks `STAT` or `B`/`V` (fall back to `?`).
- Future protocol additions will be additive; existing single-letter commands will not change meaning.

---

*Contract maintained alongside DRAKE_2_0_TAIL firmware.*  
*System-wide docs: [SYSTEM.md](SYSTEM.md)*
