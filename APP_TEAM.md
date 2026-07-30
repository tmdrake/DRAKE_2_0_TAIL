# App Team – What you can implement now

**Firmware July 2026 · Contract v1.6**

## Heartbeat (do this)

```text
App  --HB-->  Tail
App  <--HBACK Seq:n U:sec--
App  <--STAT … full snapshot--
```

- Send **`HB`** every **2–5 s** while connected  
- Send **`HB`** once right after notify subscription  
- On **`HBACK`**: link OK  
- On **`STAT`**: sync **all** UI (mode, B/V, mic, color, theme, HeadT/B)  
- If no `HBACK` for **>10 s**: show reconnect banner  

This is the supported path for **reliable data sync** after backgrounding or missed notifies.

## Fully supported UI

| Feature | Command |
|---------|---------|
| Modes 0–10 | `M0`–`M10` |
| Brightness / Speed | `B` / `V` |
| Mic S/G/A + toggle | `S` `G` `A` `E`/`e` |
| Color / themes | `C<r>,<g>,<b>` · `T0`–`T4` / names |
| **Heartbeat** | **`HB`** |
| Flash / Resync / Reboot | `L` `R` `Z` |
| Fan / Eyes | `F*` `FT` `I` `D` |

## Theme circles

`T0`/`Tpurple` · `T1`/`Tfire` · `T2`/`Tice` · `T3`/`Tgold` · `T4`/`Temerald` · custom `C…`

## STAT tokens to parse

`M B V S G A E C T Mic HeadB HeadT U Seq`

---

[APP_INTERFACE.md](APP_INTERFACE.md) is source of truth.
