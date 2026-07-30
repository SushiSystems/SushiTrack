"""Enable `python3 -m sushitrack_cli` as an entry point."""

import sys

from .cli import entry

if __name__ == "__main__":
    sys.exit(entry())
