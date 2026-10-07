# Tech stack

| Layer | Choice | Purpose |
| --- | --- | --- |
| Firmware core | ISO C17 | Portable, bounded application logic (`catalog`, `booking`, `i18n`, `mapview`). |
| UI emulator | SDL2 + SDL2_ttf (HarfBuzz-enabled) | Graphical framebuffer, mouse-as-touch input, complex-script text shaping. |
| Fonts | Noto Sans / Noto Sans Devanagari / Noto Sans Gujarati (OFL-1.1), bundled in `assets/fonts/` | Correct, portable rendering for English, Marathi, Hindi, Gujarati -- independent of what's installed on the host/target. |
| Basemap | Sentinel-2 cloudless (EOX, CC BY-NC-SA 4.0), fetched once via `tools/fetch_basemap.py` | Offline satellite imagery; the app itself never touches the network. |
| Route/stop data | `croyla/mumbai-gtfs` (MIT-0), imported once via `tools/import_gtfs.py` | Real BEST route/stop catalogue instead of hand-authored mock data. |
| Configuration | JSON + Python standard library | Editable routes/map-bounds, generated into C at build time (network-free). |
| Build/test | CMake, Ninja, CTest | Reproducible build and unit tests (one executable per test module). |
| Future board target | QEMU `mps2-an385` | Cortex-M3 target once a selected display/touch HAL is added. |

SDL is the development display emulator. A QEMU `mps2-an385` board has no universal touchscreen/framebuffer device, so a specific display-controller HAL is deliberately kept outside the tested booking core. The current dataset (473 routes, fixed-size arrays sized by `MAX_ROUTES`/`MAX_STOPS` in `include/catalog.h`) targets this SDL2 desktop prototype; a real constrained-MCU target would need to re-evaluate that memory budget (or stream/shard the catalogue) before porting.
