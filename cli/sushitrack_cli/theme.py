"""Color theme: the only piece of the presentation layer that knows about styles."""

from dataclasses import dataclass, fields, replace


@dataclass(frozen=True)
class Theme:
    info: str = "bold blue"
    success: str = "bold green"
    warn: str = "bold yellow"
    error: str = "bold red"
    cmd: str = "cyan"
    header: str = "bold #f0a500"
    panel_border: str = "red"
    rule_line: str = "dim"
    # Table chrome and de-emphasised values (paths, sources, hints).
    table_border: str = "dim"
    muted: str = "dim"

    def merged(self, overrides):
        """Return a copy with only the recognized keys in *overrides* applied."""
        known = {f.name for f in fields(self)}
        return replace(self, **{k: v for k, v in overrides.items()
                                if k in known and v})


# Near-monochrome greys plus a single amber accent (#f0a500), matching the palette
_SUSHIWEB = Theme(
    info="bold #9a9a94",
    success="bold #6bbf59",
    warn="bold #d9a441",
    error="bold #e0575c",
    cmd="bold #f0a500",
    header="bold #f0a500",
    panel_border="#e0575c",
    rule_line="dim",
    table_border="dim",
    muted="dim",
)

_PRESETS = {
    "default": _SUSHIWEB,
    "sushiweb": _SUSHIWEB,
    "mono": Theme(
        info="bold", success="bold", warn="bold", error="bold",
        cmd="bold", header="bold", panel_border="bold",
        rule_line="dim", table_border="dim", muted="dim",
    ),
    "muted": Theme(
        info="blue", success="green", warn="yellow", error="red",
        cmd="dim cyan", header="#f0a500", panel_border="red",
    ),
}


def register_theme(name, theme):
    """Add or replace a named preset, so a downstream tool can ship its own."""
    _PRESETS[name] = theme


def get_theme(name):
    try:
        return _PRESETS[name]
    except KeyError:
        known = ", ".join(sorted(_PRESETS))
        raise ValueError(
            f"Unknown CLI theme '{name}'. Known themes: {known}") from None


def known_themes():
    return tuple(sorted(_PRESETS))
