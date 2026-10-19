# Inference pipeline

`tests/regression/inference/`:

- `tracker_bridge.py`: `TrackerBridge` dispatches per tracker. `SushiTrack` is driven through the public-API binding ([`bindings/python/sushitrack.py`](../../bindings/python/README.md)), so the C API is consumed in one place and the binding is exercised on every inference frame. The reference trackers (`ByteTrack`, `OCSORT`) have no public C API and stay on the `TrackerRegistry` shim (`create_tracker` / `tracker_process_c` / `delete_tracker` in `tracker_bridge.{dll,so}`, built from `tests/regression/TrackerRegistry.cpp` when `-DBUILD_REGRESSION_TEST=ON`). Both paths return the same `{track_id, bbox, score}` shape.
- `pipeline.py`: defines `IDetector`, `YOLOXDetector` (loads weights via `yolox.exp.get_exp`, preprocesses with optional legacy normalisation, runs inference, postprocesses with NMS), and `VideoPipeline` (frame loop, detection→tracker hand-off, drawing, optional MOT-format detection logging, stress/no-screen modes).
- `demo.py`: argparse front-end exposing the full YOLOX + tracker flag set. Invoked through `st infer`.

`pipeline.py` appends `cli/` to `sys.path` before it imports `sushitrack_cli.console`. `demo.py` already does that when it imports the module, but `pipeline.py` is also importable on its own, so it repeats the step.

`tests/regression/TrackerRegistry.h` declares the C entry points of the shim for ctypes and exports them with `EXPORT_API`. That macro is a smaller copy of the pattern in `SushiTrack/c_api/sushitrack_export.hpp`, not a reuse of `SUSHITRACK_API`. `SUSHITRACK_API` is gated by `SUSHITRACK_EXPORTS`, which is specific to the `sushitrack` target, and `tracker_bridge` is a separate DLL that ctypes loads with `dlopen()` and nothing links against, so it needs no `dllimport` branch.

Example end-to-end run:

```bash
st build
st infer \
    -n yolox-x -c third_party/weight/mot17x.pth.tar \
    --source third_party/video/MOT17-04.mp4 \
    --tracker SushiTrack --num-classes 1 --legacy
```

The pipeline auto-detects MOT17 checkpoints by filename and sets `tsize=(800,1440)` accordingly.
