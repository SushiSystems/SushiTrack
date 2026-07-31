"""Subprocess helpers shared by every service.

All external work goes through here so command echoing, executable resolution
and the "tool is missing" message are identical everywhere, and so no service
has to care that a Windows PATH lookup needs help.
"""

import os
import shlex
import shutil
import subprocess

from . import console


def which(name, env=None):
    """Full path to *name*, searched in *env*'s PATH first, or None."""
    env_path = None
    if env:
        env_path = next((v for k, v in env.items() if k.upper() == "PATH"), None)
    return shutil.which(name, path=env_path) or shutil.which(name)


def _resolve(cmd, env):
    """Normalise a command for subprocess.

    A string is passed straight through — Windows accepts a raw command line and
    ``env.py`` needs one to keep the quoting around a vcvars path intact. A list
    gets argv[0] resolved to a full path so CreateProcess never does its own PATH
    lookup: on Windows an env dict can carry both "Path" and "PATH" and
    subprocess picks between them unpredictably.
    """
    if isinstance(cmd, str):
        return cmd
    resolved = [str(c) for c in cmd]
    resolved[0] = which(resolved[0], env) or resolved[0]
    return resolved


def format_command(cmd):
    if isinstance(cmd, str):
        return cmd
    parts = [str(c) for c in cmd]
    if os.name == "nt":
        return subprocess.list2cmdline(parts)
    return shlex.join(parts)


def run(cmd, cwd=None, env=None, echo=True):
    """Run *cmd*, streaming its output. Returns the exit code.

    A missing executable becomes exit code 127 with an actionable message rather
    than a traceback — it is the single most common failure (no cmake, no docker)
    and does not deserve a stack trace.
    """
    argv = _resolve(cmd, env)
    if echo:
        console.command(format_command(argv))
    try:
        return subprocess.run(argv, cwd=str(cwd) if cwd else None,
                              env=env).returncode
    except FileNotFoundError:
        console.error(f"Executable not found: {format_command(cmd)}. "
                      f"Install it or set its path in cli/config.local.json.")
        return 127
    except KeyboardInterrupt:
        console.warn("Interrupted.")
        return 130


def run_drained(cmd, cwd=None, env=None, echo=True):
    """Run *cmd* while actively draining its output, echoing it line by line.

    Unlike :func:`run` (which lets the child inherit our stdout), this pipes the
    child's output and reads it here. That matters for ``ctest`` on Windows: its
    gtest_discover_tests step spawns the test binary to enumerate cases, and when
    ctest's stdout is an inherited, slowly-drained pipe the discovery child
    intermittently stalls and registers nothing — surfacing as "No tests were
    found". Draining the pipe ourselves keeps enumeration reliable while every
    line is re-emitted, so the live output looks the same.
    """
    argv = _resolve(cmd, env)
    if echo:
        console.command(format_command(argv))
    try:
        child = subprocess.Popen(
            argv, cwd=str(cwd) if cwd else None, env=env,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            text=True, errors="replace", bufsize=1)
    except FileNotFoundError:
        console.error(f"Executable not found: {format_command(cmd)}. "
                      f"Install it or set its path in cli/config.local.json.")
        return 127
    try:
        for line in child.stdout:
            console.plain(line.rstrip("\n"))
        return child.wait()
    except KeyboardInterrupt:
        child.terminate()
        console.warn("Interrupted.")
        return 130


def capture(cmd, cwd=None, env=None, echo=False):
    """Run *cmd* capturing stdout/stderr. Returns (exit code, stdout, stderr)."""
    argv = _resolve(cmd, env)
    if echo:
        console.command(format_command(argv))
    try:
        result = subprocess.run(argv, cwd=str(cwd) if cwd else None, env=env,
                                capture_output=True, text=True,
                                errors="replace")
        return result.returncode, result.stdout, result.stderr
    except FileNotFoundError:
        return 127, "", f"Executable not found: {format_command(cmd)}"
    except KeyboardInterrupt:
        console.warn("Interrupted.")
        return 130, "", ""


def tool_version(name, args=("--version",), env=None):
    """First line of a tool's ``--version`` output, or None when unavailable."""
    if which(name, env) is None:
        return None
    code, out, err = capture([name, *args], env=env)
    if code != 0:
        return None
    text = (out or err).strip().splitlines()
    return text[0].strip() if text else None
