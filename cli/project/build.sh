#!/bin/bash
# Project root is two levels up from this script.
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$PROJECT_ROOT"

BUILD_TYPE=Release
SOURCE_DIR="."
if [ -f "$PROJECT_ROOT/cli/paths.env" ]; then
    export $(grep -v '^#' "$PROJECT_ROOT/cli/paths.env" | xargs)
fi
BUILD_DIR=${DIR_BUILD:-build}
CLEAN=0
BUILD_UNIT=0
BUILD_REGRESSION=0
BUILD_INTEGRATION=0

for arg in "$@"; do
    case "$arg" in
        --clean)        CLEAN=1 ;;
        --unit)         BUILD_UNIT=1 ;;
        --regression)   BUILD_REGRESSION=1 ;;
        --integration)  BUILD_INTEGRATION=1 ;;
        --test)         BUILD_UNIT=1; BUILD_REGRESSION=1; BUILD_INTEGRATION=1 ;;
    esac
done

# ----------------------------------------------------------------
# Compose CMake options
# ----------------------------------------------------------------
CMAKE_ARGS="-DBUILD_UNIT_TEST=OFF -DBUILD_REGRESSION_TEST=OFF -DBUILD_INTEGRATION_TEST=OFF -DBUILD_MOT_TEST=OFF"

[ $BUILD_UNIT      -eq 1 ] && CMAKE_ARGS="$CMAKE_ARGS -DBUILD_UNIT_TEST=ON"
[ $BUILD_REGRESSION -eq 1 ] && CMAKE_ARGS="$CMAKE_ARGS -DBUILD_REGRESSION_TEST=ON"
[ $BUILD_INTEGRATION -eq 1 ] && CMAKE_ARGS="$CMAKE_ARGS -DBUILD_INTEGRATION_TEST=ON"

# ----------------------------------------------------------------
# Compose build targets
# ----------------------------------------------------------------
TARGET_ARGS="--target sushitrack"
[ $BUILD_UNIT       -eq 1 ] && TARGET_ARGS="$TARGET_ARGS --target unit_test"
[ $BUILD_REGRESSION -eq 1 ] && TARGET_ARGS="$TARGET_ARGS --target regression_test --target tracker_bridge"
[ $BUILD_INTEGRATION -eq 1 ] && TARGET_ARGS="$TARGET_ARGS --target integration_test"

# ----------------------------------------------------------------
# Clean
# ----------------------------------------------------------------
if [ $CLEAN -eq 1 ]; then
    echo "[INFO] Removing build directory: $BUILD_DIR"
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"

# ----------------------------------------------------------------
# Configure
# ----------------------------------------------------------------
echo "[INFO] Configuring $SOURCE_DIR ..."
cmake -S "$SOURCE_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=$BUILD_TYPE $CMAKE_ARGS
if [ $? -ne 0 ]; then
    echo "[ERROR] CMake configuration failed."
    exit 1
fi

# ----------------------------------------------------------------
# Build
# ----------------------------------------------------------------
NPROC=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)
echo "[INFO] Building targets: $TARGET_ARGS"
cmake --build "$BUILD_DIR" --config $BUILD_TYPE --parallel "$NPROC" $TARGET_ARGS
if [ $? -ne 0 ]; then
    echo "[ERROR] Build failed."
    exit 1
fi

echo "[SUCCESS] Build process completed."
