"""
@file      report_generator.py
@brief     Console and file reporter for MOT Challenge evaluation results.
@author    Mustafa Garip (mustafagarip@sushisystems.io)
@date      28.04.2026
@version   v1.0.0
@copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
"""

import json
import csv
import sys
from pathlib import Path
from typing import Optional

_TOOLS_DIR = Path(__file__).parent.parent.parent / ".tools"
if str(_TOOLS_DIR) not in sys.path:
    sys.path.insert(0, str(_TOOLS_DIR))

from loguru import logger
RED, GREEN, YELLOW, BLUE, RESET = "\033[91m", "\033[92m", "\033[93m", "\033[94m", "\033[0m"

#: @brief ANSI escape sequence for bold text.
BOLD = "\033[1m"


def _visible_len(s: str) -> int:
    """
    @brief Calculates printable length of string.
    @param s Input string with ANSI escape sequences.
    @return Number of visible characters.
    """
    import re
    return len(re.sub(r'\033\[[0-9;]*m', '', s))


def _ansi_ljust(s: str, width: int) -> str:
    """
    @brief Left-justifies string accounting for ANSI sequences.
    @param s Input string.
    @param width Target width.
    @return Justified string.
    """
    return s + " " * max(0, width - _visible_len(s))


def _ansi_rjust(s: str, width: int) -> str:
    """
    @brief Right-justifies string accounting for ANSI sequences.
    @param s Input string.
    @param width Target width.
    @return Justified string.
    """
    return " " * max(0, width - _visible_len(s)) + s


def _pct(v: float) -> str:
    """
    @brief Formats fraction as percentage string.
    @param v Fraction in range [0, 1].
    @return Formatted string.
    """
    return f"{v * 100:6.1f}%"


def _colored(v: float, ref: Optional[float], reverse: bool = False) -> str:
    """
    @brief Applies ANSI coloring to percentage string.
    @param v Value to format.
    @param ref Reference value for comparison.
    @param reverse Flag to invert improvement logic.
    @return Colored string.
    """
    s = _pct(v)
    if ref is None:
        return s
    if (v > ref) != reverse:
        return GREEN + s + RESET
    elif (v < ref) != reverse:
        return RED + s + RESET
    return s


#: @brief Column definitions for the evaluation report table.
_COLS = [
    ("Tracker",   14, False),
    ("Scenario",  24, False),
    ("HOTA",       8, True),
    ("DetA",       8, True),
    ("AssA",       8, True),
    ("MOTA",       8, True),
    ("IDF1",       8, True),
    ("IDSW",       6, True),
    ("FP",         9, True),
    ("FN",         9, True),
    ("Lat(us)",   10, True),
]

#: @brief Pre-computed header string for the console report.
_HEADER = "".join(
    _ansi_ljust(h, w) if not rj else _ansi_rjust(h, w)
    for h, w, rj in _COLS
)

#: @brief separator string for table layout.
_SEP = "-" * sum(w for _, w, _ in _COLS)


def _fmt_row(tracker: str, seq: str, m: dict, ref: Optional[dict] = None) -> str:
    """
    @brief Formats result row for console output.
    @param tracker Tracker identifier.
    @param seq Sequence identifier.
    @param m Metric collection.
    @param ref Reference metric collection.
    @return Formatted string.
    """
    hota = m.get("HOTA", 0.0)
    deta = m.get("DetA", 0.0)
    assa = m.get("AssA", 0.0)
    mota = m.get("MOTA", 0.0)
    idf1 = m.get("IDF1", 0.0)
    idsw = m.get("IDSW", 0)
    fp   = m.get("FP",   m.get("CLR_FP", 0))
    fn   = m.get("FN",   m.get("CLR_FN", 0))

    lat_us = m.get("Latency_us", 0.0)
    if lat_us == 0.0:
        fps = m.get("FPS", 0.0)
        lat_us = (1_000_000.0 / fps) if fps > 0 else 0.0

    def c(val, key, reverse=False):
        return _colored(val, ref.get(key) if ref else None, reverse)

    cells = [
        _ansi_ljust(tracker,          14),
        _ansi_ljust(seq,              24),
        _ansi_rjust(c(hota, "HOTA"),  8),
        _ansi_rjust(c(deta, "DetA"),  8),
        _ansi_rjust(c(assa, "AssA"),  8),
        _ansi_rjust(c(mota, "MOTA"),  8),
        _ansi_rjust(c(idf1, "IDF1"),  8),
        _ansi_rjust(str(idsw),         6),
        _ansi_rjust(str(fp),           9),
        _ansi_rjust(str(fn),           9),
        _ansi_rjust(f"{lat_us:.0f}" if lat_us > 0 else "-", 10),
    ]
    return "".join(cells)


def _dataset_prefix(seq: str) -> str:
    """
    @brief Identifies dataset prefix from sequence name.
    @param seq Sequence name.
    @return Dataset prefix string.
    """
    u = seq.upper()
    if u.startswith("MOT20"):
        return "MOT20"
    if u.startswith("MOT17"):
        return "MOT17"
    if u.startswith("DANCETRACK"):
        return "DanceTrack"
    return ""


