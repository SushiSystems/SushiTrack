#!/bin/bash
# Project root is two levels up
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$PROJECT_ROOT"

SOURCE_DIR="."
BUILD_DIR="build"
CLEAN=0
TEST=0

for arg in "$@"; do
    if [ "$arg" == "--clean" ]; then CLEAN=1; fi
    if [ "$arg" == "--test" ]; then TEST=1; fi
    if [ "$arg" == "--addsanitizer" ]; then USE_ASAN="1"; fi
done

TARGET_ARG=""
EXTRA_ARGS=""

if [ $TEST -eq 1 ]; then
    EXTRA_ARGS="-DBUILD_MOT_TEST=ON"
    TARGET_ARG="--target mot_test --target tracker_bridge"
else
    EXTRA_ARGS="-DBUILD_MOT_TEST=OFF"
    TARGET_ARG="--target sushitrack"
fi

if [ "$USE_ASAN" == "1" ]; then
    EXTRA_ARGS="$EXTRA_ARGS -DUSE_ASAN=ON"
fi

if [ $CLEAN -eq 1 ]; then
    echo "[INFO] Removing build directory: $BUILD_DIR"
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"

echo "[INFO] Configuring $SOURCE_DIR..."
cmake -S "$SOURCE_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release $EXTRA_ARGS

if [ $? -ne 0 ]; then
    echo "[ERROR] CMake configuration failed!"
    exit 1
fi

echo "[INFO] Compiling SushiTrack..."
cmake --build "$BUILD_DIR" --config Release --parallel $(nproc 2>/dev/null || echo 2) $TARGET_ARG

if [ $? -ne 0 ]; then
    echo "[ERROR] Build failed!"
    exit 1
fi

echo "[SUCCESS] Build process completed."
