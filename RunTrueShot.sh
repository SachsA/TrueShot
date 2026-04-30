#!/usr/bin/env bash
# Build & run TrueShot on macOS / Linux.
#
# Looks for a vcpkg toolchain in $VCPKG_ROOT first, then in ~/vcpkg.
# Override either by exporting VCPKG_ROOT or BUILD_TYPE before running.

set -euo pipefail

BUILD_TYPE="${BUILD_TYPE:-Release}"
BUILD_DIR="${BUILD_DIR:-build}"
VCPKG_ROOT="${VCPKG_ROOT:-$HOME/vcpkg}"
TOOLCHAIN="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"

if [[ ! -f "$TOOLCHAIN" ]]; then
    echo "[error] vcpkg toolchain not found at $TOOLCHAIN"
    echo "        Install vcpkg or set VCPKG_ROOT to your vcpkg folder."
    exit 1
fi

echo "[1/3] Configuring TrueShot ($BUILD_TYPE) ..."
cmake -S . -B "$BUILD_DIR" \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE"

echo "[2/3] Building ..."
if [[ "$(uname)" == "Darwin" ]]; then
    JOBS=$(sysctl -n hw.ncpu)
else
    JOBS=$(nproc)
fi
cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" --parallel "$JOBS"

echo "[3/3] Running ..."
"$BUILD_DIR/bin/TrueShot"
