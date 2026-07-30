"""SushiTrack developer CLI (`ds`).

A dependency-free command line tool: the only requirement is a python3
interpreter, so it runs unchanged in a bare `python:3-slim` container. The
presentation layer (theme / icons / renderer / console) mirrors the Sushi CLIs'
architecture, with argparse in place of Typer and ANSI in place of Rich.

Layout:
    cli.py        command declarations, one thin layer over the services
    app.py        the declarative argparse layer (Typer replacement)
    config.py     layered JSON config + path resolution
    env.py        MSVC environment snapshotting for Windows builds
    proc.py       subprocess helpers
    console.py    semantic output facade
    theme.py      colors        icons.py   glyphs        renderer.py  ANSI backend
    services/     all behaviour: build, tests, evaluate, deploy, docker, diag
"""

__version__ = "2.0.0"
