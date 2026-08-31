#!/bin/sh
set -e
# Configure with Ninja generator into 'build.ninja'
cmake -S . -B build.ninja -G Ninja "$@"
# Build using Ninja
cmake --build build.ninja --parallel
