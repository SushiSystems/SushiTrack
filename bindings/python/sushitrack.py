"""
sushitrack.py — Python ctypes binding for the SushiTrack C API.

A complete, dependency-free wrapper around the public C interface declared in
``include/SushiTrackC.h``. It mirrors the three C structs and the five entry
points exactly, then exposes a small Pythonic facade (the ``SushiTrack`` class)
so callers never touch ctypes directly.

    from sushitrack import SushiTrack

    tracker = SushiTrack(frame_rate=30, enable_tentative=1, iou_type=1)
    for frame in stream:
        dets = [(x, y, w, h, score, label), ...]     # top-left x, y, w, h
        tracks = tracker.update(dets, dt=1.0 / 30.0)
        for t in tracks:
            print(t.track_id, t.x, t.y, t.width, t.height, t.state)
    tracker.close()

The shared library (``sushitrack.dll`` / ``libsushitrack.so``) is located
automatically under the project ``build`` tree, or via the ``SUSHITRACK_LIB``
environment variable, or by passing ``lib_path`` explicitly.

Author:    Mustafa Garip (mustafagarip@sushisystems.io)
Copyright: (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
"""

from __future__ import annotations

import ctypes as C
import os
import platform
from collections import namedtuple
from ctypes import POINTER, Structure, c_char_p, c_float, c_int, c_int64, c_void_p
from enum import IntEnum
from pathlib import Path
from typing import Iterable, List, Optional, Sequence

__all__ = [
    "SushiTrack",
    "Track",
    "TrackState",
    "KinematicState",
    "Status",
    "SushiTrackError",
    "SushiTrackObject",
    "SushiTrackTrack",
    "SushiTrackParams",
    "find_library",
]


# ---------------------------------------------------------------------------
# Enumerations (mirror SushiTrackTypes.h and the track output fields)
# ---------------------------------------------------------------------------

class Status(IntEnum):
    """Return codes from ``SushiTrack_CreateEx`` (mirror ``SushiTrack_Status``)."""
    SUCCESS = 0
    INVALID_ARGUMENT = -1
    INITIALIZATION_FAILURE = -2
    OUT_OF_MEMORY = -3
    RUNTIME_ERROR = -4
    BUFFER_OVERFLOW = -5


class TrackState(IntEnum):
    """Lifecycle state reported in ``SushiTrack_Track::state``."""
    NEW = 0
    TRACKED = 1
    LOST = 2
    REMOVED = 3
    TENTATIVE = 4


class KinematicState(IntEnum):
    """Kinematic regime reported in ``SushiTrack_Track::kinematic_state``."""
    STATIONARY = 0
    CRUISING = 1
    MANEUVERING = 2
    COASTING = 3


class LogLevel(IntEnum):
    """Logging levels passed to a registered log callback."""
    DEBUG = 0
    INFO = 1
    WARN = 2
    ERROR = 3


class SushiTrackError(RuntimeError):
    """Raised when a C-API call fails (non-success status or null handle)."""


# ---------------------------------------------------------------------------
# ctypes structs — field order and types must match include/SushiTrackC.h exactly
# ---------------------------------------------------------------------------

class SushiTrackObject(Structure):
    """Mirror of ``SushiTrack_Object`` (one input detection)."""
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
    """Mirror of ``SushiTrack_Track`` (one output track)."""
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
    """Mirror of ``SushiTrack_Params`` — every field in declared header order."""
    _fields_ = [
        # --- Basic ---
        ("time_aware", c_int),
        ("frame_rate", c_int),
        ("track_buffer", c_int),
        ("track_thresh", c_float),
        ("high_thresh", c_float),
        ("match_thresh", c_float),
        ("enable_tentative", c_int),
        ("enable_reid", c_int),
        ("enable_mahalanobis", c_int),
        # --- Advanced: association ---
        ("primary_assoc_mode", c_int),
        ("fuse_score", c_int),
        ("iou_match_thresh", c_float),
        ("unconfirmed_iou_thresh", c_float),
        ("duplicate_iou_thresh", c_float),
        # --- Advanced: coordinate system ---
        ("enable_xyah", c_int),
        ("iou_type", c_int),
        ("iou_plus_one", c_int),
        ("kalman_type", c_int),
        # --- ReID params ---
        ("feature_momentum", c_float),
        ("reid_high_thresh", c_float),
        ("reid_high_iou_weight", c_float),
        ("reid_low_thresh", c_float),
        ("reid_low_iou_weight", c_float),
        ("reid_recovery_thresh", c_float),
        ("max_feature_history", c_int),
        # --- Observation-Centric Momentum (OCM) ---
        ("enable_ocm", c_int),
        ("ocm_vdc_weight", c_float),
        ("ocm_delta_t", c_int),
        ("ocm_min_velocity", c_float),
        ("ocm_max_age", c_int),
        ("ocm_maneuver_weight_scale", c_float),
        # --- Tentative logic ---
        ("tentative_match_thresh", c_float),
        ("tentative_confirm_hits", c_int),
        ("tentative_max_age_seconds", c_float),
        ("tentative_max_miss_frames", c_int),
        ("tentative_init_suppress_iou", c_float),
        # --- Mahalanobis gating ---
        ("gating_thresh", c_float),
        ("motion_cost_weight", c_float),
        ("max_area_ratio", c_float),
        # --- Extended ---
        ("tentative_fast_thresh", c_float),
        ("tentative_fast_hits", c_int),
        ("tentative_slow_thresh", c_float),
        ("tentative_slow_hits_offset", c_int),
        ("tentative_vslow_hits_offset", c_int),
        ("reid_direct_assignment_weight", c_float),
    ]


