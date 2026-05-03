"""
@file      sushitrack.py
@brief     Command-line interface for the SushiTrack project pipeline.
@author    Mustafa Garip (mustafagarip@sushisystems.io)
@date      28.04.2026
@version   v1.0.0
@copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
"""

import os
import sys
import subprocess
import argparse
import platform
from pathlib import Path

from logger import log_info, log_warning, log_error, log_success

_TOOLS_DIR    = Path(__file__).parent
_PROJECT_ROOT = _TOOLS_DIR.parent
_EVAL_DIR     = _PROJECT_ROOT / "test" / "python"

sys.path.insert(0, str(_EVAL_DIR))

try:
    from trackeval_runner import TrackEvalRunner
    from report_generator import print_trackeval_table, export_trackeval_results
    _EVAL_AVAILABLE = True
except ImportError:
    _EVAL_AVAILABLE = False

_KNOWN_TRACKERS = ["sushitrack", "bytetrack", "ocsort"]


def get_project_root() -> Path:
    """
    @brief Returns absolute path to project root.
    @return Path object.
    """
    return _PROJECT_ROOT


def get_data_root() -> Path:
    """
    @brief Returns path to data directory.
    @return Path object.
    """
    root = get_project_root()
    if (root / "data").exists():
        return root / "data"
    return root / "test" / "data"


def project_build(clean: bool, test: bool) -> bool:
    """
    @brief Orchestrates project build process.
    @param clean Flag for clean build.
    @param test  Flag for test target inclusion.
    @return Execution status.
    """
    script = _TOOLS_DIR / "project" / ("build.bat" if platform.system() == "Windows" else "build.sh")
    args   = []
    if clean: args.append("--clean")
    if test:  args.append("--test")

    mode = "test" if test else "normal"
    log_info(f"Building project: {mode}, {'clean' if clean else 'skip clean'}")
    try:
        subprocess.run([str(script)] + args, check=True, shell=(platform.system() == "Windows"))
        return True
    except subprocess.CalledProcessError as e:
        log_error(f"Build failed with exit code {e.returncode}")
        return False


def project_clean() -> bool:
    """
    @brief Orchestrates project cleanup process.
    @return Execution status.
    """
    script = _TOOLS_DIR / "project" / ("clean.bat" if platform.system() == "Windows" else "clean.sh")
    try:
        subprocess.run([str(script)], check=True, shell=(platform.system() == "Windows"))
        return True
    except subprocess.CalledProcessError as e:
        log_error(f"Clean failed with exit code {e.returncode}")
        return False


def docker_build() -> bool:
    """
    @brief Orchestrates Docker image construction.
    @return Execution status.
    """
    script = _TOOLS_DIR / "docker" / ("build.bat" if platform.system() == "Windows" else "build.sh")
    try:
        subprocess.run([str(script)], check=True, shell=(platform.system() == "Windows"))
        return True
    except subprocess.CalledProcessError:
        return False


def docker_run() -> bool:
    """
    @brief Orchestrates Docker container execution.
    @return Execution status.
    """
    script = _TOOLS_DIR / "docker" / ("run.bat" if platform.system() == "Windows" else "run.sh")
    try:
        subprocess.run([str(script)], check=True, shell=(platform.system() == "Windows"))
        return True
    except subprocess.CalledProcessError:
        return False


def _get_test_binary() -> Path | None:
    """
    @brief Searches for compiled test executable.
    @return Path to binary or None.
    """
    root     = get_project_root()
    bin_name = "mot_test.exe" if platform.system() == "Windows" else "mot_test"

    candidates = [
        root / "build" / "Release" / bin_name,
        root / "build" / bin_name,
        root / "test" / "build" / "Release" / bin_name,
        root / "test" / "build" / bin_name,
    ]
    for c in candidates:
        if c.exists():
            return c

    found = list(root.rglob(bin_name))
    return found[0] if found else None


def _get_gtest_filter(scenario: str | None, compare: bool) -> str:
    """
    @brief Constructs GTest filter string.
    @param scenario Target sequence identifier.
    @param compare  Flag for comparative testing.
    @return Filter string.
    """
    trackers = _KNOWN_TRACKERS if compare else ["sushitrack"]
    sc       = scenario.replace('-', '_') if scenario else None

    parts = []
    for tracker in trackers:
        parts.append(f"*{sc}_{tracker}*" if sc else f"*_{tracker}*")
    return ":".join(parts)


