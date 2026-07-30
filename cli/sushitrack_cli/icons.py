"""Icon/prefix sets: the text glyphs printed before a message.

Kept separate from :mod:`sushitrack_cli.theme` (which owns only color) so the two
can be swapped independently — e.g. keep the colors but drop to ASCII prefixes
on a console that cannot render emoji.
"""

from dataclasses import dataclass, fields, replace


@dataclass(frozen=True)
class IconSet:
    info: str = "[INFO]"
    success: str = "[SUCCESS]"
    warn: str = "[WARN]"
    error: str = "[ERROR]"
    ok: str = "+"
    fail: str = "x"

    def merged(self, overrides):
        known = {f.name for f in fields(self)}
        return replace(self, **{k: v for k, v in overrides.items()
                                if k in known and v})


_PRESETS = {
    "text": IconSet(),
    "emoji": IconSet(info="i", success="OK", warn="!", error="X",
                     ok="✔", fail="✘"),
    "minimal": IconSet(info="i", success="+", warn="!", error="x"),
    "none": IconSet(info="", success="", warn="", error=""),
}


def register_icon_set(name, icons):
    _PRESETS[name] = icons


def get_icon_set(name):
    try:
        return _PRESETS[name]
    except KeyError:
        known = ", ".join(sorted(_PRESETS))
        raise ValueError(
            f"Unknown CLI icon set '{name}'. Known icon sets: {known}") from None


def known_icon_sets():
    return tuple(sorted(_PRESETS))
