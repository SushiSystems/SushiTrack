#!/bin/bash
# Project root is two levels up
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$PROJECT_ROOT"

echo "[INFO] Building SushiTrack Docker image..."
docker build -t sushitrack .

if [ $? -eq 0 ]; then
    echo "[SUCCESS] Image 'sushitrack' built successfully."
else
    echo "[ERROR] Docker build failed."
    exit 1
fi
