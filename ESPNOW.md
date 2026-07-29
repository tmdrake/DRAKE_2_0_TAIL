# Drake 2.0 – Encrypted ESP-NOW (Tail ↔ Head)

**Status:** Implemented July 2026  
**Purpose:** Lower-latency, encrypted link for mic stream, commands, and Head sensor feedback.

PAWB claws still use **ASK** (unchanged). BLE app control still talks only to the Tail.

---

## 1. Why ESP-NOW

| Old path | New path |
|----------|----------|
| UDP over SoftAP/STA IP stack | MAC-layer ESP-NOW frames |
| Higher latency / jitter | Typically lower latency |
| Ports 1234–1237 | Typed binary packets |
| No payload encryption | AES with PMK + LMK |

ESP-NOW is **bidirectional** and **best-effort** (UDP-like, not TCP). Occasional drops are OK for mic samples; commands are small and can be repeated if needed.

---

## 2. Channel & coexistence

- Head SoftAP is fixed on **WiFi channel 2**.
- ESP-NOW on both boards uses **channel 2**.
- Tail still joins SoftAP as STA when possible (same channel automatically).
- ESP-NOW can work even if STA association is slow; peer MAC + channel matter more than IP.

---

## 3. Encryption

| Key | Value (16 bytes) | Role |
|-----|------------------|------|
| **PMK** | `TMDrakePMK_2026!` | Primary master key (both boards) |
| **LMK** | `TMDrakeLMK_2026!` | Local master key for this peer pair |

- Encryption is **enabled** on the peer (`encrypt = true` / LMK passed into `add_peer`).
- Change both keys on **Tail and Head** together if you want a private suit key.
- Unencrypted peers will not interoperate with these builds.

---

## 4. Packet format

First byte = type, then payload.

| Type | Name | Payload | Direction |
|------|------|---------|-----------|
| `0x01` | **MIC** | `int16` big-endian level | Tail → Head |
| `0x02` | **CMD** | ASCII e.g. `M6`, `L0`, `R0` | Tail → Head |
| `0x03` | **LIGHT** | `uint16` big-endian CDS reading | Head → Tail |
| `0x04` | **TEMP** | `int16` big-endian (°F × 10) | Head → Tail |

### Mic value source (analog — unchanged)

```text
raw   = analogRead(MIC)          // ESP32 ADC, pin A0
level = raw - OFFSET + sensitivity
OFFSET = 1600                    // bias / mid-rail of mic preamp (ADC counts)
```

Only sent when `|level - lastSent| >= 8` (or level == 0) to save airtime.

---

## 5. MAC pairing (bench required)

ESP-NOW encryption needs **known peer MACs** (not “just same SSID”).

### Procedure

1. Flash **Head** and **Tail** with current firmware.  
2. Open **Serial Monitor @ 115200** on both.  
3. Each board prints its MAC address at boot.  
4. Edit:
   - Tail `EspNowCom.ino` → `HEAD_PEER_MAC[]` = Head’s MAC  
   - Head `EspNowCom.ino` → `TAIL_PEER_MAC[]` = Tail’s MAC  
5. Format example:
   ```cpp
   uint8_t HEAD_PEER_MAC[6] = {0x24, 0x6F, 0x28, 0xAA, 0xBB, 0xCC};
   ```
6. Reflash both.  
7. Serial should report `ESP-NOW ready (encrypted)`.

Until MACs are set, `espnowReady` stays false; UDP fallback still works for bring-up.

**Note:** On ESP8266, SoftAP MAC and STA MAC can differ — use the MAC printed by the Head firmware log. If peer fails, try the other printed MAC.

---

## 6. Files

### Tail (`DRAKE_2_0_TAIL`)
| File | Role |
|------|------|
| `EspNowCom.ino` | Init, encrypt, send mic/cmd, recv light/temp |
| `sound_activate.ino` | Analog mic → `espnowSendMic()` |
| `Serial_RoutineBT.ino` | BLE cmds → `forwardCmd()` → ESP-NOW + ASK |
| `DRAKE_2_0_TAIL.ino` | Calls `setupEspNow()` after WiFi |

### Head (`DRAKE_2_0_HEAD`)
| File | Role |
|------|------|
| `EspNowCom.ino` | Init, encrypt, recv mic/cmd, send light/temp |
| `DRAKE_2_0_HEAD.ino` | SoftAP ch2, `setupEspNow()`, sensor timers |

---

## 7. Bench test checklist

- [ ] Both Serial logs show MACs  
- [ ] Peer MACs entered and boards reflashed  
- [ ] Both report ESP-NOW ready  
- [ ] Speak into Tail mic → Head `micLevel` drives animation (watch spikes / Serial)  
- [ ] BLE `M3` / `M6` → Head mode changes (Serial “ESP-NOW Mode:”)  
- [ ] Head light/temp appear in Tail BLE `STAT` as `HeadB` / `HeadT`  
- [ ] PAWB still follows modes via ASK  
- [ ] Optional: confirm UDP still works if ESP-NOW peer missing (fallback)

### Debugging tips

| Symptom | Check |
|---------|--------|
| Init OK but no mic on Head | Peer MAC wrong, or different channels |
| Add peer failed | MAC still `FF:FF:…`, or key mismatch |
| Encrypt fail | PMK/LMK must be identical 16-byte strings on both |
| Works without SoftAP client | OK for ESP-NOW; STA is optional for IP fallback |
| Mic stuck at 0 | Analog wiring / OFFSET; plot `analogRead(A0)` on Tail |

---

## 8. What did **not** change

- Analog mic hardware path and `OFFSET 1600`  
- ASK Tail → PAWB (modes + `m####` mic)  
- BLE NUS app protocol  
- NeoPixel modes on Tail/PAWB  

---

## 9. Reliability notes

- No TCP-style guarantees.  
- Mic: drop late samples rather than queue.  
- Commands: short; resend from app if needed.  
- Radio CCA still happens under WiFi; software does not implement a separate collision protocol.

---

## 10. Rollback

If ESP-NOW misbehaves on the bench:
1. Leave peer MAC as `FF:FF:…` so `espnowReady = false`.  
2. UDP paths remain in firmware for mic (commented on Tail send; active receive on Head) and sensors.  
3. Or re-enable the commented `udp.writeTo` mic send in Tail `sampleaudio()`.

---

*Keep keys and MAC procedure in sync across Tail and Head when changing either side.*
