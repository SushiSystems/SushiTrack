"""What `proc` does today, pinned before it moves onto sushicore.

Every service in this CLI runs its external work through this module, so a
change in it reaches a user before it reaches anyone else. These tests say what
the behaviour is now; the migration is then judged by whether they still pass.

Nothing here runs a real tool. `sys.executable` is the program under test,
because it is the one executable a Python test can rely on.
"""

from __future__ import annotations

import os
import sys

from sushitrack_cli import proc


def test_which_prefers_the_path_the_caller_handed_over(tmp_path):
    """An env's PATH is searched before the process's own."""
    name = "st_probe.exe" if os.name == "nt" else "st_probe"
    planted = tmp_path / name
    planted.write_text("", encoding="utf-8")
    planted.chmod(0o755)

    found = proc.which(planted.stem, env={"PATH": str(tmp_path)})

    assert found is not None
    assert os.path.samefile(found, planted)


def test_which_answers_none_for_a_program_that_is_nowhere():
    """A missing tool is None rather than a guess at its name."""
    assert proc.which("definitely-not-a-program-4b7e") is None


def test_a_string_command_is_passed_through_untouched():
    """env.py hands a raw command line over to keep its vcvars quoting intact."""
    assert proc.format_command('"C:/a b/vcvars64.bat" && set') == \
        '"C:/a b/vcvars64.bat" && set'


def test_a_list_command_is_quoted_the_way_this_platform_quotes():
    """What is echoed is what a user could paste back into their shell."""
    formatted = proc.format_command(["cmake", "-S", "a b", "-B", "out"])

    assert "cmake" in formatted
    assert "a b" in formatted


def test_run_returns_the_child_exit_code(recorder):
    """The code the child chose, not a translation of it."""
    recorder(proc)

    assert proc.run([sys.executable, "-c", "raise SystemExit(7)"]) == 7


def test_run_echoes_the_command_unless_told_not_to(recorder):
    """A user can see what was run; a quiet call leaves no trace."""
    spy = recorder(proc)
    proc.run([sys.executable, "-c", "pass"])
    assert spy.lines

    quiet = recorder(proc)
    proc.run([sys.executable, "-c", "pass"], echo=False)
    assert quiet.lines == []


def test_a_missing_executable_is_reported_rather_than_raised(recorder):
    """The most common failure of all: no cmake, no docker."""
    spy = recorder(proc)

    code = proc.run(["definitely-not-a-program-4b7e"])

    assert code == 127
    assert spy.said("Executable not found")


def test_capture_returns_the_triple_and_stays_quiet_by_default(recorder):
    """A caller reading output does not want it echoed twice."""
    spy = recorder(proc)

    code, out, err = proc.capture([sys.executable, "-c", "print('hello')"])

    assert code == 0
    assert out.strip() == "hello"
    assert err == ""
    assert spy.lines == []


def test_capture_reports_a_missing_executable_in_its_stderr():
    """capture answers in its return value rather than through the console."""
    code, out, err = proc.capture(["definitely-not-a-program-4b7e"])

    assert code == 127
    assert out == ""
    assert "Executable not found" in err


def test_tool_version_reads_the_first_line():
    """One line is what a diagnostics table has room for."""
    version = proc.tool_version(sys.executable, args=("--version",))

    assert version is not None
    assert version.startswith("Python")


def test_tool_version_is_none_for_a_tool_that_is_not_there():
    """Absence is an answer, not an error."""
    assert proc.tool_version("definitely-not-a-program-4b7e") is None


def test_run_drained_returns_the_child_exit_code(recorder):
    """The drained path answers the same way the inherited one does."""
    recorder(proc)

    assert proc.run_drained([sys.executable, "-c", "raise SystemExit(3)"]) == 3


def test_run_drained_reports_a_missing_executable(recorder):
    """Both run paths say the same thing about the same failure."""
    spy = recorder(proc)

    code = proc.run_drained(["definitely-not-a-program-4b7e"])

    assert code == 127
    assert spy.said("Executable not found")
