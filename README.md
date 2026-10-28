# SushiTrack

SushiTrack is a multi-object tracking library in C++17 for people who turn per-frame detections
into tracks with stable identities. It uses the ByteTrack two-stage association with optional
ReID, Mahalanobis gating, an IMM motion estimator and observation-centric momentum, and it is used
through a C API (`include/SushiTrack/c_api/sushitrack_c.h`) or the Python binding in
`bindings/python/`.

After `st build`, with `bindings/python` on `PYTHONPATH`:

```python
from sushitrack import Tracker

tracker = Tracker(config="auto")              # reads sushitrack.json
tracks = tracker.update([(100.0, 100.0, 50.0, 80.0, 0.9, 0)], dt=1.0 / 30.0)
for t in tracks:
    print(t.track_id, t.x, t.y, t.width, t.height)
tracker.close()
```

Source: not compiled

A detection is `(x, y, width, height, score, label)` with a top-left origin. The C API takes
the same data as `sushitrack_object_t`; see [docs/reference/C_API.md](docs/reference/C_API.md).

## Build

The build needs a C++17 compiler, CMake, Ninja, GoogleTest, Python 3.11, and the SushiLog and
SushiProfiler packages that `st setup` fetches;
[docs/getting_started/INSTALL.md](docs/getting_started/INSTALL.md) lists the versions. Eigen,
TrackEval and YOLOX are git submodules.

```bash
git submodule update --init
pipx install cli/
st setup
st build
st test
```

With SushiHub: `hub add sushitrack` and `hub install-cli sushitrack`, or `hub link sushitrack
<path>` for a checkout you already have.

## Status

The library, its C API, the Python binding, the test suites and the MOT17, MOT20 and DanceTrack
benchmarks under `applications/benchmarks/` build and run on Windows and Linux; CI builds and
tests both, and runs the small tests under ASan and UBSan. The benchmark sequences are not in git; [the data layout](docs/reference/DATA_LAYOUT.md)
names their sources. The version numbers of the library, the binding and the CLI disagree.
[docs/design/REMAINING_WORK.md](docs/design/REMAINING_WORK.md)
holds the backlog and [docs/reference/KNOWN_ISSUES.md](docs/reference/KNOWN_ISSUES.md) the open
defects.

## Documentation

[docs/README.md](docs/README.md) is the index of the manual: installing, building, the
architecture, the command line, the configuration parameters and the recorded benchmarks.
Contributors start at [docs/CONTRIBUTING.md](docs/CONTRIBUTING.md).

## Licence

SushiTrack is source-available, free for non-commercial use under the PolyForm Noncommercial
License 1.0.0; commercial use needs a licence from Sushi Systems, see
[COMMERCIAL.md](COMMERCIAL.md). [LICENSE](LICENSE) is the binding text. Third-party code keeps
its own licence, listed in [NOTICE.md](NOTICE.md).
