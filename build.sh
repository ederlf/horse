#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
cmake --preset dev "$@"
cmake --build --preset dev --parallel "${CMAKE_BUILD_PARALLEL_LEVEL:-2}"
ctest --preset dev
python3 -m pip install -e .
