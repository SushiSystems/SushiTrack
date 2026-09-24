"""What label's regression run hands to its build environment."""

from __future__ import annotations

from types import SimpleNamespace

from sushitrack_cli.services import label


def test_run_regression_hands_the_tool_settings_to_the_build_env(tmp_path, monkeypatch):
    seen = []
    tool = SimpleNamespace()
    monkeypatch.setattr(label, "find_binary", lambda build, name: tmp_path / name)
    monkeypatch.setattr(label, "load_build_env",
                        lambda t, build: seen.append((t, build)) or {})
    monkeypatch.setattr(label, "RUNNER", SimpleNamespace(capture=lambda *a, **k: (0, "", "")))
    paths = SimpleNamespace(build=tmp_path, root=tmp_path)
    label._run_regression(tool, paths, "*")
    assert seen == [(tool, tmp_path)]
