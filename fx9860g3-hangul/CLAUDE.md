# fx-9860G III Hangul notes add-in

Builds CASIO fx-9860G III add-ins (`HanML.g1a`, `HanProd.g1a`, ... one per subject/note set) that show Korean notes.

## Common tasks

- **Build the add-in (Windows, via WSL2 Ubuntu)**: `powershell -ExecutionPolicy Bypass -File tools\windows\build_windows.ps1`
  Output: `dist\<name>.g1a`, one per note set (`python3 tools/noteset.py list`). The toolchain (sh-elf-gcc, fxSDK, gint) is already installed in WSL after
  the first run, so later builds take a few minutes. On failure, show the last ~30 lines and the `[n/5]` step.
- **Check notes before building**: `python3 tools/check_notes.py` (all sets; must report 0 errors).
- **Split files longer than 9 pages**: `python3 tools/split_notes.py --set <id> [--write]` (also fixes links).
- **Regenerate data for one set**: `python3 tools/build_assets.py --set <id>` (writes `src/font_data.c`, `src/notes.c`,
  `src/addin.cmake`; these are gitignored because they depend on the set).
- **Deliver the result**: send the user the `dist\*.g1a` files; they copy it to the calculator in
  Mass Storage mode (`MENU` -> `LINK`).

## Layout

- `sets/<id>/set.json` (`name` = add-in name, <= 8 ASCII chars) + `sets/<id>/notes/<NN_folder>/<NN_file>.txt` -
  exam notes of one subject; pages are split by a `---` line; names start with a two-digit number used for
  number-key jumps. Folder `00_색인` is the index (F2 jumps to it). Links never cross sets.
- Links inside notes: `→folder/file` or `→folder/file:page`. Pages with links are menu pages
  (UP/DOWN select, EXE follows). Link pages must keep every line <= 122 px, other pages <= 128 px.
- `src/` - add-in source (gint). `tools/sim/` - PC simulator (stub gint) used to test navigation:
  see README ("PC 에서 동작 시뮬레이션").
- `docs/PROMPT.md` - instructions for the Claude project that writes the notes.

## Rules of thumb

- Never edit `src/font_data.c`, `src/notes.c`, `src/addin.cmake`, `src/fakecalc_data.c` by hand (generated).
- Screen is 128x64 px, 7px Korean bitmap font (Galmuri7): 6 lines per page, 9 pages max per file.
- The git host for fxSDK moved to `git.planet-casio.com` (the old `gitea.planet-casio.fr` no longer resolves).
