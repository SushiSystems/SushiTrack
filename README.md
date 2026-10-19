# SushiTrack

SushiTrack is a multi-object tracking library in C++17. It takes the detections of one frame and
returns tracks with stable identities, using the ByteTrack two-stage association with optional
ReID, Mahalanobis gating, an IMM motion estimator and observation-centric momentum. It is used
through a C API (`include/SushiTrack/c_api/sushitrack_c.h`) or the Python binding in
`bindings/python/`, and builds on Windows and Linux.

## Build

The build needs a C++17 compiler, CMake, Ninja, GoogleTest and Python 3.11;
[docs/getting_started/INSTALL.md](docs/getting_started/INSTALL.md) lists the versions and the
conda environment that provides them. Eigen, TrackEval and YOLOX are git submodules.

```bash
git submodule update --init
pipx install cli/
st build
st test
```

`st` is the developer CLI of this repository. It requires `sushicore` 0.7.0; the install page
says where that comes from.

## Use

After `st build`, with `bindings/python` on `PYTHONPATH`:

```python
from sushitrack import Tracker

tracker = Tracker(config="auto")              # reads sushitrack.json
tracks = tracker.update([(100.0, 100.0, 50.0, 80.0, 0.9, 0)], dt=1.0 / 30.0)
for t in tracks:
    print(t.track_id, t.x, t.y, t.width, t.height)
tracker.close()
```

A detection is `(x, y, width, height, score, label)` with a top-left origin. The C API takes
the same data as `sushitrack_object_t`; see [docs/reference/C_API.md](docs/reference/C_API.md).

## Documentation

[docs/README.md](docs/README.md) is the index of the manual: installing, building, the
architecture, the command line, the configuration parameters and the recorded benchmarks.

## License

SushiTrack is source-available, free for non-commercial use under the PolyForm Noncommercial
License 1.0.0; commercial use needs a licence from Sushi Systems, see
[COMMERCIAL.md](COMMERCIAL.md). [LICENSE](LICENSE) is the binding text. Third-party code and
data keep their own licences, listed in [NOTICE.md](NOTICE.md).
