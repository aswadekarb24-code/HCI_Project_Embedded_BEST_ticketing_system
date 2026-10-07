#!/usr/bin/env python3
"""Fetch an offline satellite basemap of Mumbai (maintainer-only, needs network).

Downloads a Sentinel-2 cloudless mosaic tile from the EOX ``s2cloudless`` WMS
service covering the bounding box recorded in config/map_bounds.json, and
writes it to assets/mumbai_satellite.png. The kiosk itself never touches the
network -- this script is a one-time/occasional data-refresh step run by a
maintainer, not part of scripts/build.sh.

Imagery: Sentinel-2 cloudless (https://s2maps.eu) by EOX IT Services GmbH,
licensed CC BY-NC-SA 4.0. Keep the attribution in README.md/SETUP.md and the
in-app Help screen in sync with this source if it ever changes.
"""
import json
import shutil
import subprocess
import tempfile
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
WMS_URL = "https://tiles.maps.eox.at/wms"
LAYER = "s2cloudless-2025"
# The service caps GetMap requests at 2048px on either axis (mapcache default
# tile-size limit); larger requests come back as HTTP 400.
MAX_DIM = 2048


def main():
    bounds = json.loads((ROOT / "config/map_bounds.json").read_text(encoding="utf-8"))
    if bounds["image_width"] > MAX_DIM or bounds["image_height"] > MAX_DIM:
        raise SystemExit(f"image_width/image_height must be <= {MAX_DIM} (WMS server limit)")
    bbox = f"{bounds['lon_min']},{bounds['lat_min']},{bounds['lon_max']},{bounds['lat_max']}"
    query = (
        f"{WMS_URL}?service=WMS&version=1.1.1&request=GetMap&layers={LAYER}"
        f"&styles=&srs=EPSG:4326&bbox={bbox}"
        f"&width={bounds['image_width']}&height={bounds['image_height']}"
        f"&format=image/jpeg"
    )
    out = ROOT / "assets/mumbai_satellite.png"
    print(f"Fetching {bounds['image_width']}x{bounds['image_height']} basemap for bbox {bbox} ...")
    with tempfile.NamedTemporaryFile(suffix=".jpg", delete=False) as tmp:
        urllib.request.urlretrieve(query, tmp.name)
        tmp_path = Path(tmp.name)
    magick = shutil.which("magick") or shutil.which("convert")
    if not magick:
        raise SystemExit("ImageMagick (magick/convert) is required to re-encode the fetched JPEG as PNG")
    subprocess.run([magick, str(tmp_path), str(out)], check=True)
    tmp_path.unlink()
    print(f"Wrote {out} ({out.stat().st_size} bytes)")
    print("Source: Sentinel-2 cloudless (s2maps.eu) by EOX IT Services GmbH, CC BY-NC-SA 4.0.")


if __name__ == "__main__":
    main()
