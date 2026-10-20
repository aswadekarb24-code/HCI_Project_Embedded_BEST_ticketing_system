# Architecture

```text
Mouse click -> SDL2 UI/controller -> booking state machine -> printer stock check
                     |        |            |                       |
              i18n strings  camera    generated route data        ticket/error
              (4 languages) (mapview)  (real BEST GTFS data)
```

Input, drawing, business rules, and configuration are separate. `booking.c`, `catalog.c`, `i18n.c`, `mapview.c`, and `search.c` have no graphics dependency and are unit tested (`tests/test_*.c`, one executable + CTest entry per module). The simulated printer sensor is the adapter point for a GPIO printer driver.

## Data pipeline

Two independent, maintainer-only/occasional/network-using scripts feed the network-free build:

```text
croyla/mumbai-gtfs ---tools/import_gtfs.py---> config/routes.json --+
                                                                     |
EOX s2cloudless WMS ---tools/fetch_basemap.py--> assets/mumbai_satellite.png
                                                  config/map_bounds.json ----+
                                                                             |
                                        tools/generate_catalog.py (network-free, runs in scripts/build.sh)
                                                                             |
                                             generated/routes_generated.h, generated/map_bounds_generated.h
```

`config/routes.json` and `config/map_bounds.json` are the committed, human-reviewable source of truth; the `generated/` headers are fixed-size C arrays/structs, so the firmware path uses no parser or dynamic allocation regardless of how large the real dataset gets (`MAX_ROUTES`/`MAX_STOPS` in `include/catalog.h` are the only tunable bound).

`src/mapview.c` is a pure-logic camera/projection module (geo-to-pixel projection, discrete zoom levels, viewport clamping) shared conceptually with `tools/import_gtfs.py`'s Python-side projection -- both must agree on `config/map_bounds.json`'s bounding box, or stop dots and the basemap image drift apart.

The map deliberately never draws all 473 routes' colored lines at once (it always shows every stop as a small circle, and only highlights the currently *selected* route's polyline -- stops as larger circles joined by a thickened line, start/end picked out in green/orange) -- this keeps the "clean look" requirement intact even though the underlying dataset is the real, full-size BEST network, not a curated toy subset.

`src/search.c` (SDL-free, `tests/test_search.c`) is a case-insensitive substring filter over route numbers and (English) stop names; `src/ui_sdl.c` drives it from a Latin-only on-screen keypad rather than a physical keyboard, keeping the click-as-touch-only input model intact while still making a 473-route, multi-hundred-stop catalogue searchable.

Large targets, visible status, consistent Back actions, high contrast, map/list redundancy, and recoverable paper errors apply Shneiderman's consistency/error rules, Norman's affordances/feedback, and Nielsen's visibility, control, recognition, and recovery heuristics. Discrete zoom buttons (rather than pinch/drag gestures), page-scrolled lists, and an on-screen keypad (rather than assuming a physical keyboard) follow the same click-as-touch-only, large-target philosophy now that the dataset is large enough to need both pagination and search. See [HCI_Values.md](HCI_Values.md) for the full mapping from these choices back to Shneiderman/Norman/Nielsen.
