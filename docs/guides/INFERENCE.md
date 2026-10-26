# Inference pipeline

`applications/inference/`:

- `tracker_bridge.py`: `TrackerBridge` dispatches per tracker. `SushiTrack` is driven through the public-API binding ([`bindings/python/sushitrack.py`](../../bindings/python/README.md)), so the C API is consumed in one place and the binding is exercised on every inference frame. The reference trackers (`ByteTrack`, `OCSORT`) have no public C API and stay on the `TrackerRegistry` shim (`create_tracker` / `tracker_process_c` / `delete_tracker` in `tracker_bridge.{dll,so}`, built from `applications/tracker_bridge/tracker_bridge.cpp` by every `st build`). Both paths return the same `{track_id, bbox, score}` shape. On the shim path an empty frame still runs the tracker's update, so tracks age, and `tracker_process_c` returns a status (`TRACKER_BRIDGE_OK`, `TRACKER_BRIDGE_TRUNCATED`, `TRACKER_BRIDGE_INVALID_ARGUMENT` or `TRACKER_BRIDGE_ERROR`) that the bridge raises on for the last two.
- `pipeline.py`: defines `IDetector`, `YOLOXDetector` (loads weights via `yolox.exp.get_exp`, preprocesses with optional legacy normalisation, runs inference, postprocesses with NMS), and `VideoPipeline` (frame loop, detection→tracker hand-off, drawing, optional MOT-format detection logging, stress/no-screen modes).
- `demo.py`: argparse front-end exposing the full YOLOX + tracker flag set. Invoked through `st infer`.

The scripts print through `messages.py`, beside them; they import nothing from the `st` package.

`applications/tracker_bridge/tracker_bridge.h` declares the C entry points for ctypes and exports
them with `TRACKER_BRIDGE_API`. That macro has no `dllimport` branch: `tracker_bridge` is a DLL
that ctypes loads and that nothing links against.

Example end-to-end run:

```bash
st build
st infer \
    -n yolox-x -c third_party/weight/mot17x.pth.tar \
    --source third_party/video/MOT17-04.mp4 \
    --tracker SushiTrack --num-classes 1 --legacy
```

The pipeline auto-detects MOT17 checkpoints by filename and sets `tsize=(800,1440)` accordingly.
