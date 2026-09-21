"""What `env` does today, pinned before it moves onto sushicore.

The module assembles the environment a build runs under: on Windows it snapshots
what vcvars64.bat sets and caches the result beside the build tree. Nothing here
runs vcvars — the capture it goes through is patched and the tests say what the
module does with the output it is handed.
"""

from __future__ import annotations

import json
import os

from sushitrack_cli import env


def test_merge_prepends_path_rather_than_replacing_it():
    """A snapshot adds directories; it does not take the existing ones away."""
    merged = env._merge({"PATH": "/usr/bin"}, {"PATH": "/opt/tool/bin"})

    assert merged["PATH"] == "/opt/tool/bin" + os.pathsep + "/usr/bin"


def test_merge_writes_path_back_into_the_spelling_already_there():
    """Two spellings of PATH in one env is how a directory silently vanishes."""
    merged = env._merge({"Path": "C:/Windows"}, {"PATH": "C:/tool"})

    assert "PATH" not in merged
    assert merged["Path"] == "C:/tool" + os.pathsep + "C:/Windows"


def test_merge_takes_the_overlay_for_every_other_key():
    """Only PATH accumulates; the rest is what the snapshot said."""
    merged = env._merge({"INCLUDE": "old", "KEEP": "yes"}, {"INCLUDE": "new"})

    assert merged["INCLUDE"] == "new"
    assert merged["KEEP"] == "yes"


def test_merge_leaves_the_base_alone():
    """The caller's env is an input, not a thing to write through."""
    base = {"PATH": "/usr/bin"}
    env._merge(base, {"PATH": "/opt/bin"})

    assert base == {"PATH": "/usr/bin"}


def test_off_windows_the_current_environment_is_used_as_is(tmp_path, monkeypatch):
    """There is nothing to snapshot: the compiler is already reachable."""
    monkeypatch.setattr(env, "IS_WINDOWS", False)

    assert env.load_build_env(tmp_path) == dict(os.environ)


def test_a_cached_snapshot_is_read_back(tmp_path, monkeypatch):
    """The point of the cache: vcvars runs once per build tree, not per command."""
    monkeypatch.setattr(env, "IS_WINDOWS", True)
    monkeypatch.setattr(env, "_snapshot_windows",
                        lambda: (_ for _ in ()).throw(AssertionError("vcvars ran")))
    (tmp_path / env.CACHE_NAME).write_text(
        json.dumps({"env": {"INCLUDE": "C:/from-cache"}}), encoding="utf-8")

    assembled = env.load_build_env(tmp_path)

    assert assembled["INCLUDE"] == "C:/from-cache"


def test_a_cache_is_reused_whatever_produced_it(tmp_path, monkeypatch):
    """Today's behaviour, and a defect: the file carries no key.

    Nothing records which Visual Studio produced the snapshot, so a cache stays
    in use after the compiler that made it is gone. `sushicore.build_env`'s
    read_cache takes a key and answers None when it does not match, so moving
    onto it is expected to change this test rather than keep it.
    """
    monkeypatch.setattr(env, "IS_WINDOWS", True)
    (tmp_path / env.CACHE_NAME).write_text(
        json.dumps({"env": {"INCLUDE": "C:/from-a-compiler-long-gone"}}),
        encoding="utf-8")

    assert env.load_build_env(tmp_path)["INCLUDE"] == "C:/from-a-compiler-long-gone"


def test_an_unreadable_cache_is_rebuilt_rather_than_fatal(tmp_path, monkeypatch):
    """Half a JSON file costs a vcvars run, not the build."""
    monkeypatch.setattr(env, "IS_WINDOWS", True)
    monkeypatch.setattr(env, "_snapshot_windows", lambda: {"INCLUDE": "C:/fresh"})
    (tmp_path / env.CACHE_NAME).write_text("{ not json", encoding="utf-8")

    assert env.load_build_env(tmp_path)["INCLUDE"] == "C:/fresh"


def test_a_fresh_snapshot_is_written_to_the_cache(tmp_path, monkeypatch):
    """What was learned is kept, so the next command does not learn it again."""
    monkeypatch.setattr(env, "IS_WINDOWS", True)
    monkeypatch.setattr(env, "_snapshot_windows", lambda: {"LIB": "C:/libs"})

    env.load_build_env(tmp_path)

    cached = json.loads((tmp_path / env.CACHE_NAME).read_text(encoding="utf-8"))
    assert cached["env"] == {"LIB": "C:/libs"}


def test_no_snapshot_means_the_environment_is_left_alone(tmp_path, monkeypatch):
    """No Visual Studio found: the module warns elsewhere and changes nothing."""
    monkeypatch.setattr(env, "IS_WINDOWS", True)
    monkeypatch.setattr(env, "_snapshot_windows", lambda: None)

    assert env.load_build_env(tmp_path) == dict(os.environ)
    assert not (tmp_path / env.CACHE_NAME).exists()


def test_the_snapshot_reads_the_key_value_lines_set_prints(monkeypatch, recorder):
    """`cmd /c call vcvars && set` answers with KEY=VALUE lines."""
    recorder(env)
    monkeypatch.setattr(env, "IS_WINDOWS", True)
    monkeypatch.setattr(env, "find_vcvars", lambda: "C:/vs/vcvars64.bat")
    monkeypatch.delenv("VSINSTALLDIR", raising=False)
    monkeypatch.setattr(env.proc, "capture",
                        lambda cmd, **kw: (0, "INCLUDE=C:/inc\nLIB=C:/lib\nbroken\n", ""))

    snapshot = env._snapshot_windows()

    assert snapshot == {"INCLUDE": "C:/inc", "LIB": "C:/lib"}


def test_a_developer_shell_is_not_snapshotted_again(monkeypatch, recorder):
    """Already inside a VS shell: running vcvars would only add to itself."""
    recorder(env)
    monkeypatch.setattr(env, "IS_WINDOWS", True)
    monkeypatch.setenv("VSINSTALLDIR", "C:/vs")

    assert env._snapshot_windows() is None


def test_a_failing_vcvars_is_reported_rather_than_believed(monkeypatch, recorder):
    """A non-zero vcvars produced no environment worth caching."""
    spy = recorder(env)
    monkeypatch.setattr(env, "IS_WINDOWS", True)
    monkeypatch.setattr(env, "find_vcvars", lambda: "C:/vs/vcvars64.bat")
    monkeypatch.delenv("VSINSTALLDIR", raising=False)
    monkeypatch.setattr(env.proc, "capture", lambda cmd, **kw: (1, "", "boom"))

    assert env._snapshot_windows() is None
    assert spy.said("non-zero")
