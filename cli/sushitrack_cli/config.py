"""Layered configuration and path resolution.

Precedence (lowest to highest):
    built-in defaults -> config.json -> config.local.json -> DS_* env vars

Within a file, the active platform's ``platform.<name>`` table is merged over
the top-level sections, so one file describes both Linux and Windows.

JSON rather than TOML on purpose: ``tomllib`` only exists on Python 3.11+, and
the whole point of this CLI is that a bare ``python3`` is the only requirement.
This module replaces the old ``paths.env`` + ``paths_manager.py`` pair — the same
overrides now live in the ``paths`` section, resolved relative to the repo root.
"""

import json
import os
import platform
from dataclasses import dataclass, field, fields
from pathlib import Path

# Files that identify the repo root. CMakeLists.txt alone is too weak — any
# enclosing C++ checkout would match — so the public header must be there too.
_ROOT_MARKERS = ("CMakeLists.txt", "include/SushiTrackC.h")

CONFIG_NAMES = ("config.json", "config.local.json")

IS_WINDOWS = platform.system() == "Windows"


@dataclass
class Appearance:
    """The ``cli`` section: everything the presentation layer needs."""

    theme: str = "default"
    icons: str = "text"
    color: str = "auto"  # auto | always | never
    color_overrides: dict = field(default_factory=dict)
    icon_overrides: dict = field(default_factory=dict)


@dataclass
class Config:
    """Resolved configuration. Field names are ``<section>_<key>``, which is how
    :func:`_apply_document` maps a nested JSON document onto this flat record
    without a per-key table to maintain."""

    cli_theme: str = "default"
    cli_icons: str = "text"
    cli_color: str = "auto"

    paths_build: str = "build"
    paths_data: str = "data"
    paths_third_party: str = "3rdparty"
    paths_tests: str = "tests"
    paths_regression: str = "tests/regression"
    paths_evaluator: str = "tests/regression/evaluator"
    paths_inference: str = "tests/regression/inference"
    paths_package: str = "package"

    build_type: str = "Release"
    # Empty means "let cmake pick its default generator for the platform".
    build_generator: str = ""
    build_jobs: int = 0  # 0 -> os.cpu_count()
    build_cmake: str = "cmake"
    build_ctest: str = "ctest"

    docker_image: str = "sushitrack"
    docker_workdir: str = "/workspace/sushitrack"
    docker_gpus: bool = True

    test_tracker: str = "sushitrack"

    # Populated by the loader, not read from any file: field name -> origin
    # ("default", "config.json", "config.local.json", "env:DS_...").
    sources: dict = field(default_factory=dict)

    def expand(self, value):
        """Expand ``~`` and ``$VAR`` in a configured path-like string."""
        return os.path.expandvars(os.path.expanduser(value)) if value else value

    def jobs(self):
        return self.build_jobs if self.build_jobs > 0 else (os.cpu_count() or 2)


# Config field -> environment variable that overrides it.
ENV_OVERRIDES = {
    "cli_theme": "DS_THEME",
    "cli_icons": "DS_ICONS",
    "cli_color": "DS_COLOR",
    "paths_build": "DS_BUILD_DIR",
    "paths_data": "DS_DATA_DIR",
    "paths_third_party": "DS_3RDPARTY_DIR",
    "build_type": "DS_BUILD_TYPE",
    "build_generator": "DS_GENERATOR",
    "build_jobs": "DS_JOBS",
    "build_cmake": "DS_CMAKE",
    "build_ctest": "DS_CTEST",
    "docker_image": "DS_DOCKER_IMAGE",
    "docker_gpus": "DS_DOCKER_GPUS",
    "test_tracker": "DS_TRACKER",
}


def _has_markers(directory):
    return all((directory / marker).exists() for marker in _ROOT_MARKERS)


def find_project_root(start=None):
    """Locate the repo root by walking up from the invocation directory.

    The CLI may be invoked through a wrapper on PATH from anywhere inside the
    checkout, so the caller's directory is what identifies the project — not the
    location of this file, which also works but breaks the moment the CLI is
    copied or installed elsewhere. ``DS_PROJECT_ROOT`` overrides the search,
    which is what makes the CLI usable when it ships without the sources next to
    it (a bare-python container, a mounted volume).
    """
    override = os.environ.get("DS_PROJECT_ROOT")
    if override:
        return Path(os.path.expandvars(os.path.expanduser(override))).resolve()

    here = Path(start or Path.cwd()).resolve()
    for candidate in (here, *here.parents):
        if _has_markers(candidate):
            return candidate
    # Fall back to the checkout this file lives in (cli/sushitrack_cli/config.py).
    packaged = Path(__file__).resolve().parents[2]
    if _has_markers(packaged):
        return packaged
    raise SystemExit(
        "Not inside a SushiTrack project: no CMakeLists.txt + include/SushiTrackC.h "
        "found in the current directory or any parent. cd into the repo, or set "
        "DS_PROJECT_ROOT to point at it."
    )


