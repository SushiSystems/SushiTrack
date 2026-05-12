#!/bin/bash
# Project root is two levels up
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$PROJECT_ROOT"

echo "[INFO] Cleaning project build artifacts..."

if [ -f "$PROJECT_ROOT/.tools/paths.env" ]; then
    export $(grep -v '^#' "$PROJECT_ROOT/.tools/paths.env" | xargs)
fi
BUILD_DIR=${DIR_BUILD:-build}

if [ -d "$BUILD_DIR" ]; then
    echo "[INFO] Removing $BUILD_DIR/"
    rm -rf "$BUILD_DIR"
fi

echo "[SUCCESS] Clean completed."
