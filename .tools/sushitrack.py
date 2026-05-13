"""
@file      sushitrack.py
@brief     Command-line interface for the SushiTrack project pipeline.
@author    Mustafa Garip (mustafagarip@sushisystems.io)
@date      12.05.2026
@version   v2.0.0
@copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.

Command reference
-----------------
Build
  sushitrack project build               -- build library only
  sushitrack project build --unit        -- build library + unit_test
  sushitrack project build --regression  -- build library + regression_test + tracker_bridge
  sushitrack project build --clean       -- wipe build dir first
  sushitrack project build --addsanitizer

Test
  sushitrack project test                -- run all: unit → label (regression) in sequence
  sushitrack project test unit           -- run unit_test (GTest deterministic)
  sushitrack project test label          -- run regression_test on det.txt datasets + TrackEval
  sushitrack project test inference      -- run YOLOX video inference pipeline

Clean / Docker
  sushitrack project clean
  sushitrack docker build
  sushitrack docker run
"""

import os
import sys
import subprocess
import argparse
import platform
from pathlib import Path

from loguru import logger

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
_TOOLS_DIR    = Path(__file__).parent
_PROJECT_ROOT = _TOOLS_DIR.parent

sys.path.insert(0, str(_TOOLS_DIR))
from paths_manager import DIR_EVALUATOR, DIR_INFERENCE, DIR_DATA, DIR_BUILD, DIR_3RDPARTY

_EVAL_DIR     = DIR_EVALUATOR
_INFERENCE_DIR = DIR_INFERENCE

sys.path.insert(0, str(_EVAL_DIR))

try:
    from trackeval_runner import TrackEvalRunner
    from report_generator import print_trackeval_table, export_trackeval_results
    _EVAL_AVAILABLE = True
except ImportError:
    _EVAL_AVAILABLE = False

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------
_TRACKER_MAP = {
    "sushitrack": "SushiTrack",
    "bytetrack": "ByteTrack",
    "ocsort":    "OCSORT",
}
_KNOWN_TRACKERS = list(_TRACKER_MAP.keys())

_IS_WINDOWS = platform.system() == "Windows"


# ===========================================================================
# Path helpers
# ===========================================================================

def get_project_root() -> Path:
    """@brief Returns absolute path to project root."""
    return _PROJECT_ROOT


def get_data_root() -> Path:
    """@brief Returns path to the MOT data directory."""
    eval_data = DIR_EVALUATOR / "data"
    if eval_data.exists():
        return eval_data
    return DIR_DATA



# ===========================================================================
# Build
# ===========================================================================

def project_build(clean: bool, unit: bool, regression: bool, asan: bool) -> bool:
    """
    @brief Invokes the platform build script with the requested options.

    @param clean      Wipe the build directory before configuring.
    @param unit       Build the unit_test target.
    @param regression Build the regression_test + tracker_bridge targets.
    @param asan       Enable AddressSanitizer.
    @return True on success.
    """
    script = _TOOLS_DIR / "project" / ("build.bat" if _IS_WINDOWS else "build.sh")
    args: list[str] = []
    if clean:      args.append("--clean")
    if unit:       args.append("--unit")
    if regression: args.append("--regression")
    if asan:       args.append("--addsanitizer")

    mode_parts = []
    if unit:       mode_parts.append("unit")
    if regression: mode_parts.append("regression")
    if not mode_parts: mode_parts.append("library-only")
    logger.info(f"Building: {', '.join(mode_parts)}{' (clean)' if clean else ''}")

    try:
        subprocess.run([str(script)] + args, check=True, shell=_IS_WINDOWS)
        return True
    except subprocess.CalledProcessError as e:
        logger.error(f"Build failed (exit {e.returncode})")
        return False


def project_clean() -> bool:
    """@brief Runs the clean script."""
    script = _TOOLS_DIR / "project" / ("clean.bat" if _IS_WINDOWS else "clean.sh")
    try:
        subprocess.run([str(script)], check=True, shell=_IS_WINDOWS)
        return True
    except subprocess.CalledProcessError as e:
        logger.error(f"Clean failed (exit {e.returncode})")
        return False


