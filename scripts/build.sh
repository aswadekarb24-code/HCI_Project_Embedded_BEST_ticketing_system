#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
python3 "$root/tools/generate_catalog.py"
cmake -S "$root" -B "$root/build" -G Ninja
cmake --build "$root/build"
