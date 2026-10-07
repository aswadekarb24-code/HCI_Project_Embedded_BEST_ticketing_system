#!/usr/bin/env python3
"""Regenerate config/routes.json from the real BEST GTFS feed (maintainer-only).

This is an occasional data-refresh step, separate from the network-free
tools/generate_catalog.py that actually runs during scripts/build.sh. It:

 1. Downloads (and caches) the GTFS feed published by croyla/mumbai-gtfs
    (MIT-0 licensed).
 2. Filters it down to BEST (Brihanmumbai Electric Supply and Transport)
    routes and stops -- the feed also covers TMT/KDMT/VVMT, which are out of
    scope for this BEST ticketing kiosk.
 3. The feed has no shapes.txt, so each route's path is approximated as
    straight lines through its real, ordered stop sequence: one
    representative trip per route (direction 0 if present) is picked from
    trips.txt, then stop_times.txt (~140MB) is streamed once, filtering on
    that small set of trip IDs, to recover the stop order.
 4. Projects each stop's lat/lon into basemap pixel space with the same
    formula as mapview_geo_to_pixel() in src/mapview.c (kept in sync
    manually -- see geo_to_pixel() below), and assigns each route a region
    (0-3) by which quadrant of the bbox its stops' centroid falls in.
 5. Auto-transliterates English stop/route names into Devanagari (serving
    both Marathi and Hindi, which share a script) and Gujarati, using
    indic_transliteration. This is a best-effort phonetic rendering of
    proper nouns -- the same approach real transit signage uses -- not a
    human-reviewed translation. The GTFS feed has no native mr/hi/gu names
    to draw on. Route/stop *codes* (e.g. "45AS") are never transliterated,
    only descriptive names.

Needs network access and the packages in tools/requirements.txt (use a venv:
see that file's header comment). Writes config/routes.json, which is then
committed and consumed by the network-free tools/generate_catalog.py.
"""
import argparse
import csv
import io
import json
import re
import statistics
import sys
import urllib.request
import zipfile
from collections import defaultdict
from pathlib import Path

# The bundled NotoSansGujarati-Regular.ttf is an extremely narrow script-only
# subset (confirmed via `fc-query --format '%{charset}'`): just the Gujarati
# block, ZWJ/ZWNJ, a Unicode hyphen, the rupee sign, danda marks and space --
# no ASCII at all, not even parentheses or '-'. Script-to-script
# transliteration (Devanagari -> Gujarati) can still leave stray source
# punctuation in the output (e.g. "(W)" from a stop name), which would
# render as tofu boxes. Strip anything outside that confirmed-safe set
# rather than risk unrenderable glyphs.
_GUJARATI_SAFE = re.compile(r"[^઀-૿।॥​-‍‐₹\s]")


def sanitize_gujarati(text):
    return " ".join(_GUJARATI_SAFE.sub(" ", text).split())

ROOT = Path(__file__).resolve().parents[1]
GTFS_URL = "https://raw.githubusercontent.com/croyla/mumbai-gtfs/main/gtfs_compat.zip"
CACHE_DIR = ROOT / ".cache/gtfs"
CACHE_ZIP = CACHE_DIR / "gtfs_compat.zip"

# Fixed 4-color palette, one per bbox quadrant (region 0-3). Matches the
# project's existing house colors.
REGION_COLORS = ["137C8B", "E67E22", "8E44AD", "27AE60"]


def download_gtfs(force=False):
    if CACHE_ZIP.exists() and not force:
        return CACHE_ZIP
    CACHE_DIR.mkdir(parents=True, exist_ok=True)
    print(f"Downloading {GTFS_URL} ...", file=sys.stderr)
    urllib.request.urlretrieve(GTFS_URL, CACHE_ZIP)
    return CACHE_ZIP


def geo_to_pixel(lat, lon, bounds):
    """Must match mapview_geo_to_pixel() in src/mapview.c exactly."""
    fx = (lon - bounds["lon_min"]) / (bounds["lon_max"] - bounds["lon_min"])
    fy = (bounds["lat_max"] - lat) / (bounds["lat_max"] - bounds["lat_min"])
    x = int(fx * bounds["image_width"])
    y = int(fy * bounds["image_height"])
    x = max(0, min(bounds["image_width"], x))
    y = max(0, min(bounds["image_height"], y))
    return x, y


def region_bands(centroid_lats):
    """3 cut points splitting routes into 4 roughly-equal-sized bands by
    latitude (south to north along Mumbai's elongated peninsula). This is
    purely to keep the map's region color coding varied and legible across
    the real route count -- not a claim about administrative boundaries. A
    fixed lon/lat bbox-midpoint quadrant split was tried first and produced
    a lopsided ~430/8/6/... distribution because BEST's network isn't
    evenly spread across the padded bbox; per-dataset quantiles fix that."""
    return statistics.quantiles(centroid_lats, n=4)


def region_for(lat, bands):
    region = sum(1 for cut in bands if lat > cut)
    return min(region, 3)


def cap_stops(ordered_stop_ids, max_stops):
    """Evenly subsample down to max_stops, always keeping the first and last
    stop, if a route's real sequence ever exceeds the cap (the observed max
    in the BEST feed is 118, under MAX_STOPS=128, so this is a safety net,
    not something normally triggered)."""
    n = len(ordered_stop_ids)
    if n <= max_stops:
        return ordered_stop_ids
    step = (n - 1) / (max_stops - 1)
    indices = sorted({round(i * step) for i in range(max_stops)})
    return [ordered_stop_ids[i] for i in indices]


