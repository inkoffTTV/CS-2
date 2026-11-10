#!/usr/bin/env bash
# Configure, build and run the native BrutusSim unit tests.
set -euo pipefail
cd "$(dirname "$0")"
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug > /dev/null
cmake --build build -j > /dev/null
./build/brutus_sim_tests
