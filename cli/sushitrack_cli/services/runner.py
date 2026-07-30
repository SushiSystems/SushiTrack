"""Run a built executable, by name or by picking one interactively."""

from .. import console, proc
from ..env import load_build_env
from . import discovery


def _select(paths):
    """List the build tree's executables and prompt for one.

    Falls back to an error rather than a prompt when stdin is not a terminal, so
    a scripted `ds run --select` fails loudly instead of hanging.
    """
    executables = discovery.list_executables(paths.build)
    if not executables:
        console.error("No executables found. Run `ds build` first.")
        return None

    table = console.new_table("#", "Name", "Path", title="Available executables")
    table.column_style(0, console.theme.cmd)
    table.column_style(2, console.theme.muted)
    for index, exe in enumerate(executables, 1):
        table.add_row(index, exe.name, exe.relative_to(paths.root))
    console.table(table)

    try:
        raw = input("Select a number: ").strip()
    except (EOFError, KeyboardInterrupt):
        console.warn("Nothing selected.")
        return None
    if not raw.isdigit() or not 1 <= int(raw) <= len(executables):
        console.error(f"Invalid selection '{raw}'.")
        return None
    return executables[int(raw) - 1]


def run(cfg, paths, target=None, select=False, forwarded=()):
    """Run *target* (exact name, then substring match). Returns its exit code."""
    if select or not target:
        binary = _select(paths)
        if binary is None:
            return 1
    else:
        binary = discovery.match_by_name(paths.build, target)
        if binary is None:
            console.error(f"No executable matching '{target}' under {paths.build}. "
                          f"Run `ds run` with no target to list what is available.")
            return 1

    console.header(f"Run {binary.name}")
    return proc.run([str(binary), *forwarded], cwd=paths.root,
                    env=load_build_env(paths.build))