# ===========================================================================
# Docker
# ===========================================================================

def docker_build() -> bool:
    """@brief Builds the Docker image."""
    script = _TOOLS_DIR / "docker" / ("build.bat" if _IS_WINDOWS else "build.sh")
    try:
        subprocess.run([str(script)], check=True, shell=_IS_WINDOWS)
        return True
    except subprocess.CalledProcessError:
        return False


def docker_run() -> bool:
    """@brief Runs the Docker container."""
    script = _TOOLS_DIR / "docker" / ("run.bat" if _IS_WINDOWS else "run.sh")
    try:
        subprocess.run([str(script)], check=True, shell=_IS_WINDOWS)
        return True
    except subprocess.CalledProcessError:
        return False


# ===========================================================================
# Binary discovery
# ===========================================================================

def _find_binary(name: str) -> Path | None:
    """
    @brief Searches standard build output locations for a compiled binary.

    @param name  Executable base name (without extension on Linux).
    @return Path to the found binary, or None.
    """
    root     = get_project_root()
    exe_name = f"{name}.exe" if _IS_WINDOWS else name

    candidates = [
        DIR_BUILD / "bin" / "Release" / exe_name,
        DIR_BUILD / "bin" / exe_name,
        DIR_BUILD / "bin" / "Debug" / exe_name,
        DIR_BUILD / "Release" / exe_name,
        DIR_BUILD / exe_name,
        DIR_BUILD / "Debug" / exe_name,
    ]
    for c in candidates:
        if c.exists():
            return c

    found = list(root.rglob(exe_name))
    return found[0] if found else None


# ===========================================================================
# unit test
# ===========================================================================

def project_test_unit(gtest_filter: str | None, verbose: bool) -> bool:
    """
    @brief Runs the unit_test GTest binary.

    @param gtest_filter  Optional --gtest_filter string (e.g. "TrackFSMTest.*").
    @param verbose       Forward --gtest_print_time=1 and colour output.
    @return True if all tests pass.
    """
    binary = _find_binary("unit_test")
    if not binary:
        logger.error(
            "unit_test binary not found. "
            "Build first:  sushitrack project build --unit"
        )
        return False

    cmd = [str(binary)]
    if gtest_filter:
        cmd.append(f"--gtest_filter={gtest_filter}")
    if verbose:
        cmd.append("--gtest_print_time=1")

    logger.info(f"Running unit tests: {binary.name}")
    try:
        result = subprocess.run(cmd)
        return result.returncode == 0
    except Exception as e:
        logger.error(f"unit_test execution failed: {e}")
        return False


# ===========================================================================
# label (regression) test
# ===========================================================================

def _build_regression_filter(scenario: str | None, compare: bool, tracker: str) -> str:
    """
    @brief Constructs a GTest filter for the regression_test suite.

    @param scenario  Sequence name (e.g. "MOT17-02-DPM") or None for all.
    @param compare   Run all three trackers side-by-side.
    @param tracker   Single tracker name (lowercase).
    @return GTest --gtest_filter string.
    """
    trackers = _KNOWN_TRACKERS if compare else [tracker.lower()]
    sc = scenario.replace('-', '_') if scenario else None

    parts = []
    for t in trackers:
        parts.append(f"*{sc}_{t}*" if sc else f"*_{t}*")
    return ":".join(parts)


def _run_regression_binary(binary: Path, gtest_filter: str) -> tuple[bool, dict]:
    """
    @brief Executes the regression_test binary and parses PERF lines.

    @param binary        Path to the regression_test executable.
    @param gtest_filter  GTest filter string.
    @return (success, perf_data) where perf_data maps tracker→{seq→us/frame}.
    """
    try:
        result = subprocess.run(
            [str(binary), f"--gtest_filter={gtest_filter}"],
            capture_output=True, text=True,
        )
        if result.returncode != 0:
            logger.error(f"regression_test failed (exit {result.returncode})")
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
                perf_data.setdefault(tracker, {})[sc_name.strip()] = timing
            except (ValueError, IndexError):
                continue

        return True, perf_data
    except Exception as e:
        logger.error(f"regression_test execution error: {e}")
        return False, {}


