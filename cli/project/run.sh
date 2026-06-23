#!/bin/bash
# Project root is two levels up
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$PROJECT_ROOT"

# Fallback to python runner for better logic
python3 cli/sushitrack.py project run "$@"
