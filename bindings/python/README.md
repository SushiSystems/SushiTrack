# SushiTrack Python Bindings

Python ctypes bindings for SushiTrack, the real-time multi-object tracking engine.

## Installation

```bash
pip install .
```

## Quick Start

```python
from sushitrack import Tracker

tracker = Tracker(config_path="sushitrack.json")
dets = [(100.0, 100.0, 50.0, 80.0, 0.9, 0)]  # (x, y, w, h, score, label)
tracks = tracker.update(dets, dt=1.0 / 30.0)
for t in tracks:
    print(t.track_id, t.x, t.y, t.width, t.height)
tracker.close()
```
