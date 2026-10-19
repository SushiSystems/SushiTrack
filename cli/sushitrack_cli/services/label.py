# label.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""The MOT label-based benchmark pipeline behind `st eval`."""

from .. import console
from ..env import load_build_env
from ..errors import UnknownTrackerError
from ..process import RUNNER
from . import evaluate
from .artifacts import find_binary

#: The benchmark program `st eval` runs.
BENCHMARK = "mot_evaluation"

# Registry key -> the display name the C++ TrackerRegistry and demo.py use.
TRACKERS = {
    "sushitrack": "SushiTrack",
    "bytetrack": "ByteTrack",
    "ocsort": "OCSORT",
}


def resolve_tracker(name):
    """Normalise a tracker name to its registry key.

    Raises:
        UnknownTrackerError: The registry holds no tracker of that name.
    """
    key = (name or "sushitrack").lower()
    if key not in TRACKERS:
        unique_names = sorted(set(TRACKERS.values()))
        raise UnknownTrackerError(f"Unknown tracker '{name}'. "
                                  f"Available: {', '.join(unique_names)}.")
    return key


def sequences(paths, scenario=None, private=False, public=False):
    """Sequence names under the data root, filtered by name and detector flavour.

    The private/public split only means something for MOT sequences: the ``YOLOX``
    suffix marks private detections, everything else is the public benchmark
    detector, and non-MOT sets (DanceTrack) are unaffected by either flag.
    """
    data_root = paths.data_root()
    if not data_root.is_dir():
        return []

    available = sorted(d.name for d in data_root.iterdir()
                       if d.is_dir() and (d / "gt" / "gt.txt").exists())
    if scenario:
        matched = [scenario] if scenario in available else \
            [s for s in available if scenario.lower() in s.lower()]
    else:
        matched = available

    selected = []
    for name in matched:
        if name.upper().startswith("MOT"):
            is_yolox = name.upper().endswith("YOLOX")
            if private and not is_yolox:
                continue
            if public and is_yolox:
                continue
        selected.append(name)
    return selected


def _benchmark_args(paths, names, trackers):
    """The mot_evaluation arguments that run *trackers* over the sequences *names*."""
    args = ["--data", str(paths.data_root()), "--settings", str(paths.bridge)]
    for tracker in trackers:
        args += ["--tracker", tracker]
    for name in names:
        args += ["--sequence", name]
    return args


def _parse_perf(stdout):
    """Extract ``[ PERF <tracker> ] <sequence>: <n> us/frame`` lines.

    Parsed per line and skipped on any malformed one: the timing column is
    diagnostic, and a format change upstream should not fail the evaluation.
    """
    perf = {}
    for line in stdout.splitlines():
        if "[ PERF " not in line:
            continue
        try:
            start = line.index("[ PERF ") + len("[ PERF ")
            end = line.index(" ]", start)
            tracker = line[start:end].lower()
            sequence, _, timing = line[end + 2:].strip().partition(":")
            micros = float(timing.split("us/frame")[0].strip())
        except (ValueError, IndexError):
            continue
        perf.setdefault(tracker, {})[sequence.strip()] = micros
    return perf


def _run_benchmark(tool, paths, args):
    """Run mot_evaluation with *args* and parse its PERF lines from stdout."""
    binary = find_binary(paths.build, BENCHMARK)
    if not binary:
        console.error(f"{BENCHMARK} not found. Run `st build` first.")
        return 1, {}

    console.command(" ".join([str(binary), *args]))
    code, out, err = RUNNER.capture([str(binary), *args], cwd=paths.root,
                                    env=load_build_env(tool, paths.build, paths.config))
    if code != 0:
        console.error(f"{BENCHMARK} failed (exit {code}): {(err or out).strip()}")
        return code, {}
    return 0, _parse_perf(out)


def evaluate_sequences(cfg, paths, scenario=None, tracker="sushitrack",
                       compare=False, eval_only=False, trackeval=False,
                       export=None, private=False, public=False):
    """Run the benchmark and score it.

    Returns:
        2 for a flag conflict, 1 when no sequence matched, else the pipeline's exit code.
    """
    if private and public:
        console.error("--private and --public are mutually exclusive.")
        return 2

    console.header("Evaluate")
    names = sequences(paths, scenario, private, public)
    if not names:
        console.error(f"No sequence matched under {paths.data_root()}; nothing was scored.")
        return 1

    trackers = list(TRACKERS) if compare else [tracker]
    console.info(f"{len(names)} sequence(s) x {len(trackers)} tracker(s): "
                 f"{', '.join(TRACKERS[t] for t in trackers)}")

    perf = {}
    if not eval_only:
        code, perf = _run_benchmark(cfg.tool, paths, _benchmark_args(paths, names, trackers))
        if code != 0:
            return code

    if trackeval:
        return evaluate.run_official(paths, names, trackers)
    return evaluate.run_internal(paths, names, trackers, perf, export,
                                 compare=compare)
