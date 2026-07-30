"""Rendering backends — the only modules that emit escape codes.

``Renderer`` is the seam the rest of the package depends on: the
:class:`~sushitrack_cli.console.Console` facade talks to this interface and never
formats output itself, so a plain-text or JSON backend is a drop-in replacement.

Everything here is standard library only. Rich's job (style parsing, tables,
panels, rules) is covered by :func:`ansi` plus the small layout helpers below;
the style vocabulary is deliberately a subset of Rich's so themes remain
interchangeable with the Sushi CLIs.
"""

import os
import re
import shutil
import sys

RESET = "\033[0m"

_ATTRS = {
    "bold": 1,
    "dim": 2,
    "italic": 3,
    "underline": 4,
    "reverse": 7,
    "strike": 9,
}

_COLORS = {
    "black": 30, "red": 31, "green": 32, "yellow": 33,
    "blue": 34, "magenta": 35, "cyan": 36, "white": 37,
    "bright_black": 90, "bright_red": 91, "bright_green": 92,
    "bright_yellow": 93, "bright_blue": 94, "bright_magenta": 95,
    "bright_cyan": 96, "bright_white": 97,
    # "default" means the terminal's own foreground rather than a fixed color,
    # so it stays readable on both dark and light backgrounds.
    "default": 39,
}

_HEX = re.compile(r"^#([0-9a-fA-F]{6})$")


def ansi(style):
    """Compile a Rich-like style string ("bold #f0a500") into an ANSI prefix.

    Unknown tokens are ignored rather than raising: a theme comes from user
    config, and a typo there should degrade to plainer output, not abort the
    command that was trying to print something.
    """
    if not style:
        return ""
    codes = []
    for token in style.split():
        low = token.lower()
        if low in _ATTRS:
            codes.append(str(_ATTRS[low]))
        elif low in _COLORS:
            codes.append(str(_COLORS[low]))
        elif low.startswith("on_") and low[3:] in _COLORS:
            codes.append(str(_COLORS[low[3:]] + 10))
        else:
            m = _HEX.match(token)
            if m:
                r, g, b = (int(m.group(1)[i:i + 2], 16) for i in (0, 2, 4))
                codes.append(f"38;2;{r};{g};{b}")
    return f"\033[{';'.join(codes)}m" if codes else ""


def terminal_width(default=100):
    try:
        return shutil.get_terminal_size((default, 24)).columns
    except OSError:
        return default


def _enable_windows_vt():
    """Turn on ANSI processing for the current console (Windows 10+).

    A legacy conhost prints raw escape codes instead of interpreting them; the
    flag is per-handle and cheap to set, and modern Terminal has it on already.
    """
    if os.name != "nt":
        return
    try:
        import ctypes

        kernel32 = ctypes.windll.kernel32
        for handle_id in (-11, -12):  # stdout, stderr
            handle = kernel32.GetStdHandle(handle_id)
            mode = ctypes.c_uint32()
            if kernel32.GetConsoleMode(handle, ctypes.byref(mode)):
                kernel32.SetConsoleMode(handle, mode.value | 0x0004)
    except Exception:
        pass  # not a console, or an OS without the flag; output still works


def _force_utf8_streams():
    # A legacy Windows console defaults to cp1252, which cannot encode the box
    # characters used by tables; without this the first table raises
    # UnicodeEncodeError and turns rendering into a traceback.
    for stream in (sys.stdout, sys.stderr):
        reconfigure = getattr(stream, "reconfigure", None)
        if reconfigure is not None:
            try:
                reconfigure(encoding="utf-8", errors="replace")
            except (ValueError, OSError):
                pass


class Table:
    """Accumulates rows, then renders through a :class:`Renderer`.

    Columns are sized to their content and the widest one is folded when the
    total would exceed the terminal width, which is what long absolute paths in
    `ds config` / `ds paths` need.
    """

    def __init__(self, *columns, title=None):
        self.title = title
        self.columns = list(columns)
        self.rows = []
        self.styles = [None] * len(self.columns)

    def column_style(self, index, style):
        self.styles[index] = style
        return self

    def add_row(self, *cells):
        """Add a row. A cell may be a ``(text, style)`` pair to override the
        column's style for that one cell — what `ds doctor` needs to color a
        result green or red per check."""
        row = []
        for cell in cells:
            if isinstance(cell, tuple):
                text, style = cell
                row.append(("" if text is None else str(text), style))
            else:
                row.append((("" if cell is None else str(cell)), None))
        self.rows.append(row)
        return self


