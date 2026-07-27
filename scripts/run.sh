#!/usr/bin/env bash
# Build & run TrueShot on macOS / Linux.
#
# Run from anywhere — the script resolves the repo root from its own path.
#
#   ./scripts/run.sh                          Build + run offline.
#   ./scripts/run.sh --server 192.168.1.42    Build + run as a client.
#   ./scripts/run.sh -- --help                Show the game's own CLI help.
#
# Everything after the script's own flags is forwarded to the TrueShot
# binary, so any client flag works here (see `TrueShot --help`).
#
# Environment:
#   BUILD_TYPE   Release (default) | Debug | RelWithDebInfo
#   BUILD_DIR    build (default) — where CMake writes
#   VCPKG_ROOT   path to your vcpkg checkout (falls back to ~/vcpkg)
#
# Flags:
#   --no-run     Configure + build, don't launch.
#   --help, -h   Show this help.

set -euo pipefail

NO_RUN=0
GAME_ARGS=()

while [[ $# -gt 0 ]]; do
    case "$1" in
        --no-run)   NO_RUN=1; shift ;;
        --help|-h)
            sed -n '2,/^[^#]/p' "$0" | grep '^#' | sed 's/^# \{0,1\}//'
            exit 0
            ;;
        --)         shift; GAME_ARGS+=("$@"); break ;;
        *)          GAME_ARGS+=("$1"); shift ;;
    esac
done

# Resolve the repo root from the script's own location (scripts/ -> ..)
# so `cmake -S .` is correct no matter where this was invoked from.
cd "$(dirname "$0")/.."
if [[ ! -f CMakeLists.txt || ! -f vcpkg.json ]]; then
    echo "[error] can't find the TrueShot repo root" >&2
    echo "        (expected CMakeLists.txt and vcpkg.json in $PWD)" >&2
    exit 1
fi

BUILD_TYPE="${BUILD_TYPE:-Release}"
BUILD_DIR="${BUILD_DIR:-build}"
VCPKG_ROOT="${VCPKG_ROOT:-$HOME/vcpkg}"
TOOLCHAIN="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"

if [[ ! -f "$TOOLCHAIN" ]]; then
    echo "[error] vcpkg toolchain not found at $TOOLCHAIN" >&2
    echo "        Install vcpkg or set VCPKG_ROOT to your vcpkg folder." >&2
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

if [[ $NO_RUN -eq 1 ]]; then
    echo "[3/3] --no-run: build complete, not launching."
    exit 0
fi

echo "[3/3] Running ..."
"$BUILD_DIR/bin/TrueShot" "${GAME_ARGS[@]}"
