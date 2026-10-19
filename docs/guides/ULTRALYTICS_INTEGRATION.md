# Ultralytics integration

SushiTrack can replace ByteTrack/BoT-SORT as the tracker behind an Ultralytics
YOLO model (`model.track(...)`) so that any Ultralytics detector (YOLOv8/v9/v10/11,
RT-DETR, or a custom-trained checkpoint) feeds its detections into SushiTrack
per frame. The integration is **non-invasive**: you do not fork or modify the
`ultralytics` package. You build the SushiTrack shared library, wrap its C API
with a thin `ctypes` adapter that satisfies the Ultralytics tracker contract,
and register that adapter through Ultralytics' public prediction callbacks.

> This page documents the porting recipe only. No adapter code is shipped in
> this repository; the snippets below are the reference implementation you copy
> into your Ultralytics project.

## How Ultralytics tracking works

`model.track()` installs two callbacks (`ultralytics/trackers/track.py`):

- `on_predict_start`: instantiates one tracker object per batch stream from
  `TRACKER_MAP[cfg.tracker_type]` and stores them on `predictor.trackers`.
- `on_predict_postprocess_end`: for each frame, takes the detection
  `Boxes` (`predictor.results[i].boxes`), calls `tracker.update(det, img)`, and
  expects back an `(N, 7+)` array whose **last column is the source detection
  index**. Ultralytics slices the results by that index and overwrites the boxes
  with columns `[:-1]`, laid out as `[x1, y1, x2, y2, track_id, conf, cls]`.

Any object exposing a compatible `update(results, img=None)` method can take the
place of `BYTETracker`. That is the single seam SushiTrack plugs into.

## Step 1: get the library and binding into your project

You have two options. For an Ultralytics project living **outside** this
repository, the deploy package is self-contained: it needs no build tree or
environment variables on the consumer side.

**Option A: deploy package (recommended for external projects).** Produce a
portable Python package and copy it next to your Ultralytics code:

```bash
st build --deploy python      # builds + assembles package/
# then, in your Ultralytics project:
cp -r /path/to/sushitrack/package/sushitrack   ./sushitrack      # bundles the .py + native lib
```

The bundled `sushitrack/` directory carries the binding and the native library
together, so importing `from sushitrack import Tracker` works with no build tree
and no `SUSHITRACK_LIB` variable on the consumer side. See
[Deploy packages](DEPLOY_PACKAGES.md).

**Option B: in-repo binding.** When you work inside this repository, build the
library and import the binding directly:

```bash
st build           # produces build/bin/<cfg>/sushitrack.{dll,so}
# bindings/python/sushitrack.py then finds the library under build/ automatically
```

Either way the adapter uses the shipped Python binding
([`bindings/python/sushitrack.py`](../../bindings/python/README.md)), which wraps the **public C
API** (`include/SushiTrack/c_api/sushitrack_c.h`). It does not use the regression `tracker_bridge`, whose
reduced struct omits the state/feature/kinematic fields.

## Step 2: implement the Ultralytics tracker contract

The adapter converts Ultralytics detections into SushiTrack detections, calls
`Tracker.update`, and emits the `(N, 7)` array Ultralytics expects, plus the
detection-index column.

Two conversions matter:

- **Coordinates.** Ultralytics `Boxes.xywh` is **centre-based** (`cx, cy, w, h`);
  SushiTrack consumes **top-left** `x, y, w, h`. Use `Boxes.xyxy` and derive
  `x = x1`, `y = y1`, `w = x2 - x1`, `h = y2 - y1` to avoid sign mistakes.
- **Detection index.** The C API returns a filtered/Kalman-smoothed box and does
  **not** echo which input detection produced each track. Ultralytics needs that
  index to slice masks/probs. Recover it by greedy IoU-matching each output
  track box against the input detections of the current frame.

