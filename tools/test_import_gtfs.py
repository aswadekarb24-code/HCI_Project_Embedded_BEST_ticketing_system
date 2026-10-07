#!/usr/bin/env python3
"""Unit tests for the pure/offline parts of tools/import_gtfs.py.

No network access needed -- feeds tiny inline sample data instead of the
real GTFS feed. Run with: source .venv/bin/activate && python -m unittest
tools/test_import_gtfs.py (needs tools/requirements.txt installed).
"""
import csv
import io
import unittest
import zipfile

import import_gtfs as gtfs

BOUNDS = {"lon_min": 72.0, "lon_max": 73.0, "lat_min": 18.0, "lat_max": 19.0, "image_width": 1000, "image_height": 1000}


def make_zip(files):
    buf = io.BytesIO()
    with zipfile.ZipFile(buf, "w") as zf:
        for name, rows in files.items():
            text = io.StringIO()
            writer = csv.DictWriter(text, fieldnames=rows[0].keys())
            writer.writeheader()
            writer.writerows(rows)
            zf.writestr(name, text.getvalue())
    buf.seek(0)
    return zipfile.ZipFile(buf)


class ProjectionTests(unittest.TestCase):
    def test_corners(self):
        self.assertEqual(gtfs.geo_to_pixel(19.0, 72.0, BOUNDS), (0, 0))
        self.assertEqual(gtfs.geo_to_pixel(18.0, 73.0, BOUNDS), (1000, 1000))

    def test_center(self):
        x, y = gtfs.geo_to_pixel(18.5, 72.5, BOUNDS)
        self.assertAlmostEqual(x, 500, delta=1)
        self.assertAlmostEqual(y, 500, delta=1)

    def test_clamps_out_of_range(self):
        x, y = gtfs.geo_to_pixel(25.0, 60.0, BOUNDS)
        self.assertTrue(0 <= x <= 1000)
        self.assertTrue(0 <= y <= 1000)


class RegionBandTests(unittest.TestCase):
    def test_balanced_split(self):
        lats = list(range(100))  # 0..99, evenly spread
        bands = gtfs.region_bands(lats)
        counts = {}
        for lat in lats:
            r = gtfs.region_for(lat, bands)
            counts[r] = counts.get(r, 0) + 1
        self.assertEqual(set(counts), {0, 1, 2, 3})
        # Roughly equal (quartiles of an evenly spread list): no band empty,
        # none wildly dominant.
        for count in counts.values():
            self.assertGreater(count, 15)


class CapStopsTests(unittest.TestCase):
    def test_no_op_under_cap(self):
        stops = list(range(10))
        self.assertEqual(gtfs.cap_stops(stops, 20), stops)

    def test_subsamples_keeping_endpoints(self):
        stops = list(range(200))
        capped = gtfs.cap_stops(stops, 50)
        self.assertLessEqual(len(capped), 50)
        self.assertEqual(capped[0], 0)
        self.assertEqual(capped[-1], 199)
        self.assertEqual(capped, sorted(capped))


class GujaratiSanitizeTests(unittest.TestCase):
    def test_strips_ascii_punctuation(self):
        out = gtfs.sanitize_gujarati("ચુર્ચ્ગતે (ડબ્લ્યુ)")
        self.assertNotIn("(", out)
        self.assertNotIn(")", out)
        self.assertIn("ચુર્ચ્ગતે", out)

    def test_keeps_pure_gujarati_unchanged_besides_spacing(self):
        text = "બેસ્ટમાં આપનું સ્વાગત છે"
        self.assertEqual(gtfs.sanitize_gujarati(text), text)

    def test_collapses_whitespace_left_by_stripped_chars(self):
        out = gtfs.sanitize_gujarati("અ - બ")
        self.assertNotIn("  ", out)


class GtfsParsingTests(unittest.TestCase):
    def setUp(self):
        self.zf = make_zip(
            {
                "routes.txt": [
                    {"route_id": "r1", "agency_id": "BEST", "route_short_name": "1", "route_long_name": "A to B", "route_type": "3"},
                    {"route_id": "r2", "agency_id": "TMT", "route_short_name": "2", "route_long_name": "C to D", "route_type": "3"},
                ],
                "stops.txt": [
                    {"stop_id": "s1", "stop_name": "Stop One", "stop_lat": "18.2", "stop_lon": "72.3"},
                    {"stop_id": "s2", "stop_name": "Stop Two", "stop_lat": "18.4", "stop_lon": "72.5"},
                ],
                "trips.txt": [
                    {"route_id": "r1", "service_id": "wk", "trip_id": "t1", "trip_headsign": "B", "direction_id": "0"},
                    {"route_id": "r1", "service_id": "wk", "trip_id": "t2", "trip_headsign": "A", "direction_id": "1"},
                ],
                "stop_times.txt": [
                    {"trip_id": "t1", "arrival_time": "", "departure_time": "", "stop_id": "s1", "stop_sequence": "1", "timepoint": "1"},
                    {"trip_id": "t1", "arrival_time": "", "departure_time": "", "stop_id": "s2", "stop_sequence": "2", "timepoint": "1"},
                ],
            }
        )

    def test_agency_filter(self):
        routes = gtfs.load_routes(self.zf, "BEST")
        self.assertEqual(set(routes), {"r1"})

    def test_representative_trip_prefers_direction_zero(self):
        rep = gtfs.pick_representative_trips(self.zf, {"r1"})
        self.assertEqual(rep["r1"], "t1")

    def test_stop_sequence_collection(self):
        seq = gtfs.collect_stop_sequences(self.zf, {"t1"})
        self.assertEqual(sorted(seq["t1"]), [(1, "s1"), (2, "s2")])


if __name__ == "__main__":
    unittest.main()
