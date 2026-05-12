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

from loguru import logger

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
    if (root / "test" / "data").exists():
        return root / "test" / "data"
    return root / "data"


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
    # For build scripts, they probably need to know if --addsanitizer is used. 
    # But currently they don't accept it. We can pass it if we modify the bat/sh scripts later.
    # Actually, the task says: Update build.bat/build.sh to accept --addsanitizer.
    # So we'll pass it from os.environ or add an arg. Let's just pass an env var for now or we will modify build scripts.
    if os.environ.get("USE_ASAN") == "1":
        args.append("--addsanitizer")

    mode = "test" if test else "normal"
    logger.info(f"Building project: {mode}, {'clean' if clean else 'skip clean'}")
    try:
        subprocess.run([str(script)] + args, check=True, shell=(platform.system() == "Windows"))
        return True
    except subprocess.CalledProcessError as e:
        logger.error(f"Build failed with exit code {e.returncode}")
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
        logger.error(f"Clean failed with exit code {e.returncode}")
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


def _get_gtest_filter(scenario: str | None, compare: bool, tracker: str = "sushitrack") -> str:
    """
    @brief Constructs GTest filter string.
    @param scenario Target sequence identifier.
    @param compare  Flag for comparative testing.
    @param tracker  Target tracker identifier.
    @return Filter string.
    """
    trackers = _KNOWN_TRACKERS if compare else [tracker.lower()]
    sc       = scenario.replace('-', '_') if scenario else None

    parts = []
    for t in trackers:
        parts.append(f"*{sc}_{t}*" if sc else f"*_{t}*")
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
            logger.error(f"Binary failed with exit code {result.returncode}")
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
        logger.error(f"Binary execution failed: {e}")
        return False, {}

def project_test_videos(args) -> bool:
    script = _TOOLS_DIR / "project" / "test" / "demo.py"
    cmd = [sys.executable, str(script)]
    
    cmd.extend(["--source", args.source])
    cmd.extend(["--tracker", args.tracker])
    cmd.extend(["--name", args.name])
    cmd.extend(["--device", args.device])
    cmd.extend(["--conf", str(args.conf)])
    cmd.extend(["--nms", str(args.nms)])
    cmd.extend(["--num_classes", str(args.num_classes)])
    
    if args.ckpt:
        cmd.extend(["--ckpt", args.ckpt])
    
    if args.tsize is not None:
        cmd.extend(["--tsize"] + [str(t) for t in args.tsize])
        
    if args.stress:
        cmd.append("--stress")
    if args.no_save:
        cmd.append("--no_save")
    if args.save_name:
        cmd.extend(["--save_name", args.save_name])
    if args.legacy:
        cmd.append("--legacy")
        
    logger.info(f"Running video test pipeline: {' '.join(cmd)}")
    try:
        subprocess.run(cmd, check=True)
        return True
    except subprocess.CalledProcessError as e:
        logger.error(f"Video test failed with exit code {e.returncode}")
        return False


def project_test(scenario: str | None, compare: bool, eval_only: bool, export: str | None, trackeval: bool, tracker: str = "sushitrack") -> bool:
    """
    @brief Orchestrates project testing and evaluation.
    @param scenario  Target sequence identifier.
    @param compare   Flag for comparative evaluation.
    @param eval_only Flag to skip binary execution.
    @param export    Optional export directory path.
    @param trackeval Flag to use official TrackEval script directly.
    @param tracker   Target tracker identifier.
    @return Execution status.
    """
    perf_data: dict = {t: {} for t in (_KNOWN_TRACKERS if compare else [tracker.lower()])}

    if not eval_only:
        binary = _get_test_binary()
        if not binary:
            logger.error("Test binary not found. Build with 'sushitrack project build --test' first.")
            return False

        logger.info(f"Running tracking binary: {binary.name}")
        success, perf_data = _run_binary(str(binary), _get_gtest_filter(scenario, compare, tracker))
        if not success:
            return False

    if trackeval:
        logger.info("Running TrackEval via official script...")
        return run_trackeval_official(scenario, compare, tracker)

    logger.info("Running internal TrackEval evaluation...")
    return run_evaluation(scenario, compare, perf_data, export, tracker)

