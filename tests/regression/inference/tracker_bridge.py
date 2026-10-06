# tracker_bridge.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""Bridges the inference pipeline with tracker implementations."""

import ctypes
import os
import platform
import sys
from ctypes import POINTER, Structure, c_float, c_int, c_void_p

_BINDINGS_DIR = os.path.abspath(
    os.path.join(os.path.dirname(__file__), "..", "..", "..", "bindings", "python")
)
if _BINDINGS_DIR not in sys.path:
    sys.path.insert(0, _BINDINGS_DIR)


class CDetection(Structure):
    _fields_ = [
        ("x", c_float),
        ("y", c_float),
        ("w", c_float),
        ("h", c_float),
        ("score", c_float),
        ("label", c_int),
        ("feat", POINTER(c_float)),
        ("feat_dim", c_int)
    ]

class CTrackResult(Structure):
    _fields_ = [
        ("track_id", c_int),
        ("x", c_float),
        ("y", c_float),
        ("w", c_float),
        ("h", c_float),
        ("score", c_float)
    ]

class TrackerBridge:
    def __init__(self, tracker_name: str, build_dir: str = "build"):
        self.tracker_name = tracker_name

        if tracker_name.strip().lower() == "sushitrack":
            self._mode = "binding"
            from sushitrack import Tracker
            self._st = Tracker(config="auto")
            return

        self._mode = "shim"
        self.lib = self._load_library(build_dir)
        self._setup_argtypes()

        name_bytes = tracker_name.encode('utf-8')
        self.tracker_ptr = self.lib.create_tracker(name_bytes)
        if not self.tracker_ptr:
            raise RuntimeError(f"Failed to create tracker: {tracker_name}")

    def _load_library(self, build_dir: str):
        system = platform.system()
        lib_name = "tracker_bridge.dll" if system == "Windows" else "libtracker_bridge.so"

        search_paths = [
            os.path.join(build_dir, "bin", lib_name),
            os.path.join(build_dir, "bin", "Release", lib_name),
            os.path.join(build_dir, "bin", "Debug", lib_name),
            os.path.join(build_dir, lib_name),
            os.path.join(build_dir, "Release", lib_name),
            os.path.join(build_dir, "Debug", lib_name),
            os.path.join(build_dir, "test", "cpp", lib_name),
        ]

        for p in search_paths:
            if os.path.exists(p):
                return ctypes.CDLL(os.path.abspath(p))

        raise FileNotFoundError(f"Could not find {lib_name} in {build_dir}")

    def _setup_argtypes(self):
        self.lib.create_tracker.argtypes = [ctypes.c_char_p]
        self.lib.create_tracker.restype = c_void_p

        self.lib.delete_tracker.argtypes = [c_void_p]
        self.lib.delete_tracker.restype = None

        self.lib.tracker_process_c.argtypes = [
            c_void_p,
            POINTER(CDetection),
            c_int,
            POINTER(CTrackResult),
            POINTER(c_int),
            c_int
        ]
        self.lib.tracker_process_c.restype = None

    def update(self, detections: list, embeddings=None):
        """Processes detections and returns tracked results."""
        if self._mode == "binding":
            return self._update_binding(detections, embeddings)
        return self._update_shim(detections, embeddings)

    def _update_binding(self, detections: list, embeddings=None):
        dets = []
        for i, det in enumerate(detections):
            row = (det[0], det[1], det[2], det[3], det[4], int(det[5]))
            if embeddings is not None and i < len(embeddings):
                emb = embeddings[i]
                if emb is not None and len(emb) > 0:
                    row = row + (emb,)
            dets.append(row)

        tracks = self._st.update(dets, dt=1.0)
        return [
            {"track_id": t.track_id, "bbox": [t.x, t.y, t.width, t.height], "score": t.score}
            for t in tracks if t.is_activated
        ]

    def _update_shim(self, detections: list, embeddings=None):
        num_dets = len(detections)
        if num_dets == 0:
            return []

        det_array = (CDetection * num_dets)()
        feat_refs = []

        for i, det in enumerate(detections):
            det_array[i].x = det[0]
            det_array[i].y = det[1]
            det_array[i].w = det[2]
            det_array[i].h = det[3]
            det_array[i].score = det[4]
            det_array[i].label = int(det[5])

            if embeddings is not None and i < len(embeddings):
                emb = embeddings[i]
                dim = len(emb)
                c_float_array = (ctypes.c_float * dim)(*emb)
                feat_refs.append(c_float_array)
                det_array[i].feat = ctypes.cast(c_float_array, POINTER(ctypes.c_float))
                det_array[i].feat_dim = dim
            else:
                det_array[i].feat = ctypes.cast(None, POINTER(ctypes.c_float))
                det_array[i].feat_dim = 0

        max_results = max(num_dets * 2, 100)
        res_array = (CTrackResult * max_results)()
        num_res = c_int(0)

        self.lib.tracker_process_c(
            self.tracker_ptr,
            det_array,
            num_dets,
            res_array,
            ctypes.byref(num_res),
            max_results
        )

        results = []
        for i in range(num_res.value):
            r = res_array[i]
            results.append({
                "track_id": r.track_id,
                "bbox": [r.x, r.y, r.w, r.h],
                "score": r.score
            })

        return results

    def __del__(self):
        if getattr(self, '_mode', None) == "binding":
            st = getattr(self, '_st', None)
            if st is not None:
                st.close()
            return
        if hasattr(self, 'tracker_ptr') and self.tracker_ptr:
            if hasattr(self, 'lib') and self.lib:
                self.lib.delete_tracker(self.tracker_ptr)