# C log-callback signature: void(SushiTrack_LogLevel, const char*)
LOG_CALLBACK = C.CFUNCTYPE(None, c_int, c_char_p)


# ---------------------------------------------------------------------------
# Output record
# ---------------------------------------------------------------------------

Track = namedtuple(
    "Track",
    [
        "track_id", "x", "y", "width", "height", "score", "label",
        "state", "kinematic_state", "age", "last_seen_frame", "lost_time",
        "is_activated", "is_confirmed", "hits", "frames_since_last_update",
        "maneuver_probability", "feat",
    ],
)
Track.__doc__ = "A single output track. Coordinates are top-left x, y plus width, height."


# ---------------------------------------------------------------------------
# Library discovery
# ---------------------------------------------------------------------------

def find_library(explicit: Optional[str] = None) -> str:
    """Locate the SushiTrack shared library.

    Resolution order: an explicit path, the ``SUSHITRACK_LIB`` environment
    variable, then the standard CMake output locations under the project's
    ``build`` tree (this file lives at ``bindings/python/`` relative to root).

    :param explicit: An explicit path to the shared library, or None.
    :returns: An absolute path to the located library.
    :raises FileNotFoundError: If no library can be found.
    """
    candidates: List[str] = []
    if explicit:
        candidates.append(explicit)
    env = os.environ.get("SUSHITRACK_LIB")
    if env:
        candidates.append(env)

    lib_name = "sushitrack.dll" if platform.system() == "Windows" else "libsushitrack.so"

    # Self-contained deploy: the shared library sits next to this module
    # (see `sushitrack project build --deploy python`).
    here = Path(__file__).resolve().parent
    candidates.append(str(here / lib_name))

    root = Path(__file__).resolve().parents[2]   # bindings/python/ -> project root
    build = root / "build"
    for sub in ("bin/Release", "bin/Debug", "bin", "lib", "Release", "Debug", ""):
        candidates.append(str(build / sub / lib_name))

    for path in candidates:
        if path and os.path.exists(path):
            return os.path.abspath(path)

    raise FileNotFoundError(
        f"Could not locate {lib_name}. Build it with `sushitrack project build`, "
        f"or set SUSHITRACK_LIB / pass lib_path explicitly. Tried: {candidates}"
    )


def _bind(lib: C.CDLL) -> C.CDLL:
    """Declare argument and return types for the C entry points."""
    lib.SushiTrack_GetDefaultParams.argtypes = [POINTER(SushiTrackParams)]
    lib.SushiTrack_GetDefaultParams.restype = None

    lib.SushiTrack_Create.argtypes = [POINTER(SushiTrackParams)]
    lib.SushiTrack_Create.restype = c_void_p

    lib.SushiTrack_CreateEx.argtypes = [POINTER(SushiTrackParams), POINTER(c_void_p)]
    lib.SushiTrack_CreateEx.restype = c_int

    lib.SushiTrack_LoadParamsFromJson.argtypes = [c_char_p, POINTER(SushiTrackParams)]
    lib.SushiTrack_LoadParamsFromJson.restype = c_int

    lib.SushiTrack_Update.argtypes = [
        c_void_p, POINTER(SushiTrackObject), c_int,
        POINTER(SushiTrackTrack), c_int, c_float,
    ]
    lib.SushiTrack_Update.restype = c_int

    lib.SushiTrack_Destroy.argtypes = [c_void_p]
    lib.SushiTrack_Destroy.restype = None

    lib.SushiTrack_SetLogCallback.argtypes = [LOG_CALLBACK]
    lib.SushiTrack_SetLogCallback.restype = None
    return lib


# ---------------------------------------------------------------------------
# Pythonic facade
# ---------------------------------------------------------------------------

