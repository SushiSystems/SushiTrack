"""Service layer: all CLI behaviour lives here.

Commands in :mod:`sushitrack_cli.cli` only parse arguments and delegate to these
modules, each of which returns a process exit code. Nothing here imports
argparse, so every operation is callable from a script or a test.
"""