```python
import numpy as np
from sushitrack import Tracker, TrackState     # bindings/python/sushitrack.py

class SushiTrackAdapter:
    def __init__(self, frame_rate=30, **overrides):
        # overrides are sushitrack_params_t fields, e.g. enable_tentative=1, iou_type=1
        self.tracker = Tracker(frame_rate=frame_rate, **overrides)
        self.dt = 1.0 / float(frame_rate)

    def update(self, results, img=None):
        xyxy = results.xyxy.cpu().numpy() if hasattr(results.xyxy, "cpu") else np.asarray(results.xyxy)
        conf = np.asarray(results.conf)
        cls  = np.asarray(results.cls)

        dets = [(x1, y1, x2 - x1, y2 - y1, float(c), int(k))
                for (x1, y1, x2, y2), c, k in zip(xyxy, conf, cls)]
        tracks = self.tracker.update(dets, dt=self.dt)

        rows = []
        for t in tracks:
            if t.state not in (TrackState.TRACKED, TrackState.TENTATIVE):
                continue
            box = np.array([t.x, t.y, t.x + t.width, t.y + t.height])
            j = self._best_iou_idx(box, xyxy)            # recover detection index
            if j < 0:
                continue
            rows.append([box[0], box[1], box[2], box[3], t.track_id, t.score, t.label, j])
        return np.asarray(rows, dtype=np.float32) if rows else np.empty((0, 8), np.float32)

    @staticmethod
    def _best_iou_idx(box, dets, thr=0.3):
        if len(dets) == 0:
            return -1
        xx1 = np.maximum(box[0], dets[:, 0]); yy1 = np.maximum(box[1], dets[:, 1])
        xx2 = np.minimum(box[2], dets[:, 2]); yy2 = np.minimum(box[3], dets[:, 3])
        w = np.clip(xx2 - xx1, 0, None); h = np.clip(yy2 - yy1, 0, None)
        inter = w * h
        area_b = (box[2]-box[0]) * (box[3]-box[1])
        area_d = (dets[:,2]-dets[:,0]) * (dets[:,3]-dets[:,1])
        iou = inter / (area_b + area_d - inter + 1e-9)
        j = int(iou.argmax())
        return j if iou[j] >= thr else -1
```

Source: not compiled

## Step 3: register the adapter with `model.track()`

No package fork is required: install the adapter through the same callbacks
Ultralytics itself uses. The binding finds the shared library automatically:
beside the module when you used the deploy package (Option A), or under `build/`
when you import in-repo (Option B); `SUSHITRACK_LIB` overrides both:

```python
from ultralytics import YOLO
from sushitrack_adapter import SushiTrackAdapter

def on_predict_start(predictor, persist=False):
    bs = predictor.dataset.bs
    predictor.trackers = [
        SushiTrackAdapter(frame_rate=30, enable_tentative=1, iou_type=1)
        for _ in range(bs)
    ]

model = YOLO("yolo11n.pt")
model.add_callback("on_predict_start", on_predict_start)

for result in model.track(source="video.mp4", stream=True, persist=True):
    boxes = result.boxes
    if boxes.id is not None:
        ids   = boxes.id.int().tolist()
        xyxy  = boxes.xyxy.tolist()
        # ... your downstream logic ...
```

Source: not compiled

Because Ultralytics' built-in `on_predict_postprocess_end` already calls
`predictor.trackers[i].update(...)` and interprets the returned array, supplying
your own `on_predict_start` is enough to swap the tracker. (Alternatively, fork
the package and add `"sushitrack": SushiTrackAdapter` to `TRACKER_MAP` with a
matching `sushitrack.yaml`; the callback route above avoids touching vendored
code and is the recommended path.)

## Notes and caveats

- **Top-left vs. centre coordinates** are the most common integration bug;
  always derive SushiTrack boxes from `xyxy`.
- **Detection-index recovery** via IoU is robust when the Kalman correction is
  small; for fast motion raise `max_tracks` and lower the IoU floor rather than
  asserting a 1:1 mapping. Tracks with no input overlap (pure Kalman coasting
  during a missed frame) are intentionally dropped from the Ultralytics output
  for that frame, exactly as the built-in trackers do.
- **ReID is off in this recipe.** SushiTrack's appearance path expects an
  external embedding per detection; Ultralytics detection models do not produce
  one. To enable `enable_reid=1`, run an embedding extractor (e.g. the OSNet path
  used by the YOLOX inference pipeline) and append the 1-D embedding as a 7th
  element of each detection tuple passed to `update`. Without it the tracker uses
  the geometry fallback.
- **Class handling.** SushiTrack associates across all detections regardless of
  `label`; pass `classes=[...]` to `model.track()` if you need per-class
  filtering upstream, or partition detections before `update()`.
- **Stateful streaming.** Create the tracker once per stream and reuse the
  handle across frames (as above). Re-creating it per frame resets all IDs.
