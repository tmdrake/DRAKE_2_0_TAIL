# Drake Tail tools

## `tail_tui.py` — USB settings TUI

Nice terminal UI over the **same commands as the phone app** (via USB serial).

![Drake Tail TUI screenshot](../docs/tail_tui.svg)

### Setup

```bash
cd DRAKE_2_0_TAIL
pip3 install -r tools/requirements.txt
```

### Run

```bash
# Auto-pick first USB serial (CP2102)
python3 tools/tail_tui.py

# Explicit port
python3 tools/tail_tui.py -p /dev/cu.usbserial-0001

# List ports
python3 tools/tail_tui.py -l
```

**Firmware:** Tail must include `checkSerialBT()` (USB lines → `processBLECommand`).  
Close `screen` / Serial Monitor before starting (port exclusive).

### If USB serial feels “locked”
macOS only allows one process on the port. A crashed TUI / `screen` can hold it.

```bash
# See who owns the port
lsof /dev/cu.usbserial-0001

# TUI helper: SIGTERM anything holding the port
python3 tools/tail_tui.py --release
python3 tools/tail_tui.py --release -p /dev/cu.usbserial-0001
```

The TUI now:
- Opens with **`dsrdtr=False` / `rtscts=False`** (avoids ESP32 download-boot glitches)
- Uses **exclusive** open when the OS supports it
- **Always closes** the port on `q`, Ctrl+C, or crash (`atexit` + signal handlers)

### Keys

| Key | Action |
|-----|--------|
| `0`–`9` / `a` | Mode 0–10 |
| `[` `]` | Brightness −/+ |
| `;` `'` | Speed −/+ |
| `,` `.` | Mic sensitivity −/+ |
| `e` | Sound detect toggle |
| `t` | Next theme |
| `f` | Fan Off → On → Auto |
| `l` | Flash |
| `r` | Resync |
| `h` | Heartbeat / STAT |
| `p` | Solid purple `150,0,255` |
| `?` | Firmware help |
| `q` | Quit |

Live panel: mode, B/V/mic, HeadT/HeadB, HB seq, recent log.