class AnsiRenderer:
    """Colored output via raw ANSI escape codes.

    ``no_color=True`` keeps every layout decision identical and only drops the
    escape sequences, so piping to a file or CI log yields the same tables.
    """

    def __init__(self, theme, no_color=False, stream=None):
        self._theme = theme
        self._no_color = no_color
        self._stream = stream or sys.stdout
        if not no_color:
            _enable_windows_vt()
        _force_utf8_streams()

    def paint(self, style, text):
        prefix = "" if self._no_color else ansi(style)
        return f"{prefix}{text}{RESET}" if prefix else text

    def _write(self, text=""):
        # Flushed on every line: a child process writes straight to the same
        # terminal, so anything left in our buffer would surface after the
        # subprocess output it was supposed to introduce.
        print(text, file=self._stream, flush=True)

    def line(self, style, prefix, message):
        lead = f"{self.paint(style, prefix)} " if prefix else ""
        self._write(f"{lead}{message}")

    def command(self, cmd_style, prefix_style, prefix, cmd):
        lead = f"{self.paint(prefix_style, prefix)} " if prefix else ""
        self._write(f"{lead}Executing: {self.paint(cmd_style, cmd)}")

    def header(self, title, style):
        width = min(terminal_width(), 100)
        painted = self.paint(style, title)
        dashes = max(width - len(title) - 4, 0)
        left = 3
        right = max(dashes - left, 0)
        rule = self.paint(self._theme.rule_line, "─" * left)
        tail = self.paint(self._theme.rule_line, "─" * right)
        self._write()
        self._write(f"{rule} {painted} {tail}")

    def panel(self, title, body, border_style):
        width = min(terminal_width(), 100)
        inner = width - 4
        lines = []
        for raw in body.splitlines() or [""]:
            lines.extend(_wrap(raw, inner) or [""])
        cap = f"─ {title} " if title else ""
        top = "┌" + cap + "─" * (width - 2 - len(cap)) + "┐"
        self._write(self.paint(border_style, top))
        for text in lines:
            self._write(self.paint(border_style, "│ ") + text.ljust(inner)
                        + self.paint(border_style, " │"))
        self._write(self.paint(border_style,
                                "└" + "─" * (width - 2) + "┘"))

    def table(self, table):
        widths = _column_widths(table)
        border = self._theme.table_border
        head = "  ".join(
            self.paint(self._theme.header, name.ljust(widths[i]))
            for i, name in enumerate(table.columns))
        if table.title:
            self._write(self.paint(self._theme.header, table.title))
        self._write(head)
        self._write(self.paint(border, "  ".join("─" * w for w in widths)))
        for row in table.rows:
            for line in _row_lines(row, widths):
                cells = []
                for i, (cell, cell_style) in enumerate(line):
                    text = cell.ljust(widths[i]) if i < len(widths) - 1 else cell
                    style = cell_style or (table.styles[i]
                                           if i < len(table.styles) else None)
                    cells.append(self.paint(style, text) if style else text)
                self._write("  ".join(cells))


class PlainRenderer(AnsiRenderer):
    """Layout-identical output with no escape codes at all — for a dumb log sink."""

    def __init__(self, theme, stream=None):
        super().__init__(theme, no_color=True, stream=stream)


def _column_widths(table):
    """Size columns to content, folding the widest one to fit the terminal."""
    count = len(table.columns)
    widths = [len(c) for c in table.columns]
    for row in table.rows:
        for i in range(min(count, len(row))):
            widths[i] = max(widths[i], len(row[i][0]))
    available = min(terminal_width(), 160) - 2 * (count - 1)
    overflow = sum(widths) - available
    if overflow > 0:
        widest = widths.index(max(widths))
        widths[widest] = max(widths[widest] - overflow, 24)
    return widths


def _row_lines(row, widths):
    """Split one row into the physical (text, style) lines needed after folding."""
    cells = row[:len(widths)]
    wrapped = [_wrap(text, widths[i]) or [""]
               for i, (text, _) in enumerate(cells)]
    height = max(len(lines) for lines in wrapped)
    return [[(lines[i] if i < len(lines) else "", cells[column][1])
             for column, lines in enumerate(wrapped)]
            for i in range(height)]


def _wrap(text, width):
    """Hard-wrap at *width*, preferring a break at whitespace or a separator.

    Paths and cmake command lines have no spaces for whole stretches, so a
    word-only wrapper would leave them overflowing; breaking on the path
    separators keeps them readable.
    """
    if width <= 0 or len(text) <= width:
        return [text]
    lines = []
    remaining = text
    while len(remaining) > width:
        window = remaining[:width + 1]
        cut = max(window.rfind(" "), window.rfind("/"), window.rfind("\\"),
                  window.rfind(";"), window.rfind(","))
        if cut <= 0:
            cut = width
        elif remaining[cut] in "/\\;,":
            cut += 1  # keep the separator on the line it belongs to
        lines.append(remaining[:cut].rstrip())
        # Drop the whitespace we broke on; keeping it would indent the
        # continuation line by one column.
        remaining = remaining[cut:].lstrip(" ")
    if remaining:
        lines.append(remaining)
    return lines