class SushiTrack:
    """A SushiTrack tracker instance bound to one shared-library handle.

    Construct once per video stream and feed detections frame by frame through
    :meth:`update`. The instance owns a native handle; release it with
    :meth:`close`, ``del``, or by using the object as a context manager.

    :param lib_path: Explicit path to the shared library (else auto-discovered).
    :param max_tracks: Capacity of the reusable output buffer.
    :param config: Optional JSON configuration. ``None`` (default) uses the
        compiled defaults only; ``"auto"`` lets the library resolve
        ``sushitrack.json`` through its own search order (``SUSHITRACK_CONFIG``
        env var, then an upward search); any other string is treated as an
        explicit file path. Values are layered defaults → file → keyword
        overrides, so explicit ``param_overrides`` always win.
    :param param_overrides: Any ``SushiTrack_Params`` field, e.g.
        ``kalman_type=2``, ``iou_type=1``, ``time_aware=1``. Unspecified fields
        keep whatever the defaults/config established.
    """

    def __init__(self, lib_path: Optional[str] = None, max_tracks: int = 256,
                 config: Optional[str] = None, **param_overrides):
        self._lib = _bind(C.CDLL(find_library(lib_path)))

        params = SushiTrackParams()
        self._lib.SushiTrack_GetDefaultParams(C.byref(params))

        if config is not None:
            json_path = None if config == "auto" else config.encode("utf-8")
            status = self._lib.SushiTrack_LoadParamsFromJson(json_path, C.byref(params))
            if status != Status.SUCCESS:
                raise SushiTrackError(
                    f"SushiTrack_LoadParamsFromJson failed: {Status(status).name}")

        for key, value in param_overrides.items():
            if not hasattr(params, key):
                raise AttributeError(f"Unknown SushiTrack_Params field: {key!r}")
            field_type = dict(SushiTrackParams._fields_)[key]
            setattr(params, key, field_type(value).value)
        self._params = params

        handle = c_void_p()
        status = self._lib.SushiTrack_CreateEx(C.byref(params), C.byref(handle))
        if status != Status.SUCCESS or not handle.value:
            raise SushiTrackError(f"SushiTrack_CreateEx failed: {Status(status).name}")
        self._hdl = handle

        self._cap = int(max_tracks)
        self._out = (SushiTrackTrack * self._cap)()

    def update(self, detections: Iterable[Sequence], dt: float = 1.0) -> List[Track]:
        """Process one frame of detections and return the active tracks.

        :param detections: Iterable of ``(x, y, w, h, score, label)`` or
            ``(x, y, w, h, score, label, feat)`` where ``x, y`` are the
            **top-left** corner and ``feat`` is an optional 1-D float sequence
            (appearance embedding). The caller's ``feat`` buffers are only read
            during this call.
        :param dt: Time step. In frame mode (``time_aware=0``) the value is
            ignored internally; in wall-clock mode (``time_aware=1``) it is the
            elapsed seconds since the previous call.
        :returns: A list of :class:`Track` records, sorted by lifecycle
            priority (Tracked < Tentative < Lost < Removed).
        """
        dets = list(detections)
        n = len(dets)
        if self._hdl is None:
            raise SushiTrackError("update() called on a closed tracker")

        arr = (SushiTrackObject * n)() if n else None
        feat_refs = []  # keep feature buffers alive across the C call
        for i, d in enumerate(dets):
            o = arr[i]
            o.x, o.y, o.width, o.height = float(d[0]), float(d[1]), float(d[2]), float(d[3])
            o.score = float(d[4])
            o.label = int(d[5])
            feat = d[6] if len(d) > 6 else None
            if feat is not None and len(feat) > 0:
                buf = (c_float * len(feat))(*[float(f) for f in feat])
                feat_refs.append(buf)
                o.feat = C.cast(buf, POINTER(c_float))
                o.feat_dim = len(feat)
            else:
                o.feat = None
                o.feat_dim = 0

        written = self._lib.SushiTrack_Update(
            self._hdl, arr, n, self._out, self._cap, c_float(dt)
        )
        return [self._to_track(self._out[i]) for i in range(written)]

    @staticmethod
    def _to_track(t: SushiTrackTrack) -> Track:
        feat = None
        if t.feat and t.feat_dim > 0:
            feat = [t.feat[i] for i in range(t.feat_dim)]   # copy; pointer is transient
        return Track(
            track_id=int(t.track_id), x=t.x, y=t.y, width=t.width, height=t.height,
            score=t.score, label=int(t.label),
            state=TrackState(t.state), kinematic_state=KinematicState(t.kinematic_state),
            age=int(t.age), last_seen_frame=int(t.last_seen_frame), lost_time=t.lost_time,
            is_activated=bool(t.is_activated), is_confirmed=bool(t.is_confirmed),
            hits=int(t.hits), frames_since_last_update=int(t.frames_since_last_update),
            maneuver_probability=float(t.maneuver_probability),
            feat=feat,
        )

    def set_log_callback(self, callback) -> None:
        """Register a logging callback ``fn(level: LogLevel, message: str)``.

        Pass ``None`` to unregister. The wrapper is retained so it is not
        garbage-collected while the C side holds it.
        """
        if callback is None:
            self._log_cb = LOG_CALLBACK()
        else:
            def _trampoline(level, message):
                callback(LogLevel(level), message.decode("utf-8", "replace") if message else "")
            self._log_cb = LOG_CALLBACK(_trampoline)
        self._lib.SushiTrack_SetLogCallback(self._log_cb)

    def close(self) -> None:
        """Destroy the native handle. Safe to call more than once."""
        if getattr(self, "_hdl", None):
            self._lib.SushiTrack_Destroy(self._hdl)
            self._hdl = None

    def __enter__(self) -> "SushiTrack":
        return self

    def __exit__(self, *exc) -> None:
        self.close()

    def __del__(self) -> None:
        self.close()
