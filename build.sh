#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo "$@"
cmake --build build --parallel "${CMAKE_BUILD_PARALLEL_LEVEL:-2}"
ctest --test-dir build --output-on-failure
python3 -m pip install -e .
