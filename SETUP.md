# Developer setup

The graphical emulator needs CMake, Ninja, SDL2, and SDL2_ttf. SDL2_ttf must be built with HarfBuzz support (true of all mainstream distro packages) for correct Devanagari/Gujarati shaping (conjuncts, reph, matra reordering). Fonts are bundled in `assets/fonts/` -- no system font install is required for the app to run correctly.

Run the appropriate prerequisite installation yourself:

| System | Command |
| --- | --- |
| Arch | `sudo pacman -S --needed base-devel cmake ninja sdl2 sdl2_ttf python qemu-system-arm` |
| Ubuntu | `sudo apt install build-essential cmake ninja-build libsdl2-dev libsdl2-ttf-dev python3 python3-venv qemu-system-arm` |
| Fedora | `sudo dnf install gcc cmake ninja-build SDL2-devel SDL2_ttf-devel python3 qemu-system-arm` |
| macOS | `brew install cmake ninja sdl2 sdl2_ttf python qemu` |
| Windows (MSYS2 UCRT64) | `pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-SDL2 mingw-w64-ucrt-x86_64-SDL2_ttf python` |

## Build, test, and run

The normal build/test loop is network-free and needs no Python packages beyond the standard library:

```bash
chmod +x scripts/*.sh
./scripts/test.sh
./build/best-kiosk
```

Only left mouse clicks are accepted as touch input. Map zoom is two discrete `+`/`-` buttons over the map; tapping empty map area re-centers the view.

## Changing the route catalogue

`config/routes.json` is the committed, human-reviewable source of truth; `tools/generate_catalog.py` (network-free, stdlib-only) turns it into `generated/routes_generated.h` at build time -- this already runs automatically as part of `scripts/build.sh`.

To refresh `config/routes.json` itself from the real BEST GTFS feed (an occasional maintainer task, not needed for normal builds), use an isolated venv since this script has real dependencies (GTFS download + transliteration library):

```bash
python3 -m venv .venv
source .venv/bin/activate          # Windows: .venv\Scripts\activate
pip install -r tools/requirements.txt
python3 tools/import_gtfs.py
deactivate
```

This needs network access (downloads `gtfs_compat.zip` from `croyla/mumbai-gtfs` into a local `.cache/gtfs/`, gitignored) and takes a few seconds. Run its own tests with `python3 -m unittest tools/test_import_gtfs.py` (also needs the venv).

## Changing the satellite basemap

`assets/mumbai_satellite.png` and the geographic bounds it covers (`config/map_bounds.json`) are refreshed with `tools/fetch_basemap.py` (same venv as above; needs `magick`/ImageMagick on PATH to convert the fetched JPEG to PNG). This is also a maintainer-only, occasional step -- the app itself never touches the network.

## Changing language strings

Add or edit fixed UI text in `src/i18n.c` (one array per language: `EN`, `MR`, `HI`, `GU`); every `TextKey` must have an entry in all four arrays, or `tests/test_i18n.c` will fail. Note: Noto Sans Gujarati's bundled font has essentially no ASCII punctuation (confirmed via `fc-query`) -- avoid plain `,`/`:`/`-` in Gujarati strings; use the danda (`।`, U+0964) and Unicode hyphen (`‐`, U+2010) instead, as the existing `GU` array does.

Adding a fifth language needs: a new `LANG_*` enum value in `include/i18n.h`, a new string array in `src/i18n.c`, a bundled font covering its script in `assets/fonts/` (loaded in `src/ui_sdl.c`'s `ui_run()`), and `name_<lang>` fields added to `include/catalog.h`'s `Stop`/`Route` structs and threaded through `tools/generate_catalog.py` and `config/routes.json`.
