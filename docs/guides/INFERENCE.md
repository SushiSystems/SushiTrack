# Inference pipeline

`tests/regression/inference/`:

- `tracker_bridge.py`: `TrackerBridge` dispatches per tracker. `SushiTrack` is driven through the public-API binding ([`bindings/python/sushitrack.py`](../../bindings/python/README.md)), so the C API is consumed in one place and the binding is exercised on every inference frame. The reference trackers (`ByteTrack`, `OCSORT`) have no public C API and stay on the `TrackerRegistry` shim (`create_tracker` / `tracker_process_c` / `delete_tracker` in `tracker_bridge.{dll,so}`, built from `tests/regression/TrackerRegistry.cpp` when `-DBUILD_REGRESSION_TEST=ON`). Both paths return the same `{track_id, bbox, score}` shape.
- `pipeline.py`: defines `IDetector`, `YOLOXDetector` (loads weights via `yolox.exp.get_exp`, preprocesses with optional legacy normalisation, runs inference, postprocesses with NMS), and `VideoPipeline` (frame loop, detection→tracker hand-off, drawing, optional MOT-format detection logging, stress/no-screen modes).
- `demo.py`: argparse front-end exposing the full YOLOX + tracker flag set. Invoked through `st infer`.

Example end-to-end run:

```bash
st build
st infer \
    -n yolox-x -c third_party/weight/mot17x.pth.tar \
    --source third_party/video/MOT17-04.mp4 \
    --tracker SushiTrack --num-classes 1 --legacy
```

The pipeline auto-detects MOT17 checkpoints by filename and sets `tsize=(800,1440)` accordingly.
