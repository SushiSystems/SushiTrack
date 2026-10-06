#!/bin/bash
# clean_preds.sh
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.

# ANSI Color Codes
BLUE='\033[94m'
GREEN='\033[92m'
RESET='\033[0m'

# Get project root (three levels up from cli/misc)
TOOLS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." && pwd )"
ROOT_DIR="$( cd "$TOOLS_DIR/.." && pwd )"

# Relocated data roots are configured in cli/config.json; `st paths` prints the
# resolved value, which can be passed as the first argument here.
TEST_DATA_DIR="${1:-$ROOT_DIR/tests/regression/evaluator/data}"

echo -e "${BLUE}[INFO]${RESET} Cleaning prediction directories in $TEST_DATA_DIR..."

FOUND=0
if [ -d "$TEST_DATA_DIR" ]; then
    for dir in "$TEST_DATA_DIR"/*/; do
        if [ -d "${dir}pred" ]; then
            echo -e "[DELETE] Removing: ${dir}pred"
            rm -rf "${dir}pred"
            FOUND=1
        fi
    done
fi

if [ $FOUND -eq 0 ]; then
    echo -e "${BLUE}[INFO]${RESET} No 'pred' directories found to clean."
else
    echo -e "${GREEN}[SUCCESS]${RESET} All prediction directories have been cleaned."
fi
