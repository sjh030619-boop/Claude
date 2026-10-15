# fx-9860G III Hangul notes add-in

Builds `Hangul.g1a` (a CASIO fx-9860G III add-in that shows Korean notes) from `notes/*.txt`.

## Common tasks

- **Build the add-in (Windows, via WSL2 Ubuntu)**: `powershell -ExecutionPolicy Bypass -File tools\windows\build_windows.ps1`
  Output: `dist\Hangul.g1a`. The toolchain (sh-elf-gcc, fxSDK, gint) is already installed in WSL after
  the first run, so later builds take a few minutes. On failure, show the last ~30 lines and the `[n/5]` step.
- **Check notes before building**: `python3 tools/check_notes.py` (must report 0 errors).
- **Regenerate data only**: `python3 tools/build_assets.py` (writes `src/font_data.c`, `src/notes.c`).
- **Deliver the result**: send the user `dist\Hangul.g1a`; they copy it to the calculator in
  Mass Storage mode (`MENU` -> `LINK`).

## Layout

- `notes/<NN_folder>/<NN_file>.txt` - exam notes; pages are split by a `---` line; names start with a
  two-digit number used for number-key jumps. `notes/00_색인/` is the index folder (F2 jumps to it).
- Links inside notes: `→folder/file` or `→folder/file:page`. Pages with links are menu pages
  (UP/DOWN select, EXE follows). Link pages must keep every line <= 122 px, other pages <= 128 px.
- `src/` - add-in source (gint). `tools/sim/` - PC simulator (stub gint) used to test navigation:
  see README ("PC 에서 동작 시뮬레이션").
- `docs/PROMPT.md` - instructions for the Claude project that writes the notes.

## Rules of thumb

- Never edit `src/font_data.c`, `src/notes.c`, `src/fakecalc_data.c` by hand (generated).
- Screen is 128x64 px, 7px Korean bitmap font (Galmuri7): 6 lines per page, 9 pages max per file.
- The git host for fxSDK moved to `git.planet-casio.com` (the old `gitea.planet-casio.fr` no longer resolves).
