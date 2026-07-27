#!/usr/bin/env bash
# Clean TrueShot build artefacts on macOS / Linux.
#
# Run from anywhere — the script resolves the repo root from its own path.
#
# Three levels, from cheapest to most destructive:
#
#   ./scripts/clean.sh              Build artefacts only (default).
#   ./scripts/clean.sh --deps       ...plus vcpkg dependencies (needs re-download).
#   ./scripts/clean.sh --all        ...plus the global vcpkg cache (very slow rebuild).
#
# Flags:
#   --dry-run   Print what would be deleted, delete nothing.
#   --yes, -y   Skip the confirmation prompt (for CI / scripting).
#   --help, -h  Show this help.
#
# Nothing tracked by git is ever touched — the script only removes paths
# that are listed in .gitignore. Run `git status` after a clean to
# confirm the working tree is unchanged.

set -euo pipefail

LEVEL="build"
DRY_RUN=0
ASSUME_YES=0

usage() {
    # Print the header comment block (everything between line 2 and the
    # first non-comment line) as the help text.
    sed -n '2,/^[^#]/p' "$0" | grep '^#' | sed 's/^# \{0,1\}//'
    exit 0
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --deps)          LEVEL="deps" ;;
        --all|--nuke)    LEVEL="all" ;;
        --dry-run|-n)    DRY_RUN=1 ;;
        --yes|-y)        ASSUME_YES=1 ;;
        --help|-h)       usage ;;
        *)
            echo "[error] unknown argument: $1" >&2
            echo "        run '$0 --help' for usage" >&2
            exit 1
            ;;
    esac
    shift
done

# ---------------------------------------------------------------------
# Resolve the repo root from the script's own location (scripts/ -> ..)
# and refuse to run if it doesn't look right. Deleting build/ and
# vcpkg_installed/ from the wrong directory would be a bad day.
# ---------------------------------------------------------------------
cd "$(dirname "$0")/.."
if [[ ! -f CMakeLists.txt || ! -f vcpkg.json ]]; then
    echo "[error] can't find the TrueShot repo root" >&2
    echo "        (expected CMakeLists.txt and vcpkg.json in $PWD)" >&2
    exit 1
fi
REPO_ROOT="$PWD"

# ---------------------------------------------------------------------
# Build the target list for the requested level.
# ---------------------------------------------------------------------
TARGETS=()

# --- Level 1: build artefacts (always) -------------------------------
TARGETS+=(
    "build"                     # every CMake preset lands under here
    "network_module/build"      # nested sub-project builds
    "out"                       # Visual Studio / VS Code CMake default
    "dist"                      # CI artefact staging
    "CMakeCache.txt"
    "CMakeFiles"
    "CMakeUserPresets.json"
    "compile_commands.json"
)
# Any cmake-build-* directory (CLion / JetBrains convention).
# Plain glob rather than `find -printf`, which is GNU-only and would
# break on macOS's BSD find.
for d in cmake-build-*; do
    [[ -e "$d" ]] && TARGETS+=("$d")
done

# --- Level 2: dependencies -------------------------------------------
if [[ "$LEVEL" == "deps" || "$LEVEL" == "all" ]]; then
    TARGETS+=(
        "vcpkg_installed"       # manifest-mode installed packages
        "vcpkg"                 # in-tree vcpkg clone (CI bootstraps this)
    )
fi

# --- Level 3: global caches ------------------------------------------
GLOBAL_TARGETS=()
if [[ "$LEVEL" == "all" ]]; then
    # vcpkg's per-user download + binary cache. Wiping this means every
    # dependency is re-downloaded and rebuilt from source next time.
    if [[ "$(uname)" == "Darwin" ]]; then
        GLOBAL_TARGETS+=("$HOME/.cache/vcpkg")
    else
        GLOBAL_TARGETS+=("${XDG_CACHE_HOME:-$HOME/.cache}/vcpkg")
    fi
    # If VCPKG_ROOT points at an out-of-tree vcpkg, clean its build
    # scratch dirs but never the clone itself (that's the user's install).
    if [[ -n "${VCPKG_ROOT:-}" && -d "$VCPKG_ROOT" && "$VCPKG_ROOT" != "$REPO_ROOT/vcpkg" ]]; then
        GLOBAL_TARGETS+=(
            "$VCPKG_ROOT/buildtrees"
            "$VCPKG_ROOT/packages"
            "$VCPKG_ROOT/downloads"
        )
    fi
fi

# ---------------------------------------------------------------------
# Report what exists, then act.
# ---------------------------------------------------------------------
FOUND=()
for t in "${TARGETS[@]}"; do
    [[ -e "$t" ]] && FOUND+=("$t")
done
for t in "${GLOBAL_TARGETS[@]}"; do
    [[ -e "$t" ]] && FOUND+=("$t")
done

echo "TrueShot clean — level: $LEVEL"
echo ""

if [[ ${#FOUND[@]} -eq 0 ]]; then
    echo "Nothing to clean. Already pristine."
    exit 0
fi

echo "Will remove:"
TOTAL_HUMAN=""
for t in "${FOUND[@]}"; do
    SIZE=$(du -sh "$t" 2>/dev/null | cut -f1 || echo "?")
    printf '  %-8s %s\n' "$SIZE" "$t"
done
echo ""

if [[ $DRY_RUN -eq 1 ]]; then
    echo "(--dry-run: nothing was deleted)"
    exit 0
fi

if [[ "$LEVEL" == "all" && $ASSUME_YES -eq 0 ]]; then
    echo "Level 'all' wipes the global vcpkg cache — the next build will"
    echo "re-download and recompile every dependency (can take 10+ min)."
    read -r -p "Continue? [y/N] " reply
    [[ "$reply" =~ ^[Yy]$ ]] || { echo "Aborted."; exit 0; }
fi

for t in "${FOUND[@]}"; do
    rm -rf "$t"
done

echo "Done. Working tree should be unchanged — verify with 'git status'."
echo ""
case "$LEVEL" in
    build) echo "Next: ./scripts/run.sh (dependencies were kept, so this is fast)." ;;
    deps)  echo "Next: ./scripts/run.sh (vcpkg will re-install the manifest deps)." ;;
    all)   echo "Next: ./scripts/run.sh (full dependency rebuild — grab a coffee)." ;;
esac