class Transliterator:
    """Best-effort English -> Devanagari -> Gujarati transliteration, memoized
    since many stop names repeat across routes."""

    def __init__(self):
        from indic_transliteration import sanscript
        from indic_transliteration.sanscript import transliterate

        self._sanscript = sanscript
        self._transliterate = transliterate
        self._cache = {}

    @staticmethod
    def _clean(text):
        return " ".join(text.lower().replace(".", " ").replace(";", ",").split())

    def convert(self, text):
        if text in self._cache:
            return self._cache[text]
        cleaned = self._clean(text)
        deva = self._transliterate(cleaned, self._sanscript.ITRANS, self._sanscript.DEVANAGARI)
        guj = sanitize_gujarati(self._transliterate(deva, self._sanscript.DEVANAGARI, self._sanscript.GUJARATI))
        self._cache[text] = (deva, guj)
        return deva, guj


def load_routes(zf, agency):
    routes = {}
    with zf.open("routes.txt") as f:
        for row in csv.DictReader(io.TextIOWrapper(f, encoding="utf-8")):
            if row["agency_id"] == agency:
                routes[row["route_id"]] = row
    return routes


def load_stops(zf):
    stops = {}
    with zf.open("stops.txt") as f:
        for row in csv.DictReader(io.TextIOWrapper(f, encoding="utf-8")):
            stops[row["stop_id"]] = (row["stop_name"], float(row["stop_lat"]), float(row["stop_lon"]))
    return stops


def pick_representative_trips(zf, route_ids):
    """One trip_id per route (direction 0 preferred) -> {route_id: trip_id}."""
    by_route = defaultdict(dict)
    with zf.open("trips.txt") as f:
        for row in csv.DictReader(io.TextIOWrapper(f, encoding="utf-8")):
            rid = row["route_id"]
            if rid not in route_ids:
                continue
            direction = row["direction_id"]
            if direction not in by_route[rid]:
                by_route[rid][direction] = row["trip_id"]
    representative = {}
    for rid, directions in by_route.items():
        representative[rid] = directions.get("0") or next(iter(directions.values()))
    return representative


def collect_stop_sequences(zf, trip_ids):
    """One streaming pass over stop_times.txt (~140MB uncompressed) filtered
    to the small set of representative trip_ids. Returns
    {trip_id: [(stop_sequence, stop_id), ...]} (unsorted, sorted by caller)."""
    sequences = defaultdict(list)
    with zf.open("stop_times.txt") as f:
        reader = csv.DictReader(io.TextIOWrapper(f, encoding="utf-8"))
        for row in reader:
            tid = row["trip_id"]
            if tid in trip_ids:
                sequences[tid].append((int(row["stop_sequence"]), row["stop_id"]))
    return sequences


def build_routes_json(zf, bounds, agency, max_stops, limit=None):
    routes = load_routes(zf, agency)
    route_ids = set(routes)
    if limit:
        route_ids = set(list(route_ids)[:limit])
        routes = {k: v for k, v in routes.items() if k in route_ids}
    print(f"{len(routes)} {agency} routes", file=sys.stderr)

    representative = pick_representative_trips(zf, route_ids)
    trip_ids = set(representative.values())
    print(f"{len(trip_ids)} representative trips selected", file=sys.stderr)

    print("Streaming stop_times.txt (one pass, this takes a while) ...", file=sys.stderr)
    sequences = collect_stop_sequences(zf, trip_ids)

    stops = load_stops(zf)
    translit = Transliterator()
    built = []
    for route_id, route in sorted(routes.items()):
        trip_id = representative.get(route_id)
        seq = sequences.get(trip_id, [])
        if len(seq) < 2:
            continue
        seq.sort(key=lambda t: t[0])
        stop_ids = cap_stops([sid for _, sid in seq], max_stops)

        stop_entries = []
        lats, lons = [], []
        for sid in stop_ids:
            if sid not in stops:
                continue
            name, lat, lon = stops[sid]
            lats.append(lat)
            lons.append(lon)
            x, y = geo_to_pixel(lat, lon, bounds)
            deva, guj = translit.convert(name)
            stop_entries.append([name, deva, deva, guj, x, y])
        if len(stop_entries) < 2:
            continue

        long_name = route["route_long_name"]
        deva_name, guj_name = translit.convert(long_name)
        built.append(
            {
                "number": route["route_short_name"],
                "name_en": long_name,
                "name_mr": deva_name,
                "name_hi": deva_name,
                "name_gu": guj_name,
                "region": None,
                "color": None,
                "stops": stop_entries,
                "_centroid_lat": sum(lats) / len(lats),
            }
        )

    bands = region_bands([r["_centroid_lat"] for r in built])
    out = []
    for r in built:
        region = region_for(r.pop("_centroid_lat"), bands)
        r["region"] = region
        r["color"] = REGION_COLORS[region]
        out.append(r)
    return out


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--agency", default="BEST")
    parser.add_argument("--max-stops", type=int, default=128)
    parser.add_argument("--limit", type=int, default=None, help="only process the first N routes (dev/testing)")
    parser.add_argument("--force-download", action="store_true")
    args = parser.parse_args()

    bounds = json.loads((ROOT / "config/map_bounds.json").read_text(encoding="utf-8"))
    zip_path = download_gtfs(force=args.force_download)
    with zipfile.ZipFile(zip_path) as zf:
        routes_json = build_routes_json(zf, bounds, args.agency, args.max_stops, args.limit)

    out_path = ROOT / "config/routes.json"
    out_path.write_text(json.dumps(routes_json, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    print(f"Wrote {len(routes_json)} routes to {out_path}", file=sys.stderr)


if __name__ == "__main__":
    main()
