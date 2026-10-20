#!/usr/bin/env python3
"""Regenerate config/timetables.json from the real BEST GTFS feed (maintainer-only).

Companion to tools/import_gtfs.py (see that file's docstring for the overall
data pipeline) -- separate so the fetch source for schedules can be swapped
independently of the route/stop catalogue later. Shares its GTFS cache
(.cache/gtfs/gtfs_compat.zip) and download step.

Unlike tools/import_gtfs.py, which picks one representative trip per route
just to recover stop order, this script wants every trip: for each route in
the already-committed config/routes.json (read, not re-derived, so this
file's entries line up 1:1 in the same order -- see each entry's "route_id"),
it collects every trip's departure time from the first stop of its specific
route_id (so up/down directions each get their own correct schedule), sorts
them, and evenly subsamples down to MAX_DEPARTURES (keeping the first and
last of the day) if the real count is larger -- some BEST routes run close
to 1000 trips/day, far more than a kiosk screen should list. This makes the
result an approximate schedule, not the literal GTFS, which is what the UI
calls it (see T_NO_TIMETABLE / T_TIMETABLE in src/i18n.c).

Needs network access the first time (then reuses the cache). Writes
config/timetables.json, consumed by the network-free tools/generate_catalog.py.
"""
import csv
import io
import json
import sys
import zipfile
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GTFS_URL = "https://raw.githubusercontent.com/croyla/mumbai-gtfs/main/gtfs_compat.zip"
CACHE_DIR = ROOT / ".cache/gtfs"
CACHE_ZIP = CACHE_DIR / "gtfs_compat.zip"

# Keep in sync with MAX_DEPARTURES in include/timetable.h.
MAX_DEPARTURES = 48


def download_gtfs(force=False):
    if CACHE_ZIP.exists() and not force:
        return CACHE_ZIP
    CACHE_DIR.mkdir(parents=True, exist_ok=True)
    print(f"Downloading {GTFS_URL} ...", file=sys.stderr)
    import urllib.request

    urllib.request.urlretrieve(GTFS_URL, CACHE_ZIP)
    return CACHE_ZIP


def normalize_time(hms):
    """GTFS allows hour >= 24 for trips past midnight (e.g. "25:10:00" for
    1:10 AM the next service day); wrap those back into 0-23 for display."""
    h, m, _ = hms.split(":")
    return f"{int(h) % 24:02d}:{m}"


def cap_departures(times, max_count):
    """Evenly subsample down to max_count, always keeping the first and last
    departure of the day (mirrors tools/import_gtfs.py's cap_stops)."""
    n = len(times)
    if n <= max_count:
        return times
    step = (n - 1) / (max_count - 1)
    indices = sorted({round(i * step) for i in range(max_count)})
    return [times[i] for i in indices]


def trip_ids_by_route(zf, route_ids):
    by_route = defaultdict(set)
    with zf.open("trips.txt") as f:
        for row in csv.DictReader(io.TextIOWrapper(f, encoding="utf-8")):
            if row["route_id"] in route_ids:
                by_route[row["route_id"]].add(row["trip_id"])
    return by_route


def first_stop_departures(zf, wanted_trip_ids):
    """One streaming pass over stop_times.txt (~140MB uncompressed), keeping
    only each wanted trip's stop_sequence==1 departure_time."""
    departures = {}
    with zf.open("stop_times.txt") as f:
        for row in csv.DictReader(io.TextIOWrapper(f, encoding="utf-8")):
            if row["stop_sequence"] == "1" and row["trip_id"] in wanted_trip_ids:
                departures[row["trip_id"]] = row["departure_time"]
    return departures


def build_timetables_json(zf, routes):
    route_ids = {r["route_id"] for r in routes}
    print(f"Collecting trips for {len(route_ids)} routes ...", file=sys.stderr)
    trips_by_route = trip_ids_by_route(zf, route_ids)
    wanted_trip_ids = {tid for tids in trips_by_route.values() for tid in tids}

    print("Streaming stop_times.txt (one pass, this takes a while) ...", file=sys.stderr)
    departure_by_trip = first_stop_departures(zf, wanted_trip_ids)

    out = []
    for r in routes:
        times = sorted(
            normalize_time(departure_by_trip[tid])
            for tid in trips_by_route.get(r["route_id"], ())
            if tid in departure_by_trip
        )
        out.append(cap_departures(times, MAX_DEPARTURES))
    return out


def main():
    routes = json.loads((ROOT / "config/routes.json").read_text(encoding="utf-8"))
    missing = [r["number"] for r in routes if "route_id" not in r]
    if missing:
        raise SystemExit(
            "config/routes.json has entries with no \"route_id\" -- "
            "re-run tools/import_gtfs.py first to add it."
        )

    zip_path = download_gtfs()
    with zipfile.ZipFile(zip_path) as zf:
        timetables_json = build_timetables_json(zf, routes)

    out_path = ROOT / "config/timetables.json"
    out_path.write_text(json.dumps(timetables_json, indent=1) + "\n", encoding="utf-8")
    nonempty = sum(1 for t in timetables_json if t)
    print(f"Wrote timetables for {nonempty}/{len(routes)} routes to {out_path}", file=sys.stderr)


if __name__ == "__main__":
    main()
