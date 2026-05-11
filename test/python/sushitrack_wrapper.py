import ctypes
import numpy as np
import os
from typing import List, Optional

class SushiTrackObject(ctypes.Structure):
    _fields_ = [
        ("x", ctypes.c_float),
        ("y", ctypes.c_float),
        ("width", ctypes.c_float),
        ("height", ctypes.c_float),
        ("score", ctypes.c_float),
        ("label", ctypes.c_int),
        ("feat", ctypes.POINTER(ctypes.c_float)),
        ("feat_dim", ctypes.c_int),
    ]

class SushiTrackTrack(ctypes.Structure):
    _fields_ = [
        ("track_id", ctypes.c_int64),
        ("age", ctypes.c_int64),
        ("last_seen_frame", ctypes.c_int64),
        ("feat", ctypes.POINTER(ctypes.c_float)),
        ("x", ctypes.c_float),
        ("y", ctypes.c_float),
        ("width", ctypes.c_float),
        ("height", ctypes.c_float),
        ("score", ctypes.c_float),
        ("lost_time", ctypes.c_float),
        ("label", ctypes.c_int),
        ("state", ctypes.c_int),
        ("kinematic_state", ctypes.c_int),
        ("feat_dim", ctypes.c_int),
        ("is_activated", ctypes.c_int),
        ("is_confirmed", ctypes.c_int),
        ("hits", ctypes.c_int),
        ("frames_since_last_update", ctypes.c_int),
    ]

class SushiTrackParams(ctypes.Structure):
    _fields_ = [
        ("time_aware", ctypes.c_int),
        ("frame_rate", ctypes.c_int),
        ("track_buffer", ctypes.c_int),
        ("track_thresh", ctypes.c_float),
        ("high_thresh", ctypes.c_float),
        ("match_thresh", ctypes.c_float),
        ("enable_tentative", ctypes.c_int),
        ("enable_fsm", ctypes.c_int),
        ("enable_reid", ctypes.c_int),
        ("enable_mahalanobis", ctypes.c_int),
        
        ("primary_assoc_mode", ctypes.c_int),
        ("fuse_score", ctypes.c_int),
        ("iou_match_thresh", ctypes.c_float),
        ("unconfirmed_iou_thresh", ctypes.c_float),
        ("duplicate_iou_thresh", ctypes.c_float),
        
        ("enable_xyah", ctypes.c_int),
        ("enable_diou", ctypes.c_int),
        ("iou_plus_one", ctypes.c_int),
        ("kalman_type", ctypes.c_int),
        
        ("feature_momentum", ctypes.c_float),
        ("reid_high_thresh", ctypes.c_float),
        ("reid_high_iou_weight", ctypes.c_float),
        ("reid_low_thresh", ctypes.c_float),
        ("reid_low_iou_weight", ctypes.c_float),
        ("reid_recovery_thresh", ctypes.c_float),
        ("max_feature_history", ctypes.c_int),
        
        ("fsm_stationary_vel_thresh", ctypes.c_float),
        ("fsm_maneuver_accel_thresh", ctypes.c_float),
        
        ("tentative_match_thresh", ctypes.c_float),
        ("tentative_confirm_hits", ctypes.c_int),
        ("tentative_max_age", ctypes.c_int),
        
        ("proximity_thresh", ctypes.c_float),
        ("gating_thresh", ctypes.c_float),
        ("motion_cost_weight", ctypes.c_float),
        ("max_area_ratio", ctypes.c_float),
        
        ("tentative_fast_thresh", ctypes.c_float),
        ("tentative_fast_hits", ctypes.c_int),
        ("tentative_slow_thresh", ctypes.c_float),
        ("tentative_slow_hits_offset", ctypes.c_int),
        ("tentative_vslow_hits_offset", ctypes.c_int),
        ("reid_direct_assignment_weight", ctypes.c_float),
        
        ("enable_interaction_context", ctypes.c_int),
        ("interaction_iou_thresh", ctypes.c_float),
        ("interaction_occluder_penalty", ctypes.c_float),
        ("interaction_occluder_validity_frames", ctypes.c_int),
        ("interaction_occluder_validity_seconds", ctypes.c_float),
    ]

class SushiTrackTracker:
    def __init__(self, lib_path: str, params: Optional[SushiTrackParams] = None):
        if not os.path.exists(lib_path):
            raise FileNotFoundError(f"Library not found at {lib_path}")
            
        self.lib = ctypes.CDLL(lib_path)
        
        # Define function signatures
        self.lib.SushiTrack_GetDefaultParams.argtypes = [ctypes.POINTER(SushiTrackParams)]
        self.lib.SushiTrack_GetDefaultParams.restype = None
        
        self.lib.SushiTrack_Create.argtypes = [ctypes.POINTER(SushiTrackParams)]
        self.lib.SushiTrack_Create.restype = ctypes.c_void_p
        
        self.lib.SushiTrack_Destroy.argtypes = [ctypes.c_void_p]
        self.lib.SushiTrack_Destroy.restype = None
        
        self.lib.SushiTrack_Update.argtypes = [
            ctypes.c_void_p,
            ctypes.POINTER(SushiTrackObject),
            ctypes.c_int,
            ctypes.POINTER(SushiTrackTrack),
            ctypes.c_int,
            ctypes.c_float
        ]
        self.lib.SushiTrack_Update.restype = ctypes.c_int
        
        if params is None:
            params = SushiTrackParams()
            self.lib.SushiTrack_GetDefaultParams(ctypes.byref(params))
            
        self.hdl = self.lib.SushiTrack_Create(ctypes.byref(params))
        if not self.hdl:
            raise RuntimeError("Failed to create SushiTrack tracker instance")
            
        self.max_tracks = 1000
        self._out_tracks = (SushiTrackTrack * self.max_tracks)()
        
    def __del__(self):
        if hasattr(self, 'hdl') and self.hdl:
            self.lib.SushiTrack_Destroy(self.hdl)
            self.hdl = None
            
    def update(self, detections: np.ndarray, dt: float = 0.033) -> List[dict]:
        """
        detections: nx6 array (x, y, w, h, score, label)
        returns list of track dicts
        """
        n_objects = len(detections)
        objs = (SushiTrackObject * n_objects)()
        
        for i in range(n_objects):
            objs[i].x = float(detections[i, 0])
            objs[i].y = float(detections[i, 1])
            objs[i].width = float(detections[i, 2])
            objs[i].height = float(detections[i, 3])
            objs[i].score = float(detections[i, 4])
            objs[i].label = int(detections[i, 5])
            objs[i].feat = None
            objs[i].feat_dim = 0
            
        n_tracks = self.lib.SushiTrack_Update(
            self.hdl,
            objs,
            n_objects,
            self._out_tracks,
            self.max_tracks,
            dt
        )
        
        results = []
        for i in range(n_tracks):
            t = self._out_tracks[i]
            results.append({
                "track_id": t.track_id,
                "bbox": [t.x, t.y, t.width, t.height],
                "score": t.score,
                "label": t.label,
                "state": t.state,
                "kinematic_state": t.kinematic_state,
                "hits": t.hits,
                "age": t.age
            })
            
        return results
