#!/bin/bash
# Project root is two levels up
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$PROJECT_ROOT"

echo "[INFO] Running SushiTrack Docker container..."
# Using workspace path to match working examples
docker run -it --rm -v "$(pwd):/workspace/sushitrack" sushitrack bash
