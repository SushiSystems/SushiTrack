# label.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""The MOT label-based benchmark pipeline behind `st eval`."""

import shutil
from pathlib import Path

from ..errors import UnknownTrackerError
from ..workspace import Workspace, current_workspace
from . import evaluate
from .artifacts import find_binary
from .profile_record import read_latency

#: The benchmark program `st eval` runs.
BENCHMARK = "mot_engine"

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


def sequences(paths, benchmark, scenario=None, private=False, public=False):
    """Sequence names under the data root, filtered by name and detector flavour.

    The private/public split only means something for MOT sequences: the ``YOLOX``
    suffix marks private detections, everything else is the public benchmark
    detector, and non-MOT sets (DanceTrack) are unaffected by either flag.
    """
    data_root = paths.data_root(benchmark)
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


def _benchmark_args(paths, data_root, names, trackers, profiles=None):
    """The mot_engine arguments that run *trackers* over the sequences *names* of *data_root*.

    *profiles* is the folder the profile records go in; `mot_engine` writes them in its default
    folder when it is None.
    """
    args = ["--data", str(data_root), "--settings", str(paths.bridge)]
    if profiles is not None:
        args += ["--profiles", str(profiles)]
    for tracker in trackers:
        args += ["--tracker", tracker]
    for name in names:
        args += ["--sequence", name]
    return args


def _parse_perf(stdout, console):
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
            console.warn(f"Skipped a PERF line that does not parse: {line.strip()!r}")
            continue
        perf.setdefault(tracker, {})[sequence.strip()] = micros
    return perf


def _use_records(profiles, trackers, names, perf, console):
    """Replace each PERF figure with the median of its profile record, where one exists.

    Prints the median with its quartiles per record. A pair with no record keeps its PERF
    figure, and one with neither has no timing.
    """
    for tracker in trackers:
        for name in names:
            latency = read_latency(Path(profiles) / f"{tracker}_{name}.sprec.json")
            if latency is None:
                continue
            perf.setdefault(tracker, {})[name] = latency.median_us
            console.info(f"  {tracker} {name}: median {latency.median_us:.1f} us/frame "
                         f"(quartiles {latency.low_us:.1f} to {latency.high_us:.1f}, "
                         f"{latency.periods} frames)")
    return perf


def _run_benchmark(space: Workspace, tool, paths, args):
    """Run mot_engine with *args* and parse its PERF lines from stdout."""
    console = space.console
    binary = find_binary(paths.build, BENCHMARK)
    if not binary:
        console.error(f"{BENCHMARK} not found. Run `st build` first.")
        return 1, {}

    console.command(" ".join([str(binary), *args]))
    code, out, err = space.runner.capture(
        [str(binary), *args], cwd=paths.root,
        env=space.build_env(tool, paths.build, paths.config))
    if code != 0:
        console.error(f"{BENCHMARK} failed (exit {code}): {(err or out).strip()}")
        return code, {}
    return 0, _parse_perf(out, console)


def evaluate_sequences(cfg, paths, benchmark=None, scenario=None, tracker="sushitrack",
                       compare=False, eval_only=False, trackeval=False,
                       export=None, private=False, public=False, workspace=None):
    """Run the engine over each selected benchmark and score it.

    Args:
        workspace: What the run prints and starts the engine through; none uses the real ones.

    Returns:
        2 for a flag conflict or an unknown benchmark, 1 when no sequence matched, else the
        first non-zero exit code of a benchmark's run, or 0.
    """
    space = workspace or current_workspace()
    console = space.console
    if private and public:
        console.error("--private and --public are mutually exclusive.")
        return 2
    known = paths.benchmark_names()
    if benchmark is not None and benchmark not in known:
        console.error(f"Unknown benchmark '{benchmark}'. Available: {', '.join(known)}.")
        return 2

    console.header("Evaluate")
    trackers = list(TRACKERS) if compare else [tracker]
    matched = 0
    for name in [benchmark] if benchmark else known:
        names = sequences(paths, name, scenario, private, public)
        if not names:
            continue
        matched += len(names)
        code = _evaluate_benchmark(space, cfg, paths, name, names, trackers, compare,
                                   eval_only, trackeval, export)
        if code != 0:
            return code
    if not matched:
        console.error(f"No sequence matched under {paths.benchmarks}; nothing was scored.")
        return 1
    return 0


def _evaluate_benchmark(space, cfg, paths, benchmark, names, trackers, compare, eval_only,
                        trackeval, export):
    """Run the engine over one benchmark's sequences *names*, then score them."""
    console = space.console
    data_root = paths.data_root(benchmark)
    console.info(f"{benchmark}: {len(names)} sequence(s) x {len(trackers)} tracker(s): "
                 f"{', '.join(TRACKERS[t] for t in trackers)}")
    perf = {}
    if not eval_only:
        profiles = paths.build / "profiles"
        code, perf = _run_benchmark(space, cfg.tool, paths,
                                    _benchmark_args(paths, data_root, names, trackers, profiles))
        if code != 0:
            return code
        perf = _use_records(profiles, trackers, names, perf, console)
    if trackeval:
        return evaluate.run_official(paths, data_root, names, trackers, workspace=space)
    export_dir = None if export is None else str(Path(export) / benchmark)
    return evaluate.run_internal(paths, data_root, names, trackers, perf, export_dir,
                                 compare=compare, workspace=space)


def clean_predictions(paths, workspace=None):
    """Remove the pred/ folder of every sequence of every benchmark.

    Args:
        workspace: What the clean prints through; none uses the real console.

    Returns:
        0 when every pred/ folder is gone, 1 when one could not be removed.
    """
    console = (workspace or current_workspace()).console
    console.header("Clean predictions")
    removed = 0
    for benchmark in paths.benchmark_names():
        for prediction in sorted(paths.data_root(benchmark).glob("*/pred")):
            shutil.rmtree(prediction, ignore_errors=True)
            if prediction.exists():
                console.error(f"Could not remove {prediction}; close whatever holds a file in it.")
                return 1
            removed += 1
    console.success(f"Removed {removed} pred/ folder(s).")
    return 0
