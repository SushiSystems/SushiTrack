# config.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""Loads SushiTrack's layered configuration and resolves its paths.

Root and config-directory resolution belongs to :class:`sushicore.module_config.ModuleConfig`.
The sectioned tables (``cli``, ``paths``, ``build``, ``docker``, ``test``) load here; the
``[tool]`` table loads as :class:`ToolSettings` through sushicore, workspace-aware.
"""

from __future__ import annotations

import os
import platform
from dataclasses import dataclass, field, fields
from pathlib import Path

from sushicore.config_base import load_tool_config
from sushicore.module_config import ModuleConfig
from sushicore.profile import ModuleProfile
from sushicore.stack_config import StackConfig
from sushicore.workspace import read_toml

#: The compiler a consumer's package build hands the CLI; it outranks the configured one.
K_STACK_CXX_ENV = "SUSHISTACK_CXX"

#: root_marker exists only at the SushiTrack root; see docs/architecture/CLI_PACKAGE.md.
PROFILE = ModuleProfile(
    name="SushiTrack",
    program="st",
    env_prefix="ST",
    root_marker="include/SushiTrack/c_api/sushitrack_c.h",
    env_of_interest=("EIGEN", "SUSHITRACK"),
    extra_env_overrides={
        "cmake_exe": "ST_CMAKE",
        "ctest_exe": "ST_CTEST",
        "generator": "ST_GENERATOR",
    },
)

_MODULE = ModuleConfig(PROFILE, use_workspace_config=True)

find_project_root = _MODULE.find_project_root
config_dir = _MODULE.config_dir

CONFIG_NAMES = ("config.toml", "config.local.toml")

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
class ToolSettings(StackConfig):
    """Holds the ``[tool]`` section: host build tools and provisioned toolchain roots."""

    generator: str = ""

    # The compiler SUSHISTACK_CXX names; empty outside a package build. It outranks `cxx`.
    stack_cxx: str = ""

    def resolved_compiler(self, root) -> str:
        """Return the C++ compiler: SUSHISTACK_CXX, else the one the shared resolution finds."""
        if self.stack_cxx:
            return self.expand(self.stack_cxx)
        return super().resolved_compiler(root)


@dataclass
class Config:
    """Resolved configuration. Field names are ``<section>_<key>``, which is how
    :func:`_apply_document` maps a nested TOML document onto this flat record
    without a per-key table to maintain."""

    cli_theme: str = "default"
    cli_icons: str = "text"
    cli_color: str = "auto"

    paths_source: str = "source"
    paths_include: str = "include/SushiTrack"
    paths_build: str = "build"
    paths_data: str = "data"
    paths_third_party: str = "third_party"
    paths_config: str = "assets/sushitrack.json"
    paths_tests: str = "tests"
    paths_bridge: str = "applications/tracker_bridge"
    paths_benchmarks: str = "applications/benchmarks"
    paths_evaluator: str = "applications/benchmarks/mot_engine/evaluator"
    paths_inference: str = "applications/inference"
    paths_package: str = "package"

    build_type: str = "Release"
    build_jobs: int = 0  # 0 -> os.cpu_count()

    docker_image: str = "sushitrack"
    docker_workdir: str = "/workspace/sushitrack"
    docker_gpus: bool = True

    test_tracker: str = "sushitrack"

    # sources maps each field name to the layer its value came from.
    sources: dict = field(default_factory=dict)

    tool: ToolSettings = field(default_factory=ToolSettings)

    def expand(self, value):
        """Expand ``~`` and ``$VAR`` in a configured path-like string."""
        return os.path.expandvars(os.path.expanduser(value)) if value else value

    def jobs(self):
        return self.build_jobs if self.build_jobs > 0 else (os.cpu_count() or 2)


# Maps each config field to its overriding variable; see docs/architecture/CLI_PACKAGE.md.
ENV_OVERRIDES = {
    "cli_theme": "ST_THEME",
    "cli_icons": "ST_ICONS",
    "cli_color": "ST_COLOR",
    "paths_source": "ST_SOURCE_DIR",
    "paths_include": "ST_INCLUDE_DIR",
    "paths_build": "ST_BUILD_DIR",
    "paths_data": "ST_DATA_DIR",
    "paths_third_party": "ST_THIRD_PARTY_DIR",
    "paths_config": "ST_CONFIG_FILE",
    "build_type": "ST_BUILD_TYPE",
    "build_jobs": "ST_JOBS",
    "docker_image": "ST_DOCKER_IMAGE",
    "docker_gpus": "ST_DOCKER_GPUS",
    "test_tracker": "ST_TRACKER",
}


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


#: Legacy ``[build]`` key -> the ``[tool]`` key that replaced it.
_LEGACY_TOOL_KEYS = {"cmake": "cmake_exe", "ctest": "ctest_exe", "generator": "generator"}


def legacy_tool_keys(document):
    """List the legacy build-tool keys in a raw TOML document.

    Returns:
        ``(old key, replacement)`` pairs, ``[build]`` first, then each ``[platform.<os>.build]``.
    """
    found = [(f"[build] {key}", f"[tool] {new}")
             for key, new in _LEGACY_TOOL_KEYS.items() if key in (document.get("build") or {})]
    for plat, overlay in (document.get("platform") or {}).items():
        build = (overlay or {}).get("build") or {}
        found += [(f"[platform.{plat}.build] {key}", f"[tool] {new}")
                  for key, new in _LEGACY_TOOL_KEYS.items() if key in build]
    return found


def _warn(message):
    """Print one config warning through st's console."""
    from . import console  # console imports this module
    console.warn(message)