def run_trackeval_official(scenario: str | None, compare: bool, tracker: str = "sushitrack") -> bool:
    import shutil
    data_root = get_data_root()
    trackeval_dir = get_project_root() / "3rdparty" / "TrackEval"
    
    if not trackeval_dir.exists():
        logger.error("TrackEval directory not found in project root.")
        return False
        
    trackers = _KNOWN_TRACKERS if compare else [tracker.lower()]
    
    # 1. Prepare directory structure
    for tracker in trackers:
        for seq_dir in data_root.iterdir():
            if not seq_dir.is_dir() or not (seq_dir / "gt" / "gt.txt").exists():
                continue
                
            seq = seq_dir.name
            if scenario and seq != scenario:
                continue
                
            src_pred = seq_dir / "pred" / tracker / f"{seq}.txt"
            if not src_pred.exists():
                continue
                
            benchmark = "MOT20" if seq.upper().startswith("MOT20") else "MOT17"
            dst_dir = trackeval_dir / "data" / "trackers" / "mot_challenge" / f"{benchmark}-train" / tracker / "data"
            dst_dir.mkdir(parents=True, exist_ok=True)
            
            shutil.copy2(src_pred, dst_dir / f"{seq}.txt")
            logger.info(f"Copied {seq} for {tracker} to TrackEval hierarchy")
            
            # 2. Copy GT and seqinfo to TrackEval (only once per sequence)
            gt_dst_dir = trackeval_dir / "data" / "gt" / "mot_challenge" / f"{benchmark}-train" / seq
            if not (gt_dst_dir / "gt" / "gt.txt").exists():
                (gt_dst_dir / "gt").mkdir(parents=True, exist_ok=True)
                src_gt = seq_dir / "gt" / "gt.txt"
                if src_gt.exists():
                    shutil.copy2(src_gt, gt_dst_dir / "gt" / "gt.txt")
                    
                src_info = seq_dir / "seqinfo.ini"
                if src_info.exists():
                    shutil.copy2(src_info, gt_dst_dir / "seqinfo.ini")
                
                logger.info(f"Copied GT/info for {seq} to TrackEval hierarchy")
            
    # 2. Run the official script for each benchmark (grouping trackers for comparison)
    script_path = trackeval_dir / "scripts" / "run_mot_challenge.py"
    if not script_path.exists():
        logger.error(f"Official TrackEval script not found at {script_path}")
        return False
        
    success = True
    for benchmark in ["MOT17", "MOT20"]:
        # Identify which trackers actually have data for this benchmark
        valid_trackers = []
        all_seqs = set()
        for tracker in trackers:
            tracker_data_dir = trackeval_dir / "data" / "trackers" / "mot_challenge" / f"{benchmark}-train" / tracker / "data"
            if tracker_data_dir.exists():
                seqs = [f.stem for f in tracker_data_dir.iterdir() if f.is_file() and f.suffix == ".txt"]
                if seqs:
                    valid_trackers.append(tracker)
                    all_seqs.update(seqs)
        
        if not valid_trackers:
            continue

        # Generate/Update seqmap for this benchmark based on found sequences
        seqmap_dir = trackeval_dir / "data" / "gt" / "mot_challenge" / "seqmaps"
        seqmap_dir.mkdir(parents=True, exist_ok=True)
        seqmap_path = seqmap_dir / f"{benchmark}-train.txt"
        with open(seqmap_path, "w") as f:
            f.write("name\n")
            for s in sorted(all_seqs):
                f.write(f"{s}\n")
        logger.info(f"Generated seqmap: {seqmap_path.name} with {len(all_seqs)} sequences")

        # Run official script with multiple trackers
        cmd = [
            sys.executable,
            str(script_path),
            "--BENCHMARK", benchmark,
            "--TRACKERS_TO_EVAL"
        ] + valid_trackers + [
            "--SPLIT_TO_EVAL", "train"
        ]
        
        logger.info(f"Executing: {' '.join(cmd)}")
        try:
            # Note: We run with check=False because TrackEval might return non-zero if some classes fail
            result = subprocess.run(cmd, cwd=str(trackeval_dir))
            if result.returncode != 0:
                success = False
        except Exception as e:
            logger.error(f"Failed to run TrackEval script: {e}")
            success = False
                
    return success


