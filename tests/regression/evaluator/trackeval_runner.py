"""
@file      trackeval_runner.py
@brief     Programmatic interface to the official MOT Challenge evaluator (TrackEval).
@author    Mustafa Garip (mustafagarip@sushisystems.io)
@date      28.04.2026
@version   v1.0.0
@copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
"""

import tempfile
import shutil
from pathlib import Path
from typing import Optional
import sys

_PROJECT_ROOT = Path(__file__).parent.parent.parent.parent
_TOOLS_DIR = _PROJECT_ROOT / ".tools"

if str(_TOOLS_DIR) not in sys.path:
    sys.path.insert(0, str(_TOOLS_DIR))

try:
    from paths_manager import DIR_3RDPARTY
    _3RDPARTY_TRACKEVAL = DIR_3RDPARTY / "TrackEval"
except ImportError:
    _3RDPARTY_TRACKEVAL = _PROJECT_ROOT / "3rdparty" / "TrackEval"

if _3RDPARTY_TRACKEVAL.exists() and str(_3RDPARTY_TRACKEVAL) not in sys.path:
    sys.path.insert(0, str(_3RDPARTY_TRACKEVAL))

from loguru import logger

def _ensure_trackeval() -> bool:
    """
    @brief Verifies trackeval package availability.
    @return True if package is importable.
    """
    try:
        import trackeval  # noqa
        return True
    except ImportError:
        logger.error("TrackEval not found.\n"
                  "  Install: pip install git+https://github.com/JonathonLuiten/TrackEval.git")
        return False


def _make_seqinfo(seq_dir: Path, seq_name: str, frame_rate: int = 30) -> None:
    """
    @brief Generates seqinfo.ini for a sequence directory.
    @param seq_dir Destination directory.
    @param seq_name Sequence identifier.
    @param frame_rate Processing frequency.
    @return None.
    """
    seqinfo_path = seq_dir / "seqinfo.ini"
    if seqinfo_path.exists():
        return
    gt_path = seq_dir / "gt" / "gt.txt"
    seq_len = 0
    if gt_path.exists():
        with open(gt_path) as f:
            frames = {int(line.split(",")[0]) for line in f if line.strip()}
        seq_len = max(frames) if frames else 0
    with open(seqinfo_path, "w") as f:
        f.write("[Sequence]\n")
        f.write(f"name={seq_name}\n")
        f.write(f"imDir=img1\n")
        f.write(f"frameRate={frame_rate}\n")
        f.write(f"seqLength={seq_len}\n")
        f.write(f"imWidth=1920\n")
        f.write(f"imHeight=1080\n")
        f.write(f"imExt=.jpg\n")


def _detect_benchmark(seq_name: str) -> str:
    """
    @brief Identifies benchmark identifier from sequence name.
    @param seq_name Sequence identifier.
    @return Benchmark identifier string.
    """
    n = seq_name.upper()
    if n.startswith("MOT20"):
        return "MOT20"
    return "MOT17"


def _sanitize_pred_file(src: Path, dst: Path) -> tuple:
    """
    @brief Sanitizes prediction file for TrackEval compliance.
    @param src Source file path.
    @param dst Destination file path.
    @return Tuple of (rows_written, invalid_id_count, duplicate_count).
    """
    seen: dict = {}
    dst.parent.mkdir(parents=True, exist_ok=True)
    rows_written = skipped_id0 = skipped_dup = 0

    with open(src) as fin, open(dst, "w") as fout:
        for line in fin:
            line = line.strip()
            if not line:
                continue
            parts = line.split(",")
            if len(parts) < 6:
                continue
            frame  = int(parts[0])
            obj_id = int(float(parts[1]))
            if obj_id <= 0:
                skipped_id0 += 1
                continue
            seen.setdefault(frame, set())
            if obj_id in seen[frame]:
                skipped_dup += 1
                continue
            seen[frame].add(obj_id)
            while len(parts) < 10:
                parts.append("-1")
            fout.write(",".join(parts[:10]) + "\n")
            rows_written += 1

    return rows_written, skipped_id0, skipped_dup


