#!/bin/sh
# `ds` entry point. Plain sh, not bash: the CLI must also work in a minimal
# container where bash is not installed. Resolves this script's own directory so
# it can be invoked from anywhere, and forwards every argument untouched.
exec python3 "$(dirname "$0")/ds.py" "$@"
