# Drake Tail tools

## `tail_tui.py` — USB settings TUI

Nice terminal UI over the **same commands as the phone app** (via USB serial).

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
