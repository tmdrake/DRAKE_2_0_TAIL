# Drake repos – contents & publishing notes

**Audience:** humans, firmware/app agents, CI  
**Applies to:** `DRAKE_2_0_TAIL`, `DRAKE_2_0_HEAD`, `DRAKE_2_0_PAWB`, `DRAKE_2_0_APP`

---

## What these repos are for

Today they are mostly **text**:

- Arduino / ESP firmware (`.ino` and related source)
- Markdown docs (architecture, BLE contract, settings)
- Config / ignore files

That is the **current shape**, not a hard ban on other file types.

### Allowed and expected over time

| Kind | OK in git? | Notes |
|------|------------|--------|
| Source (`.ino`, `.h`, `.cpp`, Dart, etc.) | **Yes** | Primary content |
| Docs (`.md`) | **Yes** | |
| **Libraries required for the build** | **Yes** | Vendored libs, `lib/`, PlatformIO `lib/`, Flutter packages that must be pinned in-tree |
| **Assets needed to build or run** | **Yes** | Icons, fonts, badge PNGs under `assets/` when the app or firmware build needs them |
| Screenshots / datasheets | **Yes** | Useful for README and hardware reference |
| Compiled binaries (`.bin`, `.hex`, `.apk`) | Optional | Prefer **GitHub Releases** for large/frequent build products; fine in-repo if a specific workflow needs them |

**There is no project rule that “binaries must never be committed.”**  
Prefer source + docs; **add binary or library files when the build, branding, or documentation needs them.**

Wire protocols that use binary **packets** (e.g. ESP-NOW mic frames) are documented and implemented as **source code** — that is normal and expected.

---

## Agent / automation limitation (important)

Some automated GitHub integrations (including the file-push tools used by chat agents) only handle **text** payloads well.

| Action | Result |
|--------|--------|
| Push `.ino`, `.md`, `.dart`, `.yaml` via agent tools | Works |
| Push **PNG / JPG / PDF / ZIP / APK** via those same tools | Often **fails or corrupts** — tooling limit, **not** repo policy |

### How to add images and other binaries correctly

Use a normal git client or the GitHub UI on a machine that has the files:

```bash
git add path/to/image.png path/to/lib/...
git commit -m "Add assets required for UI / build"
git push
```

Or: GitHub → **Add file** → **Upload files**.

Agents should:

1. **Not** invent a “no binaries in the repo” policy from a failed PNG upload.  
2. Document paths where assets belong (e.g. `docs/screenshots/`, `assets/`).  
3. Leave a short note for a human to `git add` the binaries when needed.  
4. Vendoring **libraries required for the build** is encouraged when dependency fetch is unreliable or versions must be frozen.

---

## Suggested layout (when assets appear)

```text
DRAKE_2_0_TAIL/          # firmware + system docs (this repo is the hub)
DRAKE_2_0_HEAD/
DRAKE_2_0_PAWB/
DRAKE_2_0_APP/
  assets/                # badge, icons (PNG) — commit when branding is ready
  docs/screenshots/      # UI captures — commit when available
  lib/                   # app source
```

Firmware-side third-party code, if vendored:

```text
lib/                     # or src/ for PlatformIO
```

---

## Summary

- **Default:** `.ino` / text / markdown.  
- **Later:** libraries and binary assets that support **build, branding, or docs** are welcome.  
- **Failed image publishes via agent tools** = use local `git` or GitHub Upload — do not treat that as a ban on binaries.

*Documented July 2026 so future agents do not misread tool limits as project policy.*
