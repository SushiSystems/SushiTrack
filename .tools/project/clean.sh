#!/bin/bash
# Project root is two levels up
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$PROJECT_ROOT"

echo "[INFO] Cleaning project build artifacts..."

if [ -d "build" ]; then
    echo "[INFO] Removing build/"
    rm -rf build
fi

if [ -d "test/build" ]; then
    echo "[INFO] Removing test/build/"
    rm -rf test/build
fi

echo "[SUCCESS] Clean completed."