def project_test_label(
    scenario:   str | None,
    compare:    bool,
    eval_only:  bool,
    export:     str | None,
    trackeval:  bool,
    tracker:    str,
) -> bool:
    """
    @brief Runs label-based (det.txt) regression inference and TrackEval metrics.

    @param scenario   Sequence name filter, or None for all available sequences.
    @param compare    Evaluate all three trackers in one pass.
    @param eval_only  Skip binary execution; only re-run TrackEval on existing pred files.
    @param export     Directory to write CSV/JSON exports to.
    @param trackeval  Use the official TrackEval script instead of the internal runner.
    @param tracker    Target tracker (lowercase key).
    @return True on success.
    """
    perf_data: dict = {t: {} for t in (_KNOWN_TRACKERS if compare else [tracker])}

    if not eval_only:
        binary = _find_binary("regression_test")
        if not binary:
            logger.error(
                "regression_test binary not found. "
                "Build first:  sushitrack project build --regression"
            )
            return False

        gtest_filter = _build_regression_filter(scenario, compare, tracker)
        logger.info(f"Running regression inference: {binary.name}")
        ok, perf_data = _run_regression_binary(binary, gtest_filter)
        if not ok:
            return False

    if trackeval:
        logger.info("Running TrackEval via official script ...")
        return _run_trackeval_official(scenario, compare, tracker)

    logger.info("Running internal TrackEval evaluation ...")
    return _run_evaluation_internal(scenario, compare, perf_data, export, tracker)


# ===========================================================================
# inference (YOLOX video) test
# ===========================================================================

def project_test_inference(args: argparse.Namespace) -> bool:
    """
    @brief Launches the YOLOX video inference pipeline via demo.py.

    Requires tracker_bridge.dll / libtracker_bridge.so to be present in the
    build directory (build with --regression first).

    @param args  Parsed argparse namespace containing all YOLOX/video arguments.
    @return True on success.
    """
    demo_script = _INFERENCE_DIR / "demo.py"
    if not demo_script.exists():
        logger.error(f"demo.py not found at {demo_script}")
        return False

    if not args.source:
        logger.error("--source <video_path> is required for 'test inference'")
        return False

    # Normalise tracker name (demo.py expects registry-cased name)
    tracker_key = args.tracker.lower()
    if tracker_key not in _TRACKER_MAP:
        logger.error(
            f"Unknown tracker '{args.tracker}'. "
            f"Available: {', '.join(_TRACKER_MAP.values())}"
        )
        return False
    tracker_display = _TRACKER_MAP[tracker_key]

    cmd = [
        sys.executable, str(demo_script),
        "--source",      args.source,
        "--tracker",     tracker_display,
        "--name",        args.name,
        "--device",      args.device,
        "--conf",        str(args.conf),
        "--nms",         str(args.nms),
        "--num-classes", str(args.num_classes),
    ]

    if args.ckpt:
        cmd += ["--ckpt", args.ckpt]
    if args.tsize is not None:
        cmd += ["--tsize"] + [str(t) for t in args.tsize]
    if args.stress:
        cmd.append("--stress")
    if args.no_save:
        cmd.append("--no_save")
    if args.save_name:
        cmd += ["--save_name", args.save_name]
    if args.legacy:
        cmd.append("--legacy")
    if args.mot_label:
        cmd.append("--mot-label")
    if args.no_screen:
        cmd.append("--no-screen")

    logger.info(f"Starting YOLOX inference pipeline — tracker: {tracker_display}")
    logger.info(f"Source: {args.source}")
    try:
        subprocess.run(cmd, check=True)
        return True
    except subprocess.CalledProcessError as e:
        logger.error(f"Inference pipeline failed (exit {e.returncode})")
        return False


# ===========================================================================
# TrackEval wrappers (unchanged logic, moved here for clarity)
# ===========================================================================

