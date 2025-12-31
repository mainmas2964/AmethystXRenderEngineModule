#!/usr/bin/env bash
set -euo pipefail

# Cross-build a Windows DLL using MinGW-w64 on Linux.
# Requires a MinGW-w64 toolchain (e.g. x86_64-w64-mingw32-g++).

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build-windows-mingw"

MINGW_CC="${MINGW_CC:-x86_64-w64-mingw32-g++}"

if ! command -v "$MINGW_CC" >/dev/null 2>&1; then
  echo "MinGW compiler $MINGW_CC not found in PATH. Install mingw-w64 and try again." >&2
  exit 2
fi

echo "Configuring Windows cross-build into $BUILD_DIR using $MINGW_CC"
cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_SYSTEM_NAME=Windows -DCMAKE_CXX_COMPILER="$MINGW_CC"

echo "Building..."
cmake --build "$BUILD_DIR" --config Release --parallel "$(nproc)"

echo "Build finished. Artifacts:"
ls -l "$BUILD_DIR"/ExampleModule* "$BUILD_DIR"/*.dll 2>/dev/null || true
