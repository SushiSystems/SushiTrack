"""TrackEval metric evaluation, internal wrapper or the official script."""

import shutil
import sys

from .. import console, proc

# MOT20 sequences are scored against the MOT20 benchmark definition, everything el
_BENCHMARKS = ("MOT17", "MOT20")


def _load_evaluator(paths):
    """Import the repo's evaluator lazily. Returns the module trio or None."""
    evaluator_dir = str(paths.evaluator)
    if evaluator_dir not in sys.path:
        sys.path.insert(0, evaluator_dir)
    try:
        from trackeval_runner import TrackEvalRunner
        from report_generator import (export_trackeval_results,
                                      print_trackeval_table)
    except ImportError as exc:
        # The evaluator has its own dependency set (numpy, scipy, TrackEval); name
        console.error(
            f"Internal evaluator unavailable: {exc}.\n"
            f"Its dependencies come from environment.yml "
            f"(conda env create -f environment.yml), or install the missing "
            f"package directly.\n"
            f"To score without it, run `st eval --trackeval` and use the "
            f"official TrackEval script.")
        return None
    return TrackEvalRunner, print_trackeval_table, export_trackeval_results


def run_internal(paths, sequences, trackers, perf, export_dir=None,
                 compare=False):
    """Evaluate with the repo's own TrackEval wrapper and print the table."""
    loaded = _load_evaluator(paths)
    if loaded is None:
        return 1
    runner_cls, print_table, export_results = loaded

    data_root = paths.data_root()
    if not data_root.is_dir():
        console.error(f"Data directory not found: {data_root}")
        return 1

    console.header("TrackEval (internal)")
    try:
        runner = runner_cls(data_root=str(data_root))
        results = runner.evaluate(
            trackers=list(trackers),
            scenarios=list(sequences),
            # The binary reports microseconds per frame; the report wants FPS.
            perf_fps={tracker: {seq: 1_000_000.0 / micros
                                for seq, micros in per_seq.items()}
                      for tracker, per_seq in perf.items() if per_seq},
        )
    except Exception as exc:
        console.error(f"Evaluation failed: {exc}")
        return 1

    if not results:
        console.error("No evaluation results. Verify that pred/*.txt files exist.")
        return 1
    print_table(results, compare=compare)
    if export_dir:
        export_results(results, export_dir)
        console.success(f"Results exported to {export_dir}")
    return 0


def _stage_official_data(paths, sequences, trackers, trackeval_dir):
    """Copy predictions and ground truth into TrackEval's expected tree.

    The whole data/ tree is rebuilt each run: a leftover sequence from a previous
    invocation would otherwise stay in the seqmap and be scored again.
    """
    data_root = paths.data_root()
    shutil.rmtree(trackeval_dir / "data", ignore_errors=True)
    staged = 0

    for tracker in trackers:
        for seq_dir in sorted(data_root.iterdir()):
            name = seq_dir.name
            if name not in sequences or not (seq_dir / "gt" / "gt.txt").exists():
                continue
            prediction = seq_dir / "pred" / tracker / f"{name}.txt"
            if not prediction.exists():
                continue
            benchmark = "MOT20" if name.upper().startswith("MOT20") else "MOT17"
            tracker_dir = (trackeval_dir / "data" / "trackers" / "mot_challenge"
                           / f"{benchmark}-train" / tracker / "data")
            tracker_dir.mkdir(parents=True, exist_ok=True)
            shutil.copy2(prediction, tracker_dir / f"{name}.txt")
            staged += 1

            gt_dir = (trackeval_dir / "data" / "gt" / "mot_challenge"
                      / f"{benchmark}-train" / name)
            if not (gt_dir / "gt" / "gt.txt").exists():
                (gt_dir / "gt").mkdir(parents=True, exist_ok=True)
                shutil.copy2(seq_dir / "gt" / "gt.txt", gt_dir / "gt" / "gt.txt")
                seqinfo = seq_dir / "seqinfo.ini"
                if seqinfo.exists():
                    shutil.copy2(seqinfo, gt_dir / "seqinfo.ini")
    return staged


def run_official(paths, sequences, trackers):
    """Run 3rdparty/TrackEval's run_mot_challenge.py over the staged results."""
    trackeval_dir = paths.third_party / "TrackEval"
    script = trackeval_dir / "scripts" / "run_mot_challenge.py"
    if not script.exists():
        console.error(f"Official TrackEval script not found at {script}. "
                      f"Initialise the submodule: git submodule update --init")
        return 1

    console.header("TrackEval (official)")
    staged = _stage_official_data(paths, sequences, trackers, trackeval_dir)
    if not staged:
        console.error("No prediction files to evaluate. Run `st eval` "
                      "without --eval-only first.")
        return 1

    failures = 0
    for benchmark in _BENCHMARKS:
        available, all_sequences = [], set()
        for tracker in trackers:
            tracker_data = (trackeval_dir / "data" / "trackers" / "mot_challenge"
                            / f"{benchmark}-train" / tracker / "data")
            if not tracker_data.is_dir():
                continue
            names = [f.stem for f in tracker_data.iterdir() if f.suffix == ".txt"]
            if names:
                available.append(tracker)
                all_sequences.update(names)
        if not available:
            continue

        seqmap_dir = trackeval_dir / "data" / "gt" / "mot_challenge" / "seqmaps"
        seqmap_dir.mkdir(parents=True, exist_ok=True)
        (seqmap_dir / f"{benchmark}-train.txt").write_text(
            "name\n" + "".join(f"{name}\n" for name in sorted(all_sequences)),
            encoding="utf-8")

        command = [sys.executable, str(script), "--BENCHMARK", benchmark,
                   "--TRACKERS_TO_EVAL", *available,
                   "--SPLIT_TO_EVAL", "train"]
        if len(sequences) == 1:
            command += ["--SEQ_INFO", sequences[0]]
        if proc.run(command, cwd=trackeval_dir) != 0:
            failures += 1
    return 1 if failures else 0
