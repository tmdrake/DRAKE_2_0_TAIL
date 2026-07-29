# TMDrake Settings – App Team

**Firmware July 2026** · Modes 0–10 on whole suit · Color `C` supported

## Sound

| UI | Cmd | Default | Range |
|----|-----|---------|-------|
| Sound reactive | `E`/`e` | on | toggle |
| Gain % | `A` | 100 | 50–300 |
| Sensitivity | `S` | 75 | 0–500 UI |
| Gate | `G` | 100 | 20–800 UI |
| Mic meter | — | — | `STAT Mic:` |

```text
level = max(0, ADC-1600) * Gain/100 + Sensitivity → EMA → Gate
```

## Lighting (main Control, not only Settings)

| UI | Cmd |
|----|-----|
| Mode 0–10 | `M` |
| Brightness | `B0-100` |
| Speed | `V0-100` |
| Solid color | `C<r>,<g>,<b>` |
| Flash / Resync | `L` / `R` |

## Fan (Head)

| UI | Cmd |
|----|-----|
| Off On Auto | `F0` `F1` `F2` |
| Threshold °F | `FT` |
| Temp | `STAT HeadT:` |

## Eyes / CDS (Head)

| UI | Cmd |
|----|-----|
| Dim when ≥ | `I` |
| Dim % | `D` |
| Light | `STAT HeadB:` |

## Layout

```text
Settings
├── Sound (E, A, S, G, Mic meter)
├── Fan (F*, FT, HeadT)
├── Eyes / light (I, D, HeadB)
└── System (Z, about)
```

Control screen owns mode grid + color picker for Solid.