def _run_trackeval_official(scenario: str | None, compare: bool, tracker: str) -> bool:
    """
    @brief Copies pred/GT files into the TrackEval directory tree and runs the
           official run_mot_challenge.py script.

    @param scenario  Sequence filter.
    @param compare   Evaluate all trackers.
    @param tracker   Single tracker key.
    @return True on success.
    """
    import shutil
    data_root    = get_data_root()
    trackeval_dir = DIR_3RDPARTY / "TrackEval"

    if not trackeval_dir.exists():
        logger.error("TrackEval submodule not found.")
        return False

    trackers = _KNOWN_TRACKERS if compare else [tracker]

    # Clear old data to prevent stale results and seqmaps
    trackeval_data_dir = trackeval_dir / "data"
    if trackeval_data_dir.exists():
        shutil.rmtree(trackeval_data_dir, ignore_errors=True)

    for tr in trackers:
        for seq_dir in data_root.iterdir():
            if not seq_dir.is_dir() or not (seq_dir / "gt" / "gt.txt").exists():
                continue
            seq = seq_dir.name
            if scenario and seq != scenario:
                continue
            src_pred = seq_dir / "pred" / tr / f"{seq}.txt"
            if not src_pred.exists():
                continue
            benchmark = "MOT20" if seq.upper().startswith("MOT20") else "MOT17"
            dst_dir = trackeval_dir / "data" / "trackers" / "mot_challenge" / f"{benchmark}-train" / tr / "data"
            dst_dir.mkdir(parents=True, exist_ok=True)
            shutil.copy2(src_pred, dst_dir / f"{seq}.txt")
            gt_dst = trackeval_dir / "data" / "gt" / "mot_challenge" / f"{benchmark}-train" / seq
            if not (gt_dst / "gt" / "gt.txt").exists():
                (gt_dst / "gt").mkdir(parents=True, exist_ok=True)
                src_gt = seq_dir / "gt" / "gt.txt"
                if src_gt.exists():
                    shutil.copy2(src_gt, gt_dst / "gt" / "gt.txt")
                src_info = seq_dir / "seqinfo.ini"
                if src_info.exists():
                    shutil.copy2(src_info, gt_dst / "seqinfo.ini")

    script_path = trackeval_dir / "scripts" / "run_mot_challenge.py"
    if not script_path.exists():
        logger.error(f"Official TrackEval script not found at {script_path}")
        return False

    success = True
    for benchmark in ["MOT17", "MOT20"]:
        valid_trackers, all_seqs = [], set()
        for tr in trackers:
            tdata = trackeval_dir / "data" / "trackers" / "mot_challenge" / f"{benchmark}-train" / tr / "data"
            if tdata.exists():
                seqs = [f.stem for f in tdata.iterdir() if f.suffix == ".txt"]
                if seqs:
                    valid_trackers.append(tr)
                    all_seqs.update(seqs)
        if not valid_trackers:
            continue
        seqmap_dir = trackeval_dir / "data" / "gt" / "mot_challenge" / "seqmaps"
        seqmap_dir.mkdir(parents=True, exist_ok=True)
        seqmap_path = seqmap_dir / f"{benchmark}-train.txt"
        with open(seqmap_path, "w") as f:
            f.write("name\n")
            for s in sorted(all_seqs):
                f.write(f"{s}\n")
        cmd = [sys.executable, str(script_path),
               "--BENCHMARK", benchmark,
               "--TRACKERS_TO_EVAL"] + valid_trackers + ["--SPLIT_TO_EVAL", "train"]
        if scenario:
            cmd += ["--SEQ_INFO", scenario]
        try:
            r = subprocess.run(cmd, cwd=str(trackeval_dir))
            if r.returncode != 0:
                success = False
        except Exception as e:
            logger.error(f"TrackEval script error: {e}")
            success = False

    return success


