# data_loader.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026 Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""File loader for MOT Challenge data and prediction files."""

import os
from dataclasses import dataclass
from typing import List


@dataclass
class Detection:
    """
    @brief Data structure for object bounding box.
    @var id     Identity identifier.
    @var bbox   Coordinates [x, y, w, h] in pixels.
    @var ignore Flag for exclusion from scoring.
    @var role   Classification role (valid, distractor, zero_marked).
    @var score  Confidence score.
    """
    id:     int
    bbox:   List[float]
    ignore: bool  = False
    role:   str   = "valid"
    score:  float = 1.0


#: @brief Identifier for the pedestrian class in MOT Challenge datasets.
PEDESTRIAN_CLASS         = 1

#: @brief Set of class IDs considered distractors in MOT17.
DISTRACTOR_CLASSES_MOT17 = {2, 7, 8, 12}

#: @brief Set of class IDs considered distractors in MOT20.
DISTRACTOR_CLASSES_MOT20 = {2, 6, 7, 8, 12}


def _detect_benchmark(filepath: str) -> str:
    """
    @brief Identifies MOT17 or MOT20 benchmark from path.
    @param filepath Path to data file.
    @return Benchmark identifier string.
    """
    if "MOT20" in filepath.upper():
        return "MOT20"
    return "MOT17"


def load_gt_file(filepath: str) -> dict:
    """
    @brief Parses MOTChallenge ground-truth file.
    @param filepath Path to ground-truth file.
    @return Frame identifier mapping to Detection lists.
    """
    data: dict = {}
    if not os.path.exists(filepath):
        return data

    benchmark   = _detect_benchmark(filepath)
    distractors = (DISTRACTOR_CLASSES_MOT20 if benchmark == "MOT20"
                   else DISTRACTOR_CLASSES_MOT17)

    with open(filepath, "r", encoding="utf-8") as f:
        for line in f:
            parts = line.strip().split(",")
            if len(parts) < 6:
                continue

            frame  = int(parts[0])
            obj_id = int(float(parts[1]))
            x, y, w, h = map(float, parts[2:6])

            conf = float(parts[6]) if len(parts) >= 7 else 1.0
            cls  = int(float(parts[7])) if len(parts) >= 8 else PEDESTRIAN_CLASS

            if conf == 0:
                role = "zero_marked"
            elif cls in distractors:
                role = "distractor"
            elif cls != PEDESTRIAN_CLASS:
                role = "zero_marked"
            else:
                role = "valid"

            data.setdefault(frame, []).append(Detection(
                id=obj_id,
                bbox=[x, y, w, h],
                ignore=(role != "valid"),
                role=role,
            ))

    return data


def load_pred_file(filepath: str) -> dict:
    """
    @brief Parses tracker prediction file.
    @param filepath Path to prediction file.
    @return Frame identifier mapping to Detection lists.
    """
    data: dict = {}
    if not os.path.exists(filepath):
        return data

    with open(filepath, "r", encoding="utf-8") as f:
        for line in f:
            parts = line.strip().split(",")
            if len(parts) < 6:
                continue

            frame  = int(parts[0])
            obj_id = int(float(parts[1]))
            x, y, w, h = map(float, parts[2:6])
            conf = float(parts[6]) if len(parts) >= 7 else 1.0

            data.setdefault(frame, []).append(Detection(
                id=obj_id,
                bbox=[x, y, w, h],
                score=conf,
            ))

    return data
