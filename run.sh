#!/bin/bash
set -e

cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH="$HOME/vtk-9.1"
cmake --build build -j20