def _aggregate_seqs(tracker_results: dict, seqs: list) -> dict:
    """
    @brief Computes weighted aggregate across sequences.
    @param tracker_results Mapping of sequence to metrics.
    @param seqs List of sequences to aggregate.
    @return Aggregated metric collection.
    """
    rate_keys  = {"HOTA", "DetA", "AssA", "LocA", "DetRe", "DetPr",
                  "AssRe", "AssPr", "MOTA", "MOTP", "IDF1", "IDP", "IDR"}
    count_keys = {"IDSW", "FP", "FN", "MT", "ML", "Frag",
                  "num_objects", "num_frames"}
    perf_keys  = {"FPS", "Latency_us"}

    total_frames = 0
    agg: dict = {}

    for seq in seqs:
        m = tracker_results.get(seq)
        if m is None:
            continue
        w = m.get("num_frames", 1) or 1
        total_frames += w
        for k in rate_keys | perf_keys:
            v = m.get(k, 0.0)
            agg[k] = agg.get(k, 0.0) + v * w
        for k in count_keys:
            agg[k] = agg.get(k, 0) + m.get(k, 0)

    if total_frames > 0:
        for k in rate_keys | perf_keys:
            if k in agg:
                agg[k] /= total_frames

    agg["num_frames"] = total_frames
    return agg


def _print_aggregate_block(label: str, tracker_names: list, results: dict,
                            seqs: list, compare: bool,
                            agg_override=None) -> None:
    """
    @brief Prints aggregate block for console report.
    @param label Block label.
    @param tracker_names List of tracker identifiers.
    @param results Mapping of tracker to sequence metrics.
    @param seqs List of sequences in block.
    @param compare Flag for color comparison.
    @param agg_override Optional override for aggregate metrics.
    @return None.
    """
    print(_SEP)
    print(label)
    print(_SEP)
    print(BOLD + _HEADER + RESET)

    agg_ref = None
    for ti, tracker in enumerate(tracker_names):
        if agg_override is not None:
            m_agg = agg_override.get(tracker)
        else:
            m_agg = _aggregate_seqs(results[tracker], seqs) if seqs else None
        if not m_agg:
            continue
        row_ref = agg_ref if (compare and ti > 0) else None
        print(_fmt_row(tracker, label, m_agg, row_ref))
        if ti == 0:
            agg_ref = m_agg


def print_trackeval_table(results: dict, compare: bool = True) -> None:
    """
    @brief Prints evaluation table to console.
    @param results Mapping of tracker to sequence metrics.
    @param compare Flag for color comparison.
    @return None.
    """
    tracker_names = list(results.keys())

    all_seqs = sorted({
        seq for tr in results.values()
        for seq in tr
        if seq != "OVERALL"
    })

    mot17_seqs = [s for s in all_seqs if _dataset_prefix(s) == "MOT17"]
    mot20_seqs = [s for s in all_seqs if _dataset_prefix(s) == "MOT20"]
    dancetrack_seqs = [s for s in all_seqs if _dataset_prefix(s) == "DanceTrack"]

    print()
    print(BOLD + _HEADER + RESET)
    print(_SEP)

    for seq in all_seqs:
        ref_metrics = (
            results[tracker_names[0]].get(seq)
            if compare and len(tracker_names) > 1
            else None
        )
        for i, tracker in enumerate(tracker_names):
            m = results[tracker].get(seq)
            if m is None:
                print(_ansi_ljust(tracker, 14) + _ansi_ljust(seq, 24) + "  N/A")
                continue
            row_ref = ref_metrics if (compare and i > 0) else None
            print(_fmt_row(tracker, seq, m, row_ref))

        if len(tracker_names) > 1:
            print()

    if mot17_seqs:
        _print_aggregate_block("MOT17", tracker_names, results, mot17_seqs, compare)
    if mot20_seqs:
        _print_aggregate_block("MOT20", tracker_names, results, mot20_seqs, compare)
    if dancetrack_seqs:
        _print_aggregate_block("DanceTrack", tracker_names, results, dancetrack_seqs, compare)

    has_overall = any("OVERALL" in results[t] for t in tracker_names)
    overall_override = (
        {t: results[t].get("OVERALL") for t in tracker_names}
        if has_overall else None
    )
    _print_aggregate_block("OVERALL", tracker_names, results, all_seqs, compare,
                           agg_override=overall_override)

    print(_SEP)
    print()


def export_trackeval_results(results: dict, out_dir: str) -> None:
    """
    @brief Exports evaluation results to JSON and CSV formats.
    @param results Nested results mapping.
    @param out_dir Output directory path.
    @return None.
    """
    out = Path(out_dir)
    out.mkdir(parents=True, exist_ok=True)

    json_path = out / "mot_results.json"
    with open(json_path, "w") as f:
        json.dump(results, f, indent=2)
    logger.success(f"JSON export: {json_path}")

    csv_path = out / "mot_results.csv"
    fields = [
        "tracker", "scenario",
        "HOTA", "DetA", "AssA", "LocA",
        "MOTA", "MOTP", "IDF1", "IDP", "IDR",
        "IDSW", "FP", "FN", "MT", "ML", "Frag",
        "num_objects", "num_frames", "FPS", "Latency_us",
    ]
    with open(csv_path, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=fields, extrasaction="ignore")
        writer.writeheader()
        for tracker, seqs in results.items():
            for seq, m in seqs.items():
                row = {"tracker": tracker, "scenario": seq}
                row.update(m)
                writer.writerow(row)
    logger.success(f"CSV export: {csv_path}")