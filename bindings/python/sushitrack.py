# sushitrack.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""Python ctypes bindings for the SushiTrack C ABI.

Wraps the native library interface declared in include/SushiTrack/c_api/sushitrack_c.h.
Exposes Tracker, Track, and lifecycle states for multi-object tracking.
"""

from __future__ import annotations

import ctypes as C
import os
import platform
from collections.abc import Callable, Iterable, Sequence
from ctypes import POINTER, Structure, c_char_p, c_float, c_int, c_int64, c_void_p
from dataclasses import dataclass
from enum import IntEnum
from pathlib import Path
from typing import Any

__version__ = "1.0.0"

__all__ = [
    "__version__",
    "Tracker",
    "SushiTrack",
    "Track",
    "TrackState",
    "KinematicState",
    "LogLevel",
    "Status",
    "SushiTrackError",
    "SushiTrackObject",
    "SushiTrackTrack",
    "SushiTrackParams",
    "find_library",
]


class Status(IntEnum):
    """Return codes from tracker initialization and configuration calls."""

    SUCCESS = 0
    INVALID_ARGUMENT = -1
    INITIALIZATION_FAILURE = -2
    OUT_OF_MEMORY = -3
    RUNTIME_ERROR = -4
    BUFFER_OVERFLOW = -5


class TrackState(IntEnum):
    """Lifecycle state reported in track output records."""

    NEW = 0
    TRACKED = 1
    LOST = 2
    REMOVED = 3
    TENTATIVE = 4


class KinematicState(IntEnum):
    """Kinematic regime reported in track output records."""

    STATIONARY = 0
    CRUISING = 1
    MANEUVERING = 2
    COASTING = 3


class LogLevel(IntEnum):
    """Logging levels passed to a registered log callback, numbered as SushiLog's."""

    TRACE = 0
    DEBUG = 1
    INFO = 2
    WARN = 3
    ERROR = 4
    FATAL = 5


class SushiTrackError(RuntimeError):
    """Raised when a SushiTrack C ABI call fails."""


class SushiTrackObject(Structure):
    """Input detection bounding box and optional appearance feature."""

    _fields_ = [
        ("x", c_float),
        ("y", c_float),
        ("width", c_float),
        ("height", c_float),
        ("score", c_float),
        ("label", c_int),
        ("feat", POINTER(c_float)),
        ("feat_dim", c_int),
    ]


class SushiTrackTrack(Structure):
    """Output tracking state for a single entity from the C ABI."""

    _fields_ = [
        ("track_id", c_int64),
        ("age", c_int64),
        ("last_seen_frame", c_int64),
        ("feat", POINTER(c_float)),
        ("x", c_float),
        ("y", c_float),
        ("width", c_float),
        ("height", c_float),
        ("score", c_float),
        ("lost_time", c_float),
        ("label", c_int),
        ("state", c_int),
        ("kinematic_state", c_int),
        ("feat_dim", c_int),
        ("is_activated", c_int),
        ("is_confirmed", c_int),
        ("hits", c_int),
        ("frames_since_last_update", c_int),
        ("maneuver_probability", c_float),
    ]


class SushiTrackParams(Structure):
    """Configuration parameters for the tracking algorithm in C ABI order."""

    _fields_ = [
        ("time_aware", c_int),
        ("frame_rate", c_int),
        ("track_buffer", c_int),
        ("track_thresh", c_float),
        ("high_thresh", c_float),
        ("match_thresh", c_float),
        ("enable_tentative", c_int),
        ("enable_reid", c_int),
        ("enable_mahalanobis", c_int),
        ("primary_assoc_mode", c_int),
        ("fuse_score", c_int),
        ("iou_match_thresh", c_float),
        ("unconfirmed_iou_thresh", c_float),
        ("duplicate_iou_thresh", c_float),
        ("enable_xyah", c_int),
        ("iou_type", c_int),
        ("iou_plus_one", c_int),
        ("kalman_type", c_int),
        ("feature_momentum", c_float),
        ("reid_high_thresh", c_float),
        ("reid_high_iou_weight", c_float),
        ("reid_low_thresh", c_float),
        ("reid_low_iou_weight", c_float),
        ("reid_recovery_thresh", c_float),
        ("max_feature_history", c_int),
        ("enable_ocm", c_int),
        ("enable_oru", c_int),
        ("ocm_vdc_weight", c_float),
        ("ocm_delta_t", c_int),
        ("ocm_min_velocity", c_float),
        ("ocm_max_age", c_int),
        ("ocm_maneuver_weight_scale", c_float),
        ("tentative_match_thresh", c_float),
        ("tentative_confirm_hits", c_int),
        ("tentative_max_age_seconds", c_float),
        ("tentative_max_miss_frames", c_int),
        ("tentative_init_suppress_iou", c_float),
        ("gating_thresh", c_float),
        ("motion_cost_weight", c_float),
        ("max_area_ratio", c_float),
        ("tentative_fast_thresh", c_float),
        ("tentative_fast_hits", c_int),
        ("tentative_slow_thresh", c_float),
        ("tentative_slow_hits_offset", c_int),
        ("tentative_vslow_hits_offset", c_int),
        ("reid_direct_assignment_weight", c_float),
    ]


