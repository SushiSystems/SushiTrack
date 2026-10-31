# history.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""Reads the commits after a base from a repository, for the checkers that judge each commit."""

from __future__ import annotations

import subprocess
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True, slots=True)
class Commit:
    """Holds one commit to check: where it is, its hash, its parent and its message."""

    root: Path
    sha: str
    parent: str
    message: str


def git(root: Path, *arguments: str) -> str | None:
    """Returns the output of a git command, or None when it fails."""
    try:
        completed = subprocess.run(
            ["git", "-C", str(root), *arguments], capture_output=True, text=True, encoding="utf-8")
    except OSError:
        return None
    return completed.stdout if completed.returncode == 0 else None


def commits_since(root: Path, since: str) -> list[Commit]:
    """Returns each non-merge commit after *since* below HEAD, oldest first; stops when git cannot read them."""
    listing = git(root, "rev-list", "--no-merges", "--reverse", f"{since}..HEAD")
    if listing is None:
        raise SystemExit(f"cannot read the history after {since}; fetch the full history")
    commits: list[Commit] = []
    for sha in listing.split():
        parent = git(root, "rev-parse", f"{sha}^")
        message = git(root, "log", "-1", "--format=%B", sha)
        if parent is not None and message is not None:
            commits.append(Commit(root, sha, parent.strip(), message))
    return commits
