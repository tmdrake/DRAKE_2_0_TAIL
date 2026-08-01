# Drake 2.0 – ESP-NOW (Tail ESP32 ↔ Head ESP8266)

**Status:** August 2026 (hard-won bench notes)  
**Purpose:** Low-latency link for mic, commands, phase sync, and Head sensor feedback.

PAWB claws still use **ASK**. BLE app control still talks only to the Tail.

---

## BLAME: ESP-NOW / ESP8266 SoftAP gotchas

These cost us real bench time. Documented so we don’t re-learn them.

### 1. SoftAP MAC ≠ STA MAC (ESP8266) — **#1 silent killer**

On ESP8266 in SoftAP mode, **`WiFi.macAddress()` (STA) is not the SoftAP BSSID**.

Example from our suit:

| Address | Value | Use |
|---------|--------|-----|
| Head **STA** MAC (often printed first) | `9C:9C:1F:46:82:33` | Wrong for Tail peer |
| Head **SoftAP** BSSID | `9E:9C:1F:46:82:33` | **Correct** Tail `HEAD_PEER_MAC` |

First octet often differs by the “locally administered” bit (`9C` → `9E`).

**Symptom:** Tail logs “joined TMDRAKE ch=2” and “ESP-NOW ready”, but Head never changes mode / no `ESP-NOW CMD:` on Head serial.  
**Cause:** Peer was the STA MAC; SoftAP radio identity is the SoftAP MAC.  
**Fix (current firmware):** Tail, after STA join, sets peer from **`WiFi.BSSID()`** (live SoftAP MAC). Head boot prints both:

```text
STA MAC:     ...
SoftAP MAC:  ...   ← what Tail must use if hardcoding
```

**Blame:** ESP8266 WiFi dual-interface MAC model + ESP-NOW requiring the *exact* interface MAC — not a Drake logic bug.

### 2. Encrypted ESP-NOW ESP32 ↔ ESP8266 is unreliable

| Stack | API |
|-------|-----|
| ESP32 Arduino 3.x | `esp_now_set_pmk` + `peer.encrypt` + LMK |
| ESP8266 Arduino | `esp_now_set_kok` + `add_peer(..., key, 16)` |

Keys can match byte-for-byte and still fail **silently** across chips/IDF vs non-OS SDK.

**Current policy:** `#define ESPNOW_ENCRYPT 0` on **both** Tail and Head.  
Keys remain in source for same-chip / future use; peer is **unencrypted**.  
Suit traffic is short-range SoftAP-adjacent; AES was not worth the dead link.

**Blame:** Cross-architecture ESP-NOW crypto interoperability, not “wrong password” in our strings.

### 3. Peer channel ≠ home channel

ESP32 refuses send with:

```text
E (…) ESPNOW: Peer channel is not equal to the home channel, send fail!
```

Head SoftAP is fixed on **channel 2**. Tail must use the same home channel.

**Fix:** `WiFi.begin(ssid, NULL, 2)`, `ensureEspNowChannel()` before send, peer `channel = 0` (follow home).

**Blame:** ESP-NOW channel rules + STA roaming/scan leaving radio off ch 2.

### 4. Hidden SoftAP + broadcast UDP

- Hidden SSID (`softAP(..., ssid_hidden=true)`) made STA join flaky.  
- `udp.broadcastTo(..., 255.255.255.255)` often **does not** hit SoftAP host.

**Fix:** SoftAP **visible**; UDP **unicast** to `192.168.4.1:1234` (+ subnet broadcast `192.168.4.255`).

**Blame:** SoftAP isolation / broadcast semantics on ESP8266 AP.

### 5. “Ready” does not mean “linked”

Both sides can print ESP-NOW ready while:

- Wrong peer MAC  
- Encrypt mismatch  
- No packets received  

**Verify on Head serial when changing modes:**

```text
ESP-NOW CMD: M5
```
or
```text
UDP CMD: M5
```

No line → nothing arrived (MAC/encrypt/channel/path).

### 6. Library / core churn