class SushiLogRecord(Structure):
    """Holds one record SushiLog hands a C sink; no string is NUL-terminated."""

    _fields_ = [
        ("size", C.c_uint32),
        ("level", C.c_uint8),
        ("project", c_void_p),
        ("project_size", C.c_size_t),
        ("module", c_void_p),
        ("module_size", C.c_size_t),
        ("file", c_void_p),
        ("file_size", C.c_size_t),
        ("line", C.c_uint32),
        ("thread", C.c_uint32),
        ("time_ns", c_int64),
        ("message", c_void_p),
        ("message_size", C.c_size_t),
        ("text", c_void_p),
        ("text_size", C.c_size_t),
    ]


LOG_SINK = C.CFUNCTYPE(None, POINTER(SushiLogRecord), c_void_p)


@dataclass(frozen=True, slots=True)
class Track:
    """Holds the output state for a single active track."""

    track_id: int
    x: float
    y: float
    width: float
    height: float
    score: float
    label: int
    state: TrackState
    kinematic_state: KinematicState
    age: int
    last_seen_frame: int
    lost_time: float
    is_activated: bool
    is_confirmed: bool
    hits: int
    frames_since_last_update: int
    maneuver_probability: float
    feat: list[float] | None = None


def find_library(explicit: str | None = None) -> str:
    """Locates the SushiTrack shared library binary on disk."""
    candidates: list[str] = []
    if explicit:
        candidates.append(explicit)
    val = os.environ.get("SUSHITRACK_LIB")
    if val:
        candidates.append(val)

    suffixes = [".dll"] if platform.system() == "Windows" else [".so", ".dylib"]
    names = [f"sushitrack{s}" for s in suffixes] + [f"libsushitrack{s}" for s in suffixes]

    here = Path(__file__).resolve().parent
    for name in names:
        candidates.append(str(here / name))

    root = Path(__file__).resolve().parents[2]
    build = root / "build"
    for sub in ("bin/Release", "bin/Debug", "bin", "lib", "Release", "Debug", ""):
        for name in names:
            candidates.append(str(build / sub / name))

    for path in candidates:
        if path and os.path.exists(path):
            return os.path.abspath(path)

    raise FileNotFoundError(
        f"Could not locate SushiTrack shared library. Build with `st build` or set SUSHITRACK_LIB."
    )


def _resolve_fn(lib: C.CDLL, *names: str) -> Any:
    """Returns the first matching attribute on lib, or None if none exist."""
    for name in names:
        if hasattr(lib, name):
            return getattr(lib, name)
    return None