def _run_evaluation_internal(
    scenario:   str | None,
    compare:    bool,
    perf_data:  dict,
    export_dir: str | None,
    tracker:    str,
) -> bool:
    """
    @brief Runs the internal Python TrackEval wrapper.

    @param scenario   Sequence filter.
    @param compare    Evaluate all trackers.
    @param perf_data  FPS data from the regression binary run.
    @param export_dir Optional export directory.
    @param tracker    Single tracker key.
    @return True on success.
    """
    if not _EVAL_AVAILABLE:
        logger.error(
            "Internal evaluator unavailable. Install TrackEval:\n"
            "  pip install git+https://github.com/JonathonLuiten/TrackEval.git"
        )
        return False

    data_root = get_data_root()
    if not data_root.exists():
        logger.error(f"Data directory not found: {data_root}")
        return False

    trackers  = _KNOWN_TRACKERS if compare else [tracker]
    scenarios = [scenario] if scenario else None

    try:
        runner  = TrackEvalRunner(data_root=str(data_root))
        results = runner.evaluate(
            trackers=trackers,
            scenarios=scenarios,
            perf_fps={
                tr: {sc: 1_000_000.0 / us for sc, us in sc_map.items()}
                for tr, sc_map in perf_data.items() if sc_map
            },
        )
        if not results:
            logger.error("No evaluation results. Verify that pred/*.txt files exist.")
            return False
        print_trackeval_table(results, compare=compare)
        if export_dir:
            export_trackeval_results(results, export_dir)
        return True
    except Exception as e:
        logger.error(f"Evaluation failed: {e}")
        return False


# ===========================================================================
# Argument parser
# ===========================================================================

def _build_parser() -> argparse.ArgumentParser:
    """@brief Constructs the full CLI argument parser."""
    parser = argparse.ArgumentParser(
        prog="sushitrack",
        description="SushiTrack Pipeline Tool",
    )
    sub = parser.add_subparsers(dest="command")

    # ------------------------------------------------------------------
    # project
    # ------------------------------------------------------------------
    proj = sub.add_parser("project", help="Project actions (build / test / clean)")
    proj_sub = proj.add_subparsers(dest="action")

    # project build
    p_build = proj_sub.add_parser("build", help="Compile the project")
    p_build.add_argument("--clean",        action="store_true", help="Wipe build dir first")
    p_build.add_argument("--unit",         action="store_true", help="Include unit_test target")
    p_build.add_argument("--regression",   action="store_true", help="Include regression_test + tracker_bridge")
    p_build.add_argument("--test",         action="store_true", help="Alias for --regression (deprecated)")
    p_build.add_argument("--addsanitizer", action="store_true", help="Enable AddressSanitizer")

    # project test
    p_test = proj_sub.add_parser("test", help="Run tests")
    p_test.add_argument(
        "test_type",
        nargs="?",
        choices=["all", "unit", "label", "inference"],
        default="all",
        help=(
            "(default) all — unit tests then label regression in sequence\n"
            "unit           — GTest deterministic unit tests (no external data)\n"
            "label          — regression_test on det.txt datasets + TrackEval metrics\n"
            "inference      — YOLOX video inference pipeline via tracker_bridge"
        ),
    )

    # shared test options
    p_test.add_argument("--filter",     type=str,  default=None, help="GTest --gtest_filter (unit mode)")
    p_test.add_argument("--verbose",    action="store_true",     help="Verbose GTest output (unit mode)")
    p_test.add_argument("--scenario",   type=str,  default=None, help="Sequence name filter (label mode)")
    p_test.add_argument("--compare",    action="store_true",     help="Run all three trackers (label mode)")
    p_test.add_argument("--eval-only",  action="store_true",     help="Skip binary; re-run TrackEval only (label mode)")
    p_test.add_argument("--trackeval",  action="store_true",     help="Use official TrackEval script (label mode)")
    p_test.add_argument("--export",     type=str,  default=None, help="Export results directory (label mode)")
    p_test.add_argument("--addsanitizer", action="store_true",   help="Rebuild with ASAN before running (label mode)")

    # YOLOX / video options (inference mode)
    p_test.add_argument("--source",      type=str,               help="Video path or camera index (inference)")
    p_test.add_argument("--tracker",     type=str, default="SushiTrack", help="Tracker name (inference / label)")
    p_test.add_argument("-n", "--name",  type=str, default="yolox-s",   help="YOLOX model name")
    p_test.add_argument("-c", "--ckpt",  type=str, default=None,        help="YOLOX checkpoint path")
    p_test.add_argument("--device",      type=str, default="gpu",       help="cpu or gpu")
    p_test.add_argument("--conf",        type=float, default=0.1,       help="Detection confidence threshold")
    p_test.add_argument("--nms",         type=float, default=0.45,      help="NMS threshold")
    p_test.add_argument("--tsize",       type=int, nargs="+", default=None, help="Inference resolution [H W] or [S]")
    p_test.add_argument("--num-classes", type=int, default=80,          help="YOLOX class count")
    p_test.add_argument("--legacy",      action="store_true",           help="Legacy YOLOX normalisation")
    p_test.add_argument("--stress",      action="store_true",           help="Headless maximum throughput mode")
    p_test.add_argument("--no_save",     action="store_true",           help="Do not save output video")
    p_test.add_argument("--save_name",   type=str, default=None,        help="Output video filename")
    p_test.add_argument("--mot-label",   action="store_true",           help="Write detections to output/det.txt (inference)")
    p_test.add_argument("--no-screen",   action="store_true",           help="Disable UI display (inference)")

    # project clean
    proj_sub.add_parser("clean", help="Remove build artefacts")

    # ------------------------------------------------------------------
    # docker
    # ------------------------------------------------------------------
    dock = sub.add_parser("docker", help="Docker actions")
    dock_sub = dock.add_subparsers(dest="action")
    dock_sub.add_parser("build", help="Build Docker image")
    dock_sub.add_parser("run",   help="Run Docker container")

    return parser