def config_dir(root=None):
    """Directory holding config.json / config.local.json (the repo's cli/)."""
    return (root or find_project_root()) / "cli"


def _read_json(path):
    try:
        with open(path, "r", encoding="utf-8") as handle:
            return json.load(handle)
    except FileNotFoundError:
        return {}
    except (json.JSONDecodeError, OSError) as exc:
        raise SystemExit(f"Cannot read {path}: {exc}")


def _merge_platform(document):
    """Overlay ``platform.<current>`` onto the document's other sections."""
    plat = platform.system().lower()
    overlay = (document.get("platform") or {}).get(plat) or {}
    merged = {k: dict(v) if isinstance(v, dict) else v
              for k, v in document.items() if k != "platform"}
    for section, values in overlay.items():
        if isinstance(values, dict):
            merged.setdefault(section, {}).update(values)
        else:
            merged[section] = values
    return merged


def _coerce(current, raw):
    """Coerce a config/env value to the type of the existing field value."""
    if isinstance(current, bool):
        return str(raw).strip().lower() in ("1", "true", "yes", "on")
    if isinstance(current, int) and not isinstance(raw, bool):
        try:
            return int(raw)
        except (TypeError, ValueError):
            return current
    return raw


def _apply_document(cfg, document, origin):
    known = {f.name for f in fields(cfg)}
    for section, values in _merge_platform(document).items():
        if not isinstance(values, dict):
            continue
        for key, value in values.items():
            name = f"{section}_{key}"
            if name in known:
                setattr(cfg, name, _coerce(getattr(cfg, name), value))
                cfg.sources[name] = origin


def _apply_env(cfg):
    for name, var in ENV_OVERRIDES.items():
        raw = os.environ.get(var)
        if raw is not None and raw != "":
            setattr(cfg, name, _coerce(getattr(cfg, name), raw))
            cfg.sources[name] = f"env:{var}"


def load_config(root=None):
    """Load and resolve the layered configuration for the current platform."""
    cfg = Config()
    cfg.sources = {f.name: "default" for f in fields(cfg) if f.name != "sources"}
    try:
        cfg_dir = config_dir(root)
    except SystemExit:
        _apply_env(cfg)  # still usable outside a checkout (e.g. `ds doctor`)
        return cfg
    for name in CONFIG_NAMES:
        _apply_document(cfg, _read_json(cfg_dir / name), name)
    _apply_env(cfg)
    return cfg


def load_appearance(root=None):
    """The ``cli`` section plus its free-form override tables.

    Read separately from :func:`load_config` so building the console never
    depends on the rest of the schema being valid.
    """
    cfg = load_config(root)
    document = {}
    try:
        cfg_dir = config_dir(root)
    except SystemExit:
        cfg_dir = None
    if cfg_dir is not None:
        for name in CONFIG_NAMES:
            section = _merge_platform(_read_json(cfg_dir / name)).get("cli") or {}
            document.update(section)
    return Appearance(
        theme=cfg.cli_theme,
        icons=cfg.cli_icons,
        color=cfg.cli_color,
        color_overrides=document.get("colors") or {},
        icon_overrides=document.get("icon_overrides") or {},
    )


class Paths:
    """Absolute project directories, resolved from the ``paths`` section.

    A configured value may be relative (joined to the repo root) or absolute,
    which is what lets a data set live outside the checkout.
    """

    def __init__(self, cfg, root):
        self.root = root
        self.cli = root / "cli"
        self.build = self._resolve(cfg, cfg.paths_build)
        self.data = self._resolve(cfg, cfg.paths_data)
        self.third_party = self._resolve(cfg, cfg.paths_third_party)
        self.tests = self._resolve(cfg, cfg.paths_tests)
        self.regression = self._resolve(cfg, cfg.paths_regression)
        self.evaluator = self._resolve(cfg, cfg.paths_evaluator)
        self.inference = self._resolve(cfg, cfg.paths_inference)
        self.package = self._resolve(cfg, cfg.paths_package)

    def _resolve(self, cfg, value):
        expanded = Path(cfg.expand(value))
        return expanded if expanded.is_absolute() else self.root / expanded

    def data_root(self):
        """Where MOT sequences live: the evaluator's own data/ wins when present,
        matching how the regression binary and TrackEval both look it up."""
        eval_data = self.evaluator / "data"
        return eval_data if eval_data.exists() else self.data

    def as_dict(self):
        return {
            "root": self.root,
            "cli": self.cli,
            "build": self.build,
            "data": self.data,
            "data_root": self.data_root(),
            "third_party": self.third_party,
            "tests": self.tests,
            "regression": self.regression,
            "evaluator": self.evaluator,
            "inference": self.inference,
            "package": self.package,
        }


def context(root=None):
    """The (config, paths) pair every service starts from."""
    resolved_root = root or find_project_root()
    cfg = load_config(resolved_root)
    return cfg, Paths(cfg, resolved_root)