class _NativeBindings:
    """Wraps resolved native C function pointers for a loaded library."""

    def __init__(self, lib: C.CDLL) -> None:
        self.lib = lib

        self.get_default_params = _resolve_fn(
            lib, "sushitrack_get_default_params", "sushitrack_GetDefaultParams"
        )
        if self.get_default_params:
            self.get_default_params.argtypes = [POINTER(SushiTrackParams)]
            self.get_default_params.restype = None

        self.create = _resolve_fn(
            lib, "sushitrack_create", "sushitrack_Create"
        )
        if self.create:
            self.create.argtypes = [POINTER(SushiTrackParams)]
            self.create.restype = c_void_p

        self.create_ex = _resolve_fn(
            lib, "sushitrack_create_ex", "sushitrack_CreateEx"
        )
        if self.create_ex:
            self.create_ex.argtypes = [POINTER(SushiTrackParams), POINTER(c_void_p)]
            self.create_ex.restype = c_int

        self.create_tracker = _resolve_fn(
            lib, "sushitrack_create_tracker", "sushitrack_CreateFromConfig"
        )
        if self.create_tracker:
            self.create_tracker.argtypes = [c_char_p]
            self.create_tracker.restype = c_void_p

        self.load_params_from_json = _resolve_fn(
            lib, "sushitrack_load_params_from_json", "sushitrack_LoadParamsFromJson"
        )
        if self.load_params_from_json:
            self.load_params_from_json.argtypes = [c_char_p, POINTER(SushiTrackParams)]
            self.load_params_from_json.restype = c_int

        self.update = _resolve_fn(
            lib, "sushitrack_update", "sushitrack_Update"
        )
        if self.update:
            self.update.argtypes = [
                c_void_p,
                POINTER(SushiTrackObject),
                c_int,
                POINTER(SushiTrackTrack),
                c_int,
                c_float,
                POINTER(c_int),
            ]
            self.update.restype = c_int

        self.destroy = _resolve_fn(
            lib, "sushitrack_destroy_tracker", "sushitrack_destroy", "sushitrack_Destroy"
        )
        if self.destroy:
            self.destroy.argtypes = [c_void_p]
            self.destroy.restype = None


def _load_sushilog(lib_dir: str) -> C.CDLL:
    """Loads the SushiLog shared library that sits beside the SushiTrack one."""
    name = "sushilog.dll" if platform.system() == "Windows" else "libsushilog.so"
    lib = C.CDLL(os.path.join(lib_dir, name))
    lib.sl_set_log_sink.argtypes = [LOG_SINK, c_void_p]
    lib.sl_set_log_sink.restype = None
    lib.sl_set_log_synchronous.argtypes = [c_int]
    lib.sl_set_log_synchronous.restype = None
    return lib