def run_evaluation(
    scenario:   str | None,
    compare:    bool,
    perf_data:  dict,
    export_dir: str | None = None,
    tracker:    str = "sushitrack",
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
        logger.error("Evaluator not available. Install TrackEval:\n"
                  "  pip install git+https://github.com/JonathonLuiten/TrackEval.git")
        return False

    data_root = get_data_root()
    if not data_root.exists():
        logger.error(f"Data directory not found: {data_root}")
        return False

    trackers  = _KNOWN_TRACKERS if compare else [tracker.lower()]
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
            logger.error("No evaluation results produced. Check that pred/*.txt files exist.")
            return False

        print_trackeval_table(results, compare=compare)

        if export_dir:
            export_trackeval_results(results, export_dir)

        return True
    except ImportError as e:
        if "trackeval" in str(e):
            logger.error("TrackEval library not found.\n"
                      "  Install: pip install git+https://github.com/JonathonLuiten/TrackEval.git")
        else:
            logger.error(f"Evaluation failed: {e}")
        return False
    except Exception as e:
        logger.error(f"Evaluation failed: {e}")
        return False


def main():
    """
    @brief CLI entry point for command dispatching.
    """
    parser = argparse.ArgumentParser(description="SushiTrack Pipeline Tool")
    subparsers = parser.add_subparsers(dest="command")

    proj = subparsers.add_parser("project")
    proj.add_argument("action",      choices=["build", "test", "clean"])
    proj.add_argument("test_type",   nargs="?", choices=["labels", "videos"], default="labels")
    proj.add_argument("--clean",     action="store_true")
    proj.add_argument("--test",      action="store_true")
    proj.add_argument("--scenario",  type=str)
    proj.add_argument("--compare",   action="store_true")
    proj.add_argument("--eval-only", action="store_true")
    proj.add_argument("--trackeval", action="store_true")
    proj.add_argument("--export",    type=str)
    
    # New arguments for testing (videos & yolox)
    proj.add_argument("--addsanitizer", action="store_true")
    proj.add_argument("--stress", action="store_true")
    proj.add_argument("--source", type=str)
    proj.add_argument("--tracker", type=str, default="SushiTrack")
    proj.add_argument("-n", "--name", type=str, default="yolox-s", help="YOLOX model architecture name")
    proj.add_argument("-c", "--ckpt", default=None, type=str, help="Path to YOLOX checkpoint weights")
    proj.add_argument("--device", default="gpu", type=str, help="Device to run inference (cpu or gpu)")
    proj.add_argument("--conf", default=0.1, type=float, help="Detection confidence threshold")
    proj.add_argument("--nms", default=0.45, type=float, help="NMS threshold")
    proj.add_argument("--tsize", default=None, nargs="+", type=int, help="Test image size (height, width) or single square value")
    proj.add_argument("--num_classes", default=80, type=int, help="Number of classes in the model")
    proj.add_argument("--legacy", action="store_true", help="Use legacy normalization (BGR2RGB, 0-1 scale, Mean/Std)")
    proj.add_argument("--no_save", action="store_true", help="Do not save the output video")
    proj.add_argument("--save_name", default=None, help="Explicit filename for saving output")

    dock = subparsers.add_parser("docker")
    dock.add_argument("action", choices=["build", "run"])

    args = parser.parse_args()

    if args.command == "project":
        success = False
        if args.addsanitizer:
            os.environ["USE_ASAN"] = "1"
            
        if args.action == "build":
            success = project_build(args.clean, args.test)
        elif args.action == "clean":
            success = project_clean()
        elif args.action == "test":
            if args.test_type == "labels":
                if args.addsanitizer:
                    # If ASAN is requested, we should probably rebuild the project first.
                    logger.info("ASAN requested. Ensuring project is built with ASAN...")
                    project_build(clean=False, test=True)
                
                # Normalize tracker name for registry (C++ is case sensitive)
                tracker_map = {t.lower(): t for t in ["ByteTrack", "SushiTrack", "OCSORT"]}
                normalized_tracker = tracker_map.get(args.tracker.lower(), args.tracker)
                args.tracker = normalized_tracker

                success = project_test(args.scenario, args.compare, args.eval_only, args.export, args.trackeval, args.tracker)
            elif args.test_type == "videos":
                if not args.source:
                    logger.error("--source is required for video testing")
                else:
                    # Normalize tracker name for video test registry as well
                    tracker_map = {t.lower(): t for t in ["ByteTrack", "SushiTrack", "OCSORT"]}
                    args.tracker = tracker_map.get(args.tracker.lower(), args.tracker)
                    success = project_test_videos(args)

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