# ===========================================================================
# Entry point
# ===========================================================================

def main():
    """@brief CLI dispatcher."""
    parser = _build_parser()
    args   = parser.parse_args()

    if args.command == "project":
        success = False

        if args.action == "build":
            # --test builds both unit and regression targets
            unit = args.unit or args.test
            regression = args.regression or args.test
            success = project_build(
                clean      = args.clean,
                unit       = unit,
                regression = regression,
                asan       = args.addsanitizer,
            )

        elif args.action == "clean":
            success = project_clean()

        elif args.action == "test":
            test_type = args.test_type or "all"

            if test_type == "all":
                # Sequential pipeline: unit → label (regression).
                # Fail-fast: label stage is skipped if unit stage fails.
                logger.info("[Pipeline] Stage 1/2 — unit tests")
                unit_ok = project_test_unit(
                    gtest_filter = args.filter,
                    verbose      = args.verbose,
                )
                if not unit_ok:
                    logger.error("[Pipeline] Unit tests failed — skipping label regression.")
                    success = False
                else:
                    logger.info("[Pipeline] Stage 2/2 — label regression")
                    tracker_key = args.tracker.lower()
                    if tracker_key not in _TRACKER_MAP:
                        logger.error(
                            f"Unknown tracker '{args.tracker}'. "
                            f"Available: {', '.join(_TRACKER_MAP.values())}"
                        )
                        sys.exit(1)
                    success = project_test_label(
                        scenario  = args.scenario,
                        compare   = args.compare,
                        eval_only = args.eval_only,
                        export    = args.export,
                        trackeval = args.trackeval,
                        tracker   = tracker_key,
                    )

            elif test_type == "unit":
                success = project_test_unit(
                    gtest_filter = args.filter,
                    verbose      = args.verbose,
                )

            elif test_type == "label":
                if args.addsanitizer:
                    logger.info("ASAN requested — rebuilding with sanitizer ...")
                    project_build(clean=False, unit=False, regression=True, asan=True)

                tracker_key = args.tracker.lower()
                if tracker_key not in _TRACKER_MAP:
                    logger.error(
                        f"Unknown tracker '{args.tracker}'. "
                        f"Available: {', '.join(_TRACKER_MAP.values())}"
                    )
                    sys.exit(1)

                success = project_test_label(
                    scenario  = args.scenario,
                    compare   = args.compare,
                    eval_only = args.eval_only,
                    export    = args.export,
                    trackeval = args.trackeval,
                    tracker   = tracker_key,
                )

            elif test_type == "inference":
                success = project_test_inference(args)

        else:
            parser.print_help()
            sys.exit(0)

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
