"""The MOT label-based benchmark pipeline behind `ds eval`.

Not a CTest suite: it runs the regression binary over det.txt-backed sequences,
parses the per-sequence ``[ PERF ]`` timings out of its output, and feeds them to
TrackEval to print a metric table (HOTA / DetA / AssA / MOTA / IDF1 / IDSW / …).
`ds test --suite regression` only answers pass/fail for the same binary.
"""

import sys

from .. import console, proc
from ..env import load_build_env
from . import discovery, evaluate

# Registry key -> the display name the C++ TrackerRegistry and demo.py use.
TRACKERS = {
    "sushitrack": "SushiTrack",
    "bytetrack": "ByteTrack",
    "ocsort": "OCSORT",
}


def resolve_tracker(name):
    """Normalise a tracker name to its registry key, or exit with the choices."""
    key = (name or "sushitrack").lower()
    if key not in TRACKERS:
        raise SystemExit(f"Unknown tracker '{name}'. "
                         f"Available: {', '.join(TRACKERS.values())}.")
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


def _gtest_filter(names, trackers):
    """GTest filter over the ``<Sequence>_<tracker>`` test names."""
    parts = [f"*{name.replace('-', '_')}_{tracker}*"
             for tracker in trackers for name in names]
    return ":".join(parts) if parts else "-*"


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


def _run_regression(paths, gtest_filter):
    binary = discovery.find_binary(paths.build, "regression_test")
    if not binary:
        console.error("regression_test binary not found. Run `ds build` first.")
        return 1, {}

    console.command(f"{binary} --gtest_filter={gtest_filter}")
    # Captured rather than streamed because the PERF lines have to be parsed; the
    # output is echoed on failure so nothing is lost when it matters.
    code, out, err = proc.capture([str(binary), f"--gtest_filter={gtest_filter}"],
                                  cwd=paths.root,
                                  env=load_build_env(paths.build))
    if code != 0:
        console.error(f"regression_test failed (exit {code}).")
        print(out)
        print(err, file=sys.stderr)
        return code, {}
    return 0, _parse_perf(out)


def evaluate_sequences(cfg, paths, scenario=None, tracker="sushitrack",
                       compare=False, eval_only=False, trackeval=False,
                       export=None, private=False, public=False):
    """Run the benchmark and score it. Returns an exit code."""
    if private and public:
        console.error("--private and --public are mutually exclusive.")
        return 2

    console.header("Evaluate")
    names = sequences(paths, scenario, private, public)
    if not names:
        console.warn(f"No sequences matched under {paths.data_root()}.")
        return 0

    trackers = list(TRACKERS) if compare else [tracker]
    console.info(f"{len(names)} sequence(s) x {len(trackers)} tracker(s): "
                 f"{', '.join(TRACKERS[t] for t in trackers)}")

    perf = {}
    if not eval_only:
        code, perf = _run_regression(paths, _gtest_filter(names, trackers))
        if code != 0:
            return code

    if trackeval:
        return evaluate.run_official(paths, names, trackers)
    return evaluate.run_internal(paths, names, trackers, perf, export,
                                 compare=compare)
