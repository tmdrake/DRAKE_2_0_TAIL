# App Team – What you can implement now

**Firmware updated July 2026 for issue #6 (C + T).**

## Fully supported (wire to UI, no longer optimistic)

| Feature | Command | Notes |
|---------|---------|-------|
| Modes 0–10 | `M0`–`M10` | Whole suit |
| Brightness / Speed | `B` / `V` | |
| Mic S / G / A | `S` `G` `A` | + live Mic meter |
| Sound toggle | `E` / `e` | |
| **Custom RGB** | `C<r>,<g>,<b>` | → mode 9, persisted |
| **Themes** | `T0`–`T4` or `Tpurple`… | → mode 9, persisted |
| Flash / Resync / Reboot | `L` `R` `Z` | |
| Fan | `F0` `F1` `F2` `FT` | |
| Eyes / CDS | `I` `D` | |
| Live STAT | includes `C:` and `T:` | sync picker after connect |

### Theme circles → send

| Circle | Command |
|--------|---------|
| Purple | `T0` or `Tpurple` |
| Fire | `T1` or `Tfire` |
| Ice | `T2` or `Tice` |
| Gold | `T3` or `Tgold` |
| Emerald | `T4` or `Temerald` |
| Custom HSV | `C<r>,<g>,<b>` |

After connect, parse `STAT` `C:r,g,b` and `T:n` to restore the picker.

## Still firmware backlog (optional later)

- Mic auto-calibrate / decay knobs  
- User presets `P` / `W`  
- Flash duration  
- Per-mode color tint beyond Solid  

## Screens

Control · Status · Settings — as in your v0.2 README. Contract: [APP_INTERFACE.md](APP_INTERFACE.md) v1.5.
