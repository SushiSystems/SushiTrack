# inference

The YOLOX video pipeline behind `st infer`: it detects objects in a video or a camera stream and
tracks them through `tracker_bridge`.

| File | Holds |
| --- | --- |
| `demo.py` | The command line `st infer` launches |
| `pipeline.py` | The detector, the optional re-identification model and the frame loop |
| `tracker_bridge.py` | The `ctypes` wrapper over `tracker_bridge.dll` or `libtracker_bridge.so` |
| `messages.py` | Its output: progress and results on stdout, problems on stderr |

It needs a `st build` and the Python packages of `.config/conda/environment.yml`. The
[Inference pipeline](../../docs/guides/INFERENCE.md) says how to run it.