def _coerce(current, raw, name=""):
    """Coerce a config/env value to the type of the existing field value.

    A value an integer field cannot read keeps the current one and says so under *name*.
    """
    if isinstance(current, bool):
        return str(raw).strip().lower() in ("1", "true", "yes", "on")
    if isinstance(current, int) and not isinstance(raw, bool):
        try:
            return int(raw)
        except (TypeError, ValueError):
            _warn(f"{name}: {raw!r} is not an integer; keeping {current}")
            return current
    return raw


def _legacy_pairs(document):
    """The (section, key) pairs `legacy_tool_keys` already names, so they are not warned twice."""
    pairs = set()
    for old, _new in legacy_tool_keys(document):
        header, _, key = old.rpartition("] ")
        pairs.add((header.lstrip("[").split(".")[-1], key))
    return pairs


def _apply_document(cfg, document, origin):
    known = {f.name for f in fields(cfg)}
    legacy = _legacy_pairs(document)
    for section, values in _merge_platform(document).items():
        if not isinstance(values, dict):
            continue
        for key, value in values.items():
            name = f"{section}_{key}"
            if name in known:
                setattr(cfg, name, _coerce(getattr(cfg, name), value, f"{origin}: {section}.{key}"))
                cfg.sources[name] = origin
            elif (section, key) not in legacy:
                _warn(f"{origin}: {section}.{key} is not a setting and is ignored")


def _apply_env(cfg):
    for name, var in ENV_OVERRIDES.items():
        raw = os.environ.get(var)
        if raw is not None and raw != "":
            setattr(cfg, name, _coerce(getattr(cfg, name), raw, f"env:{var}"))
            cfg.sources[name] = f"env:{var}"


def load_tool(root=None):
    """Load the ``[tool]`` section through sushicore's layered loader.

    Returns:
        The workspace-aware settings inside a checkout, else defaults with env overrides.
    """
    try:
        config_dir(root)
    except SystemExit:
        plat = platform.system().lower()
        tool = load_tool_config(ToolSettings, [], plat, PROFILE.env_overrides())
    else:
        tool = _MODULE.load(ToolSettings)
    tool.stack_cxx = os.environ.get(K_STACK_CXX_ENV, "")
    return tool


def load_config(root=None):
    """Load and resolve the layered configuration for the current platform."""
    cfg = Config()
    cfg.sources = {f.name: "default" for f in fields(cfg)
                   if f.name not in ("sources", "tool")}
    cfg.tool = load_tool(root)
    try:
        cfg_dir = config_dir(root)
    except SystemExit:
        _apply_env(cfg)  # still usable outside a checkout (e.g. `st doctor`)
        return cfg
    for name in CONFIG_NAMES:
        document = read_toml(cfg_dir / name)
        for old, new in legacy_tool_keys(document):
            _warn(f"{name}: {old} is ignored; use {new}")
        _apply_document(cfg, document, name)
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
            section = _merge_platform(read_toml(cfg_dir / name)).get("cli") or {}
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
        self.source = self._resolve(cfg, cfg.paths_source)
        self.include = self._resolve(cfg, cfg.paths_include)
        self.build = self._resolve(cfg, cfg.paths_build)
        self.data = self._resolve(cfg, cfg.paths_data)
        self.third_party = self._resolve(cfg, cfg.paths_third_party)
        self.config = self._resolve(cfg, cfg.paths_config)
        self.tests = self._resolve(cfg, cfg.paths_tests)
        self.bridge = self._resolve(cfg, cfg.paths_bridge)
        self.benchmarks = self._resolve(cfg, cfg.paths_benchmarks)
        self.evaluator = self._resolve(cfg, cfg.paths_evaluator)
        self.inference = self._resolve(cfg, cfg.paths_inference)
        self.package = self._resolve(cfg, cfg.paths_package)

    def _resolve(self, cfg, value):
        expanded = Path(cfg.expand(value))
        return expanded if expanded.is_absolute() else self.root / expanded

    def benchmark_names(self):
        """The benchmarks with sequences: each folder of `benchmarks` or of `data` that holds data/."""
        names = {entry.name for root in (self.benchmarks, self.data) if root.is_dir()
                 for entry in root.iterdir() if (entry / "data").is_dir()}
        return sorted(names)

    def data_root(self, benchmark):
        """Where *benchmark*'s sequences live: its folder under `benchmarks`, else under `data`."""
        own = self.benchmarks / benchmark / "data"
        return own if own.is_dir() else self.data / benchmark / "data"

    def as_dict(self):
        return {
            "root": self.root,
            "cli": self.cli,
            "source": self.source,
            "include": self.include,
            "build": self.build,
            "data": self.data,
            "benchmarks": self.benchmarks,
            "third_party": self.third_party,
            "config": self.config,
            "tests": self.tests,
            "bridge": self.bridge,
            "evaluator": self.evaluator,
            "inference": self.inference,
            "package": self.package,
        }


def context(root=None):
    """The (config, paths) pair every service starts from."""
    resolved_root = root or find_project_root()
    cfg = load_config(resolved_root)
    return cfg, Paths(cfg, resolved_root)