class TrackEvalRunner:
    """
    @brief Orchestrator for TrackEval-based evaluation.
    """

    def __init__(self, data_root: str, split: str = "train", frame_rate: int = 30):
        """
        @brief Initializes runner with dataset parameters.
        @param data_root Root directory of the dataset.
        @param split Dataset split (train/test).
        @param frame_rate Sequence frame rate.
        """
        self.data_root  = Path(data_root)
        self.split      = split
        self.frame_rate = frame_rate

    def evaluate(
        self,
        trackers:  list,
        scenarios: Optional[list] = None,
        perf_fps:  Optional[dict] = None,
    ) -> dict:
        """
        @brief Executes evaluation across multiple trackers and sequences.
        @param trackers List of tracker identifiers.
        @param scenarios List of sequence identifiers.
        @param perf_fps Mapping of tracker performance metrics.
        @return Nested results mapping.
        """
        if not _ensure_trackeval():
            return {}
        if scenarios is None:
            scenarios = self._discover_scenarios()
        if not scenarios:
            logger.warning(f"No scenarios found in {self.data_root}")
            return {}

        groups: dict = {}
        for seq in scenarios:
            bm = _detect_benchmark(seq)
            groups.setdefault(bm, []).append(seq)

        results = {}
        for tracker in trackers:
            tracker_results: dict = {}

            for bm, bm_seqs in groups.items():
                bm_res = self._run_tracker(tracker, bm_seqs)
                for seq, m in bm_res.items():
                    if seq != "OVERALL":
                        tracker_results[seq] = m

            if perf_fps and tracker in perf_fps:
                for seq, fps in perf_fps[tracker].items():
                    if seq in tracker_results:
                        tracker_results[seq]["FPS"]        = fps
                        tracker_results[seq]["Latency_us"] = (1_000_000.0 / fps) if fps > 0 else 0.0

            from report_generator import _aggregate_seqs
            all_seq_names = list(tracker_results.keys())
            if all_seq_names:
                tracker_results["OVERALL"] = _aggregate_seqs(tracker_results, all_seq_names)

            results[tracker] = tracker_results
        return results

    def print_results(self, results: dict, compare: bool = True) -> None:
        """
        @brief Displays evaluation results to console.
        @param results Results mapping.
        @param compare Flag for color comparison.
        @return None.
        """
        from report_generator import print_trackeval_table
        print_trackeval_table(results, compare=compare)

    def export_results(self, results: dict, out_dir: str) -> None:
        """
        @brief Persists evaluation results to files.
        @param results Results mapping.
        @param out_dir Output directory path.
        @return None.
        """
        from report_generator import export_trackeval_results
        export_trackeval_results(results, out_dir)

    def _discover_scenarios(self) -> list:
        """
        @brief Scans data root for valid sequence directories.
        @return List of sequence identifiers.
        """
        return sorted(
            d.name for d in self.data_root.iterdir()
            if d.is_dir() and (d / "gt" / "gt.txt").exists()
        )

    def _run_tracker(self, tracker_name: str, scenarios: list) -> dict:
        """
        @brief Executes TrackEval engine for a single tracker.
        @param tracker_name Tracker identifier.
        @param scenarios List of sequences to evaluate.
        @return Results mapping for the tracker.
        """
        import trackeval
        import numpy as np

        for attr in ("float", "int", "bool", "complex"):
            if not hasattr(np, attr):
                setattr(np, attr, __builtins__[attr] if isinstance(__builtins__, dict)
                        else getattr(__builtins__, attr))

        tmp = Path(tempfile.mkdtemp(prefix="trackeval_"))
        print(f"{'='*60}")
        print(f"TRACKER: {tracker_name.upper()}")
        print(f"{'='*60}")

        try:
            gt_folder       = tmp / "gt"
            trackers_folder = tmp / "trackers"

            logger.info("Loading ground truth...")
            valid_seqs = []
            for seq in scenarios:
                src_gt = self.data_root / seq / "gt" / "gt.txt"
                if not src_gt.exists():
                    logger.warning(f"  [MISS] {seq:<32} gt.txt not found")
                    continue
                seq_root = gt_folder / seq
                (seq_root / "gt").mkdir(parents=True, exist_ok=True)
                shutil.copy(src_gt, seq_root / "gt" / "gt.txt")
                _make_seqinfo(seq_root, seq, self.frame_rate)
                (seq_root / "img1").mkdir(exist_ok=True)
                valid_seqs.append(seq)
                print(f"  [ OK ] {seq:<32} gt loaded")
            logger.info(f"  -> {len(valid_seqs)} sequences with GT")
            if not valid_seqs:
                return {}

            logger.info(f"Loading predictions for '{tracker_name}'...")
            submitted_seqs = []
            for seq in valid_seqs:
                src_pred = self.data_root / seq / "pred" / tracker_name / f"{seq}.txt"
                if not src_pred.exists():
                    logger.warning(f"  [MISS] {seq:<32} pred/{tracker_name}/{seq}.txt not found")
                    continue
                dst_dir = trackers_folder / tracker_name / self.split
                dst_dir.mkdir(parents=True, exist_ok=True)
                rows, sid0, sdup = _sanitize_pred_file(src_pred, dst_dir / f"{seq}.txt")
                msg = f"  [ OK ] {seq:<32} {rows} rows"
                if sid0: msg += f"  ! {sid0} id=0 removed"
                if sdup: msg += f"  ! {sdup} duplicates removed"
                print(msg)
                submitted_seqs.append(seq)
            logger.info(f"  -> {len(submitted_seqs)} sequences submitted")
            if not submitted_seqs:
                logger.warning(f"No pred files for '{tracker_name}' -- skipping.")
                return {}

            benchmark = _detect_benchmark(submitted_seqs[0])
            logger.info(f"Benchmark inferred: {benchmark}")
            seqmap_dir = tmp / "seqmaps"
            seqmap_dir.mkdir(parents=True, exist_ok=True)
            with open(seqmap_dir / f"{benchmark}-{self.split}.txt", "w") as f:
                f.write("name\n")
                for seq in submitted_seqs:
                    f.write(f"{seq}\n")

            logger.info("Running TrackEval engine...")
            dataset_config = {
                "GT_FOLDER":            str(gt_folder),
                "TRACKERS_FOLDER":      str(trackers_folder),
                "OUTPUT_FOLDER":        str(tmp / "output"),
                "TRACKERS_TO_EVAL":     [tracker_name],
                "CLASSES_TO_EVAL":      ["pedestrian"],
                "BENCHMARK":            benchmark,
                "SPLIT_TO_EVAL":        self.split,
                "DO_PREPROC":           True,
                "SEQMAP_FOLDER":        str(seqmap_dir),
                "SKIP_SPLIT_FOL":       True,
                "TRACKER_SUB_FOLDER":   self.split,
                "PRINT_RESULTS":        False,
                "PRINT_ONLY_COMBINED":  False,
                "OUTPUT_SUMMARY":       False,
                "OUTPUT_EMPTY_CLASSES": False,
                "OUTPUT_DETAILED":      False,
                "PLOT_CURVES":          False,
                "PRINT_CONFIG":         False,
            }
            evaluator_config = {
                "USE_PARALLEL":          False,
                "NUM_PARALLEL_CORES":    1,
                "BREAK_ON_ERROR":        False,
                "RETURN_ON_ERROR":       False,
                "LOG_ON_ERROR":          str(tmp / "error.log"),
                "PRINT_RESULTS":         False,
                "PRINT_CONFIG":          False,
                "TIME_PROGRESS":         False,
                "DISPLAY_LESS_PROGRESS": True,
                "OUTPUT_SUMMARY":        False,
                "OUTPUT_EMPTY_CLASSES":  False,
                "OUTPUT_DETAILED":       False,
                "PLOT_CURVES":           False,
            }

            hota_thresholds = [round(0.05 * i, 2) for i in range(1, 20)]

            dataset   = trackeval.datasets.MotChallenge2DBox(dataset_config)
            metrics   = [
                trackeval.metrics.HOTA({"HOTA_THRESH": hota_thresholds}),
                trackeval.metrics.CLEAR({"THRESHOLD": 0.5}),
                trackeval.metrics.Identity({"THRESHOLD": 0.5}),
            ]
            evaluator = trackeval.Evaluator(evaluator_config)
            raw, _msg = evaluator.evaluate([dataset], metrics)

            logger.info("Processing results...")
            return self._extract_results(raw, tracker_name, submitted_seqs)

        finally:
            shutil.rmtree(tmp, ignore_errors=True)

    @staticmethod
    def _scalar(v) -> float:
        """
        @brief Normalizes TrackEval metric to float.
        @param v Input metric value or collection.
        @return Normalized float.
        """
        import numpy as np
        if hasattr(v, "__len__"):
            return float(np.mean(v))
        return float(v) if v is not None else 0.0

    @staticmethod
    def _ci(d: dict, primary: str, *fallbacks) -> int:
        """
        @brief Retrieves integer metric with fallback support.
        @param d Source dictionary.
        @param primary Primary key.
        @param fallbacks Ordered fallback keys.
        @return Retrieved integer value.
        """
        for key in (primary,) + fallbacks:
            if key in d:
                return int(d[key])
        return 0

    @staticmethod
    def _cf(d: dict, primary: str, *fallbacks) -> float:
        """
        @brief Retrieves float metric with fallback support.
        @param d Source dictionary.
        @param primary Primary key.
        @param fallbacks Ordered fallback keys.
        @return Retrieved float value.
        """
        for key in (primary,) + fallbacks:
            if key in d:
                return float(d[key])
        return 0.0

    def _extract_results(self, raw: dict, tracker_name: str, submitted_seqs: list) -> dict:
        """
        @brief Parses raw TrackEval output into flat metrics.
        @param raw Raw TrackEval output.
        @param tracker_name Tracker identifier.
        @param submitted_seqs List of processed sequences.
        @return Mapped metrics.
        """
        results: dict = {}

        tracker_data = None
        for dk, dv in raw.items():
            if isinstance(dv, dict) and tracker_name in dv:
                tracker_data = dv[tracker_name]
                break

        if tracker_data is None:
            print(f"[TRACKEVAL] Tracker '{tracker_name}' not found in raw output.")
            return results

        for seq_name, seq_classes in tracker_data.items():
            if not isinstance(seq_classes, dict):
                continue

            seq_data = next(iter(seq_classes.values()), {})
            if not isinstance(seq_data, dict):
                continue

            flat: dict = {}

            if "HOTA" in seq_data:
                h = seq_data["HOTA"]
                flat["HOTA"]  = self._scalar(h.get("HOTA",  0))
                flat["DetA"]  = self._scalar(h.get("DetA",  0))
                flat["AssA"]  = self._scalar(h.get("AssA",  0))
                flat["LocA"]  = self._scalar(h.get("LocA",  0))
                flat["DetRe"] = self._scalar(h.get("DetRe", 0))
                flat["DetPr"] = self._scalar(h.get("DetPr", 0))
                flat["AssRe"] = self._scalar(h.get("AssRe", 0))
                flat["AssPr"] = self._scalar(h.get("AssPr", 0))

            if "CLEAR" in seq_data:
                c = seq_data["CLEAR"]
                flat["MOTA"]        = self._cf(c, "MOTA")
                flat["MOTP"]        = self._cf(c, "MOTP")
                flat["MT"]          = self._ci(c, "MT")
                flat["ML"]          = self._ci(c, "ML")
                flat["FP"]          = self._ci(c, "FP",   "CLR_FP",  "num_false_positives")
                flat["FN"]          = self._ci(c, "FN",   "CLR_FN",  "num_misses")
                flat["IDSW"]        = self._ci(c, "IDSW", "num_switches")
                flat["Frag"]        = self._ci(c, "Frag", "num_fragmentations")
                flat["num_objects"] = self._ci(c, "num_gt_dets", "Gt")
                flat["num_frames"]  = self._ci(c, "num_timesteps")

            if "Identity" in seq_data:
                i = seq_data["Identity"]
                flat["IDF1"] = float(i.get("IDF1", 0))
                flat["IDP"]  = float(i.get("IDP",  0))
                flat["IDR"]  = float(i.get("IDR",  0))

            flat.setdefault("FPS",        0.0)
            flat.setdefault("Latency_us", 0.0)

            if seq_name != "COMBINED_SEQ":
                results[seq_name] = flat
                print(f"  [ RESULT ] {seq_name:<32} "
                      f"HOTA={flat.get('HOTA',0)*100:.1f}%  "
                      f"MOTA={flat.get('MOTA',0)*100:.1f}%  "
                      f"IDF1={flat.get('IDF1',0)*100:.1f}%  "
                      f"FP={flat.get('FP',0)}  FN={flat.get('FN',0)}")

        return results