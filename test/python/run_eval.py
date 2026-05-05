"""
@file      run_eval.py
@brief     Command-line entry point for MOT Challenge evaluation.
@author    Mustafa Garip (mustafagarip@sushisystems.io)
@date      28.04.2026
@version   v1.0.0
@copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
"""

import sys
import argparse
from pathlib import Path

_THIS_DIR = Path(__file__).parent
sys.path.insert(0, str(_THIS_DIR))

_TOOLS_DIR = _THIS_DIR.parent.parent / ".tools"
if str(_TOOLS_DIR) not in sys.path:
    sys.path.insert(0, str(_TOOLS_DIR))

from logger import log_info, log_error, log_warning, log_success
from trackeval_runner import TrackEvalRunner


def parse_args():
    """
    @brief Parses command-line arguments.
    @return Argparse namespace object.
    """
    p = argparse.ArgumentParser(description="MOT Challenge compliant evaluator (TrackEval)")

    p.add_argument("--data-dir",  type=str, default=None)
    p.add_argument("--scenario",  type=str, default=None)
    p.add_argument("--trackers",  type=str, nargs="+",
                   default=["sushitrack", "bytetrack"])
    p.add_argument("--compare",   action="store_true")
    p.add_argument("--export",    type=str, default=None)
    p.add_argument("--split",     type=str, default="train")

    p.add_argument("--gt",      type=str, default=None)
    p.add_argument("--pred",    type=str, default=None)
    p.add_argument("--tracker", type=str, default="tracker")

    return p.parse_args()


def resolve_data_dir(args) -> Path:
    """
    @brief Resolves dataset root directory path.
    @param args Argparse namespace object.
    @return Resolved path object.
    """
    if args.data_dir:
        return Path(args.data_dir)
    candidates = [
        Path(__file__).parent.parent.parent / "data",
        Path(__file__).parent.parent / "data",
    ]
    for c in candidates:
        if c.exists():
            return c.resolve()
    return Path(__file__).parent.parent.parent / "data"


def main():
    """
    @brief Execution entry point.
    """
    args = parse_args()

    if args.gt and args.pred:
        _run_direct_mode(args)
        return

    data_dir = resolve_data_dir(args)
    log_info(f"Data directory : {data_dir}")

    runner    = TrackEvalRunner(data_root=str(data_dir), split=args.split)
    scenarios = [args.scenario] if args.scenario else None
    results   = runner.evaluate(trackers=args.trackers, scenarios=scenarios)

    if not results:
        log_error("No results produced. Check that pred/ files exist.")
        sys.exit(1)

    runner.print_results(results, compare=args.compare)

    if args.export:
        runner.export_results(results, args.export)


def _run_direct_mode(args):
    """
    @brief Executes evaluation on specific ground-truth and prediction files.
    @param args Argparse namespace object.
    """
    import tempfile, shutil

    gt_path   = Path(args.gt)
    pred_path = Path(args.pred)
    seq_name  = gt_path.parent.parent.name or "custom"

    tmp = Path(tempfile.mkdtemp(prefix="eval_direct_"))
    try:
        (tmp / seq_name / "gt").mkdir(parents=True)
        (tmp / seq_name / "pred").mkdir(parents=True)
        shutil.copy(gt_path,   tmp / seq_name / "gt"   / "gt.txt")
        shutil.copy(pred_path, tmp / seq_name / "pred" / f"{args.tracker}.txt")

        runner  = TrackEvalRunner(data_root=str(tmp), split=args.split)
        results = runner.evaluate(trackers=[args.tracker], scenarios=[seq_name])
        runner.print_results(results, compare=False)

        if args.export:
            runner.export_results(results, args.export)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


if __name__ == "__main__":
    main()