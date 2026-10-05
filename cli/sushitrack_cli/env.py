# env.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026 Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""Assembles the environment st's build, test and run children see."""

from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path

from sushicore.build_env import merge_env, read_cache, snapshot_vcvars, write_cache
from sushicore.provision.probe import find_vcvars

from . import console

CACHE_NAME = ".sushitrack_env.json"


def cache_key(tool) -> str:
    """Hash the settings that decide what a vcvars snapshot contains."""
    material = json.dumps({"platform": tool.platform, "vs_vcvars": tool.vs_vcvars},
                          sort_keys=True)
    return hashlib.sha256(material.encode()).hexdigest()


def vcvars_path(tool) -> Path | None:
    """Return the configured vcvars64.bat when it exists, else the detected one."""
    configured = tool.expand(tool.vs_vcvars) if tool.vs_vcvars else ""
    if configured and Path(configured).is_file():
        return Path(configured)
    return find_vcvars()


def _snapshot(tool) -> dict[str, str] | None:
    """Return a fresh vcvars snapshot, or None when there is nothing to take."""
    if os.environ.get("VSINSTALLDIR"):
        return None
    vcvars = vcvars_path(tool)
    if vcvars is None:
        console.warn("No Visual Studio installation detected; "
                     "assuming the compiler is already on PATH.")
        return None
    return snapshot_vcvars(vcvars, console)


def load_build_env(tool, build_dir) -> dict[str, str]:
    """Return the environment for build, test and run children.

    Args:
        tool: The resolved `cfg.tool` settings.
        build_dir: Where the snapshot cache lives.

    Returns:
        `os.environ` off Windows or without a snapshot; else it merged with one.
    """
    if not tool.is_windows:
        return dict(os.environ)
    cache_file = Path(build_dir) / CACHE_NAME
    key = cache_key(tool)
    snapshot = read_cache(cache_file, key)
    if snapshot is None:
        snapshot = _snapshot(tool)
        if not snapshot:
            return dict(os.environ)
        try:
            Path(build_dir).mkdir(parents=True, exist_ok=True)
        except OSError:
            return merge_env(os.environ, snapshot)
        write_cache(cache_file, key, snapshot)
    return merge_env(os.environ, snapshot)