def _run_binary(binary: str, gtest_filter: str) -> tuple[bool, dict]:
    """
    @brief Executes test binary and parses performance data.
    @param binary Path to executable.
    @param gtest_filter Target test filter.
    @return Tuple containing status and performance mapping.
    """
    try:
        result = subprocess.run(
            [binary, f"--gtest_filter={gtest_filter}"],
            capture_output=True,
            text=True,
        )
        if result.returncode != 0:
            log_error(f"Binary failed with exit code {result.returncode}")
            print(result.stdout)
            print(result.stderr)
            return False, {}

        perf_data: dict = {t: {} for t in _KNOWN_TRACKERS}
        for line in result.stdout.splitlines():
            if "[ PERF " not in line:
                continue
            try:
                tag_start = line.index("[ PERF ") + len("[ PERF ")
                tag_end   = line.index(" ]", tag_start)
                tracker   = line[tag_start:tag_end].lower()
                rest      = line[tag_end + 2:].strip()
                sc_name, timing_str = rest.split(":", 1)
                timing = float(timing_str.split("us/frame")[0].strip())
                if tracker not in perf_data:
                    perf_data[tracker] = {}
                perf_data[tracker][sc_name.strip()] = timing
            except (ValueError, IndexError):
                continue

        return True, perf_data
    except Exception as e:
        log_error(f"Binary execution failed: {e}")
        return False, {}


def project_test(scenario: str | None, compare: bool, eval_only: bool, export: str | None) -> bool:
    """
    @brief Orchestrates project testing and evaluation.
    @param scenario  Target sequence identifier.
    @param compare   Flag for comparative evaluation.
    @param eval_only Flag to skip binary execution.
    @param export    Optional export directory path.
    @return Execution status.
    """
    perf_data: dict = {t: {} for t in _KNOWN_TRACKERS}

    if not eval_only:
        binary = _get_test_binary()
        if not binary:
            log_error("Test binary not found. Build with 'sushitrack project build --test' first.")
            return False

        log_info(f"Running tracking binary: {binary.name}")
        success, perf_data = _run_binary(str(binary), _get_gtest_filter(scenario, compare))
        if not success:
            return False

    log_info("Running TrackEval evaluation...")
    return run_evaluation(scenario, compare, perf_data, export)


def run_evaluation(
    scenario:   str | None,
    compare:    bool,
    perf_data:  dict,
    export_dir: str | None = None,
) -> bool:
    """
    @brief Orchestrates TrackEval-based metric calculation.
    @param scenario   Target sequence identifier.
    @param compare    Flag for comparative evaluation.
    @param perf_data  Performance metrics mapping.
    @param export_dir Optional export directory path.
    @return Execution status.
    """
    if not _EVAL_AVAILABLE:
        log_error("Evaluator not available. Install TrackEval:\n"
                  "  pip install git+https://github.com/JonathonLuiten/TrackEval.git")
        return False

    data_root = get_data_root()
    if not data_root.exists():
        log_error(f"Data directory not found: {data_root}")
        return False

    trackers  = _KNOWN_TRACKERS if compare else ["sushitrack"]
    scenarios = [scenario] if scenario else None

    try:
        runner  = TrackEvalRunner(data_root=str(data_root))
        results = runner.evaluate(
            trackers=trackers,
            scenarios=scenarios,
            perf_fps={
                tr: {sc: 1_000_000.0 / us for sc, us in sc_map.items()}
                for tr, sc_map in perf_data.items()
                if sc_map
            },
        )

        if not results:
            log_error("No evaluation results produced. Check that pred/*.txt files exist.")
            return False

        print_trackeval_table(results, compare=compare)

        if export_dir:
            export_trackeval_results(results, export_dir)

        return True
    except ImportError as e:
        if "trackeval" in str(e):
            log_error("TrackEval library not found.\n"
                      "  Install: pip install git+https://github.com/JonathonLuiten/TrackEval.git")
        else:
            log_error(f"Evaluation failed: {e}")
        return False
    except Exception as e:
        log_error(f"Evaluation failed: {e}")
        return False


def main():
    """
    @brief CLI entry point for command dispatching.
    """
    parser = argparse.ArgumentParser(description="SushiTrack Pipeline Tool")
    subparsers = parser.add_subparsers(dest="command")

    proj = subparsers.add_parser("project")
    proj.add_argument("action",      choices=["build", "test", "clean"])
    proj.add_argument("--clean",     action="store_true")
    proj.add_argument("--test",      action="store_true")
    proj.add_argument("--scenario",  type=str)
    proj.add_argument("--compare",   action="store_true")
    proj.add_argument("--eval-only", action="store_true")
    proj.add_argument("--export",    type=str)

    dock = subparsers.add_parser("docker")
    dock.add_argument("action", choices=["build", "run"])

    args = parser.parse_args()

    if args.command == "project":
        success = False
        if args.action == "build":
            success = project_build(args.clean, args.test)
        elif args.action == "clean":
            success = project_clean()
        elif args.action == "test":
            success = project_test(args.scenario, args.compare, args.eval_only, args.export)

        if not success:
            sys.exit(1)

    elif args.command == "docker":
        success = False
        if args.action == "build":
            success = docker_build()
        elif args.action == "run":
            success = docker_run()

        if not success:
            sys.exit(1)

    else:
        parser.print_help()


if __name__ == "__main__":
    main()
