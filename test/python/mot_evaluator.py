"""
@file      mot_evaluator.py
@brief     Metric evaluator for CLEAR MOT based on motmetrics.
@author    Mustafa Garip (mustafagarip@sushisystems.io)
@date      28.04.2026
@version   v1.0.0
@copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
"""

import time
from dataclasses import dataclass, field
from typing import Dict, List, Optional

import motmetrics as mm
import numpy as np

from data_loader import Detection


_DEFAULT_IOU_THRESHOLD = 0.5


@dataclass
class EvalResult:
    """
    @brief Container for evaluation results.
    @var name        Identifier of the sequence.
    @var summary     Metric calculation summary object.
    @var elapsed_sec Execution time in seconds.
    @var num_frames  Number of processed frames.
    """
    name: str
    summary: object
    elapsed_sec: float = 0.0
    num_frames: int = 0


def _iou(b1: List[float], b2: List[float]) -> float:
    """
    @brief Calculates Intersection over Union between bounding boxes.
    @param b1 First box [x, y, w, h].
    @param b2 Second box [x, y, w, h].
    @return IoU value in range [0, 1].
    """
    x1, y1, w1, h1 = b1
    x2, y2, w2, h2 = b2
    ix = max(0.0, min(x1 + w1, x2 + w2) - max(x1, x2))
    iy = max(0.0, min(y1 + h1, y2 + h2) - max(y1, y2))
    inter = ix * iy
    if inter == 0.0:
        return 0.0
    return inter / (w1 * h1 + w2 * h2 - inter)


class SequenceEvaluator:
    """
    @brief Evaluator for a single tracking sequence.
    """

    def __init__(self, name: str, iou_threshold: float = _DEFAULT_IOU_THRESHOLD):
        """
        @brief Initializes evaluator with sequence identifier.
        @param name Sequence identifier.
        @param iou_threshold Threshold for match acceptance.
        """
        self.name          = name
        self.iou_threshold = iou_threshold
        self._acc          = mm.MOTAccumulator(auto_id=True)
        self._t0           = time.perf_counter()
        self._frame_count  = 0

    def update(
        self,
        gt_frame:   List[Detection],
        pred_frame: List[Detection],
    ) -> None:
        """
        @brief Accumulates metrics for a single frame.
        @param gt_frame Ground-truth detections.
        @param pred_frame Tracker predictions.
        """
        self._frame_count += 1

        valid_gt  = [d for d in gt_frame if not d.ignore]
        ignore_gt = [d for d in gt_frame if d.ignore]

        valid_pred: List[Detection] = []
        for p in pred_frame:
            suppressed = any(
                _iou(ig.bbox, p.bbox) >= self.iou_threshold
                for ig in ignore_gt
            )
            if not suppressed:
                valid_pred.append(p)

        gt_ids   = [d.id for d in valid_gt]
        pred_ids = [d.id for d in valid_pred]

        n_gt, n_pred = len(gt_ids), len(pred_ids)
        if n_gt == 0 and n_pred == 0:
            self._acc.update([], [], np.empty((0, 0)))
            return

        dists = np.full((n_gt, n_pred), np.nan)
        for i, g in enumerate(valid_gt):
            for j, p in enumerate(valid_pred):
                iou = _iou(g.bbox, p.bbox)
                if iou >= self.iou_threshold:
                    dists[i, j] = 1.0 - iou

        self._acc.update(gt_ids, pred_ids, dists)

    def result(self) -> EvalResult:
        """
        @brief Computes metrics for the accumulated sequence data.
        @return Evaluation result object.
        """
        mh = mm.metrics.create()
        summary = mh.compute(
            self._acc,
            metrics=[
                "num_frames",
                "num_objects",
                "num_unique_objects",
                "mota",
                "motp",
                "idf1",
                "idp",
                "idr",
                "num_switches",
                "num_false_positives",
                "num_misses",
                "num_fragmentations",
                "mostly_tracked",
                "mostly_lost",
                "precision",
                "recall",
            ],
            name=self.name,
        )
        elapsed = time.perf_counter() - self._t0
        return EvalResult(
            name=self.name,
            summary=summary,
            elapsed_sec=elapsed,
            num_frames=self._frame_count,
        )


class MultiSequenceEvaluator:
    """
    @brief Aggregator for metrics across multiple sequences.
    """

    def __init__(self):
        """
        @brief Initializes aggregator with empty state.
        """
        self._accumulators: List[mm.MOTAccumulator] = []
        self._names: List[str] = []

    def add_sequence(self, seq_ev: SequenceEvaluator) -> None:
        """
        @brief Adds sequence evaluator data to the aggregator.
        @param seq_ev Sequence evaluator instance.
        """
        self._accumulators.append(seq_ev._acc)
        self._names.append(seq_ev.name)

    def aggregate(self, name: str = "OVERALL") -> EvalResult:
        """
        @brief Computes aggregated metrics across all sequences.
        @param name Identifier for the aggregate result.
        @return Evaluation result object.
        """
        mh = mm.metrics.create()
        metrics = [
            "num_frames", "num_objects", "num_unique_objects",
            "mota", "motp", "idf1", "idp", "idr",
            "num_switches", "num_false_positives", "num_misses",
            "num_fragmentations", "mostly_tracked", "mostly_lost",
            "precision", "recall",
        ]
        summary = mh.compute_many(
            self._accumulators,
            metrics=metrics,
            names=self._names,
            generate_overall=True,
        )
        summary = summary.rename(index={"OVERALL": name})
        return EvalResult(name=name, summary=summary)