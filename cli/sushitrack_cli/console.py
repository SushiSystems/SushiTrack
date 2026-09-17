"""The public-facing console facade."""

import os
import sys

from .icons import get_icon_set
from .renderer import AnsiRenderer, Table
from .theme import get_theme


class Console:
    def __init__(self, renderer, theme, icons):
        self._renderer = renderer
        self._theme = theme
        self._icons = icons

    @property
    def theme(self):
        return self._theme

    @property
    def icons(self):
        return self._icons

    @property
    def accent(self):
        """The theme's accent style, same one ``header()`` uses. Callers styling
        their own output should read it from here instead of hardcoding a color,
        so they stay in sync with the configured theme."""
        return self._theme.header

    def paint(self, style, text):
        """Style *text* with a theme style, honouring the active color mode.

        The one way for a caller outside this module to color something — going
        through :func:`~sushitrack_cli.renderer.ansi` directly would emit escapes
        even when color is off.
        """
        return self._renderer.paint(style, text)

    def info(self, msg):
        self._renderer.line(self._theme.info, self._icons.info, msg)

    def success(self, msg):
        self._renderer.line(self._theme.success, self._icons.success, msg)

    def warn(self, msg):
        self._renderer.line(self._theme.warn, self._icons.warn, msg)

    def error(self, msg):
        self._renderer.line(self._theme.error, self._icons.error, msg)

    def plain(self, msg=""):
        self._renderer.line("", "", msg)

    def muted(self, msg):
        self._renderer.line(self._theme.muted, "", msg)

    def command(self, cmd):
        """Echo the command about to be executed."""
        self._renderer.command(self._theme.cmd, self._theme.info,
                               self._icons.info, cmd)

    def header(self, title):
        self._renderer.header(title, self._theme.header)

    def fail_panel(self, title, body):
        self._renderer.panel(title, body, self._theme.panel_border)

    def table(self, table):
        self._renderer.table(table)

    def new_table(self, *columns, title=None):
        return Table(*columns, title=title)


def _use_color(mode):
    # NO_COLOR is honoured before the configured mode: it is the user's environment-wi
    if os.environ.get("NO_COLOR"):
        return False
    if mode == "always":
        return True
    if mode == "never":
        return False
    return sys.stdout.isatty()


def build_console(appearance=None):
    """Assemble a :class:`Console` from an appearance spec (see config.py)."""
    from .config import Appearance

    spec = appearance or Appearance()
    theme = get_theme(spec.theme).merged(spec.color_overrides)
    icons = get_icon_set(spec.icons).merged(spec.icon_overrides)
    renderer = AnsiRenderer(theme, no_color=not _use_color(spec.color))
    return Console(renderer, theme, icons)


_console = None

_ATTRS = {
    "info", "success", "warn", "error", "plain", "muted", "paint",
    "command", "header", "fail_panel", "table", "new_table",
    "accent", "theme", "icons",
}


def _get_console():
    global _console
    if _console is None:
        try:
            from .config import load_appearance

            spec = load_appearance()
        except SystemExit:
            spec = None  # outside a checkout: default theme is good enough
        _console = build_console(spec)
    return _console


def __getattr__(name):
    if name in _ATTRS:
        return getattr(_get_console(), name)
    raise AttributeError(f"module {__name__!r} has no attribute {name!r}")
