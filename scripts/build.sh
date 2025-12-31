#!/usr/bin/env bash
set -euo pipefail

# Build script for Unix/Linux (produces .so)
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"

echo "Configuring (Release) into $BUILD_DIR"
cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release

echo "Building..."
cmake --build "$BUILD_DIR" --config Release --parallel "$(nproc)"

echo "Build finished. Artifacts:"
ls -l "$BUILD_DIR"/ExampleModule* "$BUILD_DIR"/*.so 2>/dev/null || true
