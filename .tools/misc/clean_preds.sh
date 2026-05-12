#!/bin/bash

# ANSI Color Codes
BLUE='\033[94m'
GREEN='\033[92m'
RESET='\033[0m'

# Get project root (two levels up from .tools/project)
TOOLS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ROOT_DIR="$TOOLS_DIR/../.."
TEST_DATA_DIR="$ROOT_DIR/test/data"

echo -e "${BLUE}[INFO]${RESET} Cleaning prediction directories in test/data..."

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
