import ctypes
import os
import platform
from ctypes import c_float, c_int, c_void_p, Structure, POINTER

class CDetection(Structure):
    _fields_ = [
        ("x", c_float),
        ("y", c_float),
        ("w", c_float),
        ("h", c_float),
        ("score", c_float),
        ("label", c_int)
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
        self.lib = self._load_library(build_dir)
        self._setup_argtypes()
        
        name_bytes = tracker_name.encode('utf-8')
        self.tracker_ptr = self.lib.create_tracker(name_bytes)
        if not self.tracker_ptr:
            raise RuntimeError(f"Failed to create tracker: {tracker_name}")
            
    def _load_library(self, build_dir: str):
        # find the DLL/SO
        system = platform.system()
        lib_name = "tracker_bridge.dll" if system == "Windows" else "libtracker_bridge.so"
        
        # Look in standard cmake output directories
        search_paths = [
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

    def update(self, detections: list):
        """
        detections format: list of dicts or list of lists: [x, y, w, h, score, label]
        returns list of dicts: [{'track_id': id, 'bbox': [x,y,w,h], 'score': s}]
        """
        num_dets = len(detections)
        if num_dets == 0:
            return []
            
        det_array = (CDetection * num_dets)()
        for i, det in enumerate(detections):
            # assume det is [x, y, w, h, score, class_id]
            det_array[i].x = det[0]
            det_array[i].y = det[1]
            det_array[i].w = det[2]
            det_array[i].h = det[3]
            det_array[i].score = det[4]
            det_array[i].label = int(det[5])
            
        max_results = max(num_dets * 2, 100) # safe buffer
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
        if hasattr(self, 'tracker_ptr') and self.tracker_ptr:
            if hasattr(self, 'lib') and self.lib:
                self.lib.delete_tracker(self.tracker_ptr)