class Tracker:
    """Manages a SushiTrack tracking engine instance bound to a native handle.

    Construct once per video stream and feed detections frame by frame through
    update(). Native resources release on close() or context manager exit.
    """

    def __init__(
        self,
        lib_path: str | None = None,
        max_tracks: int = 256,
        config_path: str | None = None,
        config: str | None = None,
        **param_overrides: Any,
    ) -> None:
        self._hdl: c_void_p | None = None
        self._log_cb: Any = None
        resolved_lib = find_library(lib_path)
        if platform.system() == "Windows" and hasattr(os, "add_dll_directory"):
            lib_dir = os.path.dirname(os.path.abspath(resolved_lib))
            if os.path.isdir(lib_dir):
                os.add_dll_directory(lib_dir)
        raw_lib = C.CDLL(resolved_lib)
        self._lib_dir = os.path.dirname(os.path.abspath(resolved_lib))
        self._bind = _NativeBindings(raw_lib)

        cfg = config_path if config_path is not None else config
        if not param_overrides and cfg is not None and self._bind.create_tracker:
            path_bytes = None if cfg == "auto" else cfg.encode("utf-8")
            self._hdl = c_void_p(self._bind.create_tracker(path_bytes))
            if not self._hdl or not self._hdl.value:
                raise SushiTrackError(f"Failed to instantiate tracker from config: {cfg}")
        else:
            params = SushiTrackParams()
            if self._bind.get_default_params:
                self._bind.get_default_params(C.byref(params))

            if cfg is not None and self._bind.load_params_from_json:
                path_bytes = None if cfg == "auto" else cfg.encode("utf-8")
                status = self._bind.load_params_from_json(path_bytes, C.byref(params))
                if status != Status.SUCCESS:
                    raise SushiTrackError(f"load_params_from_json failed: {Status(status).name}")

            for key, value in param_overrides.items():
                if not hasattr(params, key):
                    raise AttributeError(f"Unknown SushiTrack parameter: {key!r}")
                field_type = dict(SushiTrackParams._fields_)[key]
                setattr(params, key, field_type(value).value)

            handle = c_void_p()
            if self._bind.create_ex:
                status = self._bind.create_ex(C.byref(params), C.byref(handle))
                if status != Status.SUCCESS or not handle.value:
                    raise SushiTrackError(f"Tracker creation failed: {Status(status).name}")
                self._hdl = handle
            elif self._bind.create:
                self._hdl = c_void_p(self._bind.create(C.byref(params)))
                if not self._hdl or not self._hdl.value:
                    raise SushiTrackError("Tracker creation failed with null handle")
            else:
                raise SushiTrackError("No valid tracker constructor found in shared library")

        self._cap = int(max_tracks)
        self._out = (SushiTrackTrack * self._cap)()

    def update(self, detections: Iterable[Sequence[Any]], dt: float = 1.0) -> list[Track]:
        """Processes one frame of detections and returns active tracks.

        Returns at most max_tracks tracks, the lowest-priority ones left out.

        Raises:
            SushiTrackError: The native update returned a failure status.
        """
        if self._hdl is None or not self._hdl.value:
            raise SushiTrackError("update() called on an uninitialized or closed tracker")

        dets = list(detections)
        n = len(dets)
        arr = (SushiTrackObject * n)() if n else None
        feat_refs: list[Any] = []

        for i, d in enumerate(dets):
            item = arr[i]
            item.x = float(d[0])
            item.y = float(d[1])
            item.width = float(d[2])
            item.height = float(d[3])
            item.score = float(d[4])
            item.label = int(d[5])
            feat = d[6] if len(d) > 6 else None
            if feat is not None and len(feat) > 0:
                buf = (c_float * len(feat))(*[float(f) for f in feat])
                feat_refs.append(buf)
                item.feat = C.cast(buf, POINTER(c_float))
                item.feat_dim = len(feat)
            else:
                item.feat = None
                item.feat_dim = 0

        written = c_int(0)
        status = self._bind.update(
            self._hdl, arr, n, self._out, self._cap, c_float(dt), C.byref(written)
        )
        if status not in (Status.SUCCESS, Status.BUFFER_OVERFLOW):
            raise SushiTrackError(f"update() of {n} detections failed: {Status(status).name}")
        return [self._to_track(self._out[i]) for i in range(written.value)]

    @staticmethod
    def _to_track(t: SushiTrackTrack) -> Track:
        """Converts a native C track record into a Python Track dataclass."""
        feat = None
        if t.feat and t.feat_dim > 0:
            feat = [t.feat[i] for i in range(t.feat_dim)]
        return Track(
            track_id=int(t.track_id),
            x=float(t.x),
            y=float(t.y),
            width=float(t.width),
            height=float(t.height),
            score=float(t.score),
            label=int(t.label),
            state=TrackState(t.state),
            kinematic_state=KinematicState(t.kinematic_state),
            age=int(t.age),
            last_seen_frame=int(t.last_seen_frame),
            lost_time=float(t.lost_time),
            is_activated=bool(t.is_activated),
            is_confirmed=bool(t.is_confirmed),
            hits=int(t.hits),
            frames_since_last_update=int(t.frames_since_last_update),
            maneuver_probability=float(t.maneuver_probability),
            feat=feat,
        )

    def set_log_callback(self, callback: Callable[[LogLevel, str], None] | None) -> None:
        """Makes fn(level: LogLevel, message: str) SushiLog's sink, for the whole process.

        Records reach the callback on the thread that logged them. None restores stderr.
        """
        sushilog = _load_sushilog(self._lib_dir)
        if callback is None:
            sushilog.sl_set_log_sink(LOG_SINK(), None)
            sushilog.sl_set_log_synchronous(0)
            self._log_cb = None
            return

        def _sink(record: Any, _user: Any) -> None:
            value = record.contents
            text = C.string_at(value.message, value.message_size).decode("utf-8", "replace")
            callback(LogLevel(value.level), text)

        self._log_cb = LOG_SINK(_sink)
        sushilog.sl_set_log_synchronous(1)
        sushilog.sl_set_log_sink(self._log_cb, None)

    def close(self) -> None:
        """Destroys the native tracker instance. Safe to call multiple times."""
        if self._hdl and self._hdl.value and self._bind.destroy:
            self._bind.destroy(self._hdl)
            self._hdl = None

    def __enter__(self) -> Tracker:
        """Enters the runtime context for the tracker."""
        return self

    def __exit__(self, *args: Any) -> None:
        """Exits the runtime context and destroys native tracker resources."""
        self.close()

    def __del__(self) -> None:
        """Releases native tracker resources on garbage collection."""
        self.close()


SushiTrack = Tracker