- ESP32 Arduino **3.x** changed ESP-NOW callback signatures (`esp_now_recv_info_t`, send info).  
- `esp_task_wdt_init` API changed.  
- NimBLE 2.x: `Service::start()` no-op; connect/write callbacks need `NimBLEConnInfo`.

**Blame:** Upstream Arduino-ESP32 / NimBLE API breaks, not suit app protocol.

---

## Current link design (what we ship)

```text
Phone BLE ──► Tail (ESP32)
                │
                ├─ ESP-NOW (unencrypted peer, SoftAP BSSID, ch 2)
                │     MIC / CMD / PHASE ──► Head
                │     LIGHT / TEMP ◄── Head
                │
                ├─ UDP unicast 192.168.4.1:1234 (CMD fallback)
                │
                └─ ASK ──► PAWB claws
```

| Setting | Value |
|---------|--------|
| Channel | **2** (Head SoftAP + Tail lock) |
| Encrypt | **OFF** (`ESPNOW_ENCRYPT 0`) |
| Head peer on Tail | **SoftAP BSSID** after join (fallback hardcoded) |
| Tail peer on Head | Tail STA MAC `24:0A:C4:81:4A:B0` |
| SoftAP SSID | `TMDRAKE` (visible) |
| TX | Tail `WIFI_POWER_19_5dBm`; Head `setOutputPower(20.5)` |

---

## Packet format

| Type | Name | Payload | Direction |
|------|------|---------|-----------|
| `0x01` | **MIC** | `int16` BE level | Tail → Head |
| `0x02` | **CMD** | ASCII e.g. `M6` | Tail → Head |
| `0x03` | **LIGHT** | `uint16` BE CDS | Head → Tail |
| `0x04` | **TEMP** | `int16` BE (°F × 10) | Head → Tail |
| `0x05` | **PHASE** | `uint16` BE phase + `uint8` mode | Tail → Head ~25 Hz |

**PHASE** locks rainbow / comet / breathe / wave (~100 B/s). Head free-runs if no PHASE >250 ms.

Mic: only when `|level - last| >= 8` (or 0).

---

## Files

| Board | File | Role |
|-------|------|------|
| Tail | `EspNowCom.ino` | Channel lock, BSSID peer, send MIC/CMD/PHASE, recv sensors |
| Tail | `Serial_RoutineBT.ino` | BLE → `forwardCmd` (ESP-NOW + UDP unicast + ASK) |
| Tail | `New_Modes.ino` | `suitPhase` master + `pushPhaseSync()` |
| Head | `EspNowCom.ino` | Recv CMD/MIC/PHASE, send LIGHT/TEMP; print STA + SoftAP MAC |
| Head | `DRAKE_2_0_HEAD.ino` | SoftAP ch2 visible, UDP CMD log |

---

## Bench checklist

- [ ] Head: SoftAP MAC + STA MAC printed; note **SoftAP** for any hardcode  
- [ ] Tail: `joined TMDRAKE ch=2` + `BSSID=…` + `peer Head MAC` **matches SoftAP**  
- [ ] Both: `encrypt=OFF` (or both ON only if same-chip)  
- [ ] Head serial shows `ESP-NOW CMD:` or `UDP CMD:` on mode change  
- [ ] `M5` / `M3` — phase looks locked  
- [ ] `STAT HeadT` / `HeadB` move when sensors OK  
- [ ] PAWB still follows ASK  

### Debug table

| Symptom | Likely cause |
|---------|----------------|
| WiFi joined, no mode on Head | Wrong peer MAC (STA vs SoftAP) — see BLAME §1 |
| Encrypt on, nothing moves | Cross-chip crypto — set `ESPNOW_ENCRYPT 0` both sides |
| Channel send fail log | Tail not on ch 2 — `ensureEspNowChannel` |
| Only UDP works | ESP-NOW peer wrong; unicast UDP still OK |
| HeadT stuck | Temp bus / re-probe — not ESP-NOW keys |

---

## What did not change

- Analog mic + OFFSET 1600  
- ASK Tail → PAWB  
- BLE NUS app protocol  
- Mode IDs 0–10  

---

*If it “looks connected” (SSID joined) but LEDs don’t match — check SoftAP MAC and encrypt OFF before rewriting modes.*
