#!/usr/bin/env python3
"""Launcher for the `sushitrack` CLI.

Runnable directly (`python3 cli/sushitrack.py build`) with no installation and no
PYTHONPATH set up: it puts its own directory on sys.path so the sushitrack_cli
package next to it is importable from any working directory.
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from sushitrack_cli.cli import entry  # noqa: E402  (path set up above)

if __name__ == "__main__":
    sys.exit(entry(prog="sushitrack"))
