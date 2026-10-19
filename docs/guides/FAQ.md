# Frequently asked questions

This page answers the questions a developer meeting SushiTrack for the first time tends to ask:
what it is, what it needs, how to build and call it, where it runs, what it leaves to the caller
and how it is licensed. Each answer links to the manual page that holds the detail.

## What is SushiTrack?

SushiTrack is a multi-object tracking library in C++17. It takes the detections of one frame and
returns tracks with stable identities. A detection is a box with a top-left origin, a score and a
label: `(x, y, width, height, score, label)`.

The pipeline follows the ByteTrack two-stage association: detections at or above `track_thresh`
are matched first, then the tracks left over try the low-score detections. Appearance ReID,
Mahalanobis gating, the NSA and IMM motion estimators, observation-centric momentum and
observation-centric re-update are optional and off in the compiled defaults. The
[architecture overview](../architecture/OVERVIEW.md) describes each stage.

The repository also holds a Python binding, the `st` developer CLI, a MOTChallenge and DanceTrack
benchmark and a YOLOX video inference pipeline.

## What do I need to build SushiTrack?

A C++17 compiler, CMake 3.14 or later, Ninja, GoogleTest and Python 3.11. `.config/conda/environment.yml`
defines a conda environment that installs Python, CMake, Ninja, GoogleTest and the Python
packages the evaluator and the inference pipeline use:

```bash
conda env create -f .config/conda/environment.yml
conda activate sushitrack
```

Eigen, TrackEval and YOLOX are git submodules, so run `git submodule update --init` after
cloning. Docker is optional; CUDA inference in the container needs Engine 20.10 or later with the
NVIDIA Container Toolkit. [Installing](../getting_started/INSTALL.md) lists every version.

## How do I install the `st` command?

`st` is the developer CLI of this repository, also installed as `sushitrack`. It is a Python
package in `cli/`, installed with pipx:

```bash
pipx install cli/
```

`cli/pyproject.toml` requires `sushicore>=0.7.0`, and PyPI carries sushicore 0.1.0 to 0.4.0.
Until 0.7.0 is published, `pipx install cli/` cannot resolve that requirement. Install sushicore
from a checkout into one environment with `pip install -e /path/to/sushicore`, then the CLI beside
it with `pip install -e cli`. The conda environment pins `sushicore==0.4.0`, so an environment
built from `.config/conda/environment.yml` needs the same step; see
[Installing](../getting_started/INSTALL.md#cli-pipx).

`st --help` lists every command and `st --version` prints the installed version of
`sushitrack-cli`.

## How do I build and test SushiTrack?

```bash
st build
st test
```

`st build` configures and compiles the library and every test target; `st build --no-test`
compiles the library only. The build type is `release` unless `--type` names `debug`,
`relwithdebinfo` or `minsizerel`. The library lands at
`build/bin/{Release,Debug}/sushitrack.{dll,so}`.

`st test` runs the small and medium tests together. `st test --label small` runs one size. [Building](../getting_started/BUILD.md) holds
the CMake options and the [command line reference](../reference/CLI.md) holds every flag.

## How do I use SushiTrack from Python?

After `st build`, put `bindings/python` on `PYTHONPATH`. The module finds the library under
`build/` by itself.

```python
from sushitrack import Tracker

tracker = Tracker(config="auto")              # reads sushitrack.json
tracks = tracker.update([(100.0, 100.0, 50.0, 80.0, 0.9, 0)], dt=1.0 / 30.0)
for t in tracks:
    print(t.track_id, t.x, t.y, t.width, t.height)
tracker.close()
```

Source: not compiled

`bindings/python/sushitrack.py` is a `ctypes` binding with no dependency outside the standard
library. For a project outside this repository, `st build --deploy python` assembles `package/`
with the binding and the native library side by side; copy its `sushitrack/` directory into your
source tree. The [binding README](../../bindings/python/README.md) lists the constructor arguments
and the order in which the module looks for the library, and
[Deploy packages](DEPLOY_PACKAGES.md) shows the package layout.

## How do I use SushiTrack from a C or C++ project?

Through the C API in `include/SushiTrack/c_api/sushitrack_c.h`, linked against the compiled `sushitrack`
shared library. The header includes `SushiTrack/c_api/sushitrack_export.hpp`, which holds preprocessor
macros only, and nothing else from the library, so a C consumer needs no Eigen and no C++ header.
Fill a `sushitrack_params_t` with `sushitrack_get_default_params`, create the tracker with
`sushitrack_create_ex`, and call `sushitrack_update` once per frame with an array of
`sushitrack_object_t`; it returns a `sushitrack_status_t` and writes the number of
`sushitrack_track_t` records through its last argument. The
[C API reference](../reference/C_API.md) has the status codes and the track fields.

`st build --deploy cpp` assembles `package/` with the headers, the library, a default
`sushitrack.json` and `example/example.c` with its `CMakeLists.txt`. Add `package/include` to the
include path and link against `package/lib/sushitrack.lib` on Windows or
`package/lib/libsushitrack.so` on Linux. [Deploy packages](DEPLOY_PACKAGES.md) covers how the
runtime library is found at load time.

## Which platforms and compilers does SushiTrack support?

SushiTrack builds on Windows and Linux. The compilers listed are MSVC 19.20 or later, GCC 9 or
later and Clang 10 or later. The library is built as a shared library with hidden-by-default
symbol visibility on every platform; only functions carrying `SUSHITRACK_API` are exported.

A deploy package is platform-specific: `st build --deploy` copies the artefacts of the operating
system it runs on, a `.dll` and `.lib` on Windows or a `.so` on Linux. There is no
cross-compilation, so shipping both platforms means running the deploy once on each.

`st container build` builds an image from `.config/docker/Dockerfile` on a CUDA 12.1, Ubuntu 22.04 base, and
`st container run` starts it with the repository mounted at `/workspace/sushitrack`. See
[Installing](../getting_started/INSTALL.md#docker).

## How do I change the tracker's parameters?

Edit `sushitrack.json`. The library parses the file itself, so a compiled build is retuned with no
recompilation. It looks for the file in the `SUSHITRACK_CONFIG` environment variable first, then
searches upward from the working directory for `sushitrack.json`. A key absent from the file keeps
its compiled default.

The shipped `sushitrack.json` enables the tentative lifecycle, observation-centric momentum and
observation-centric re-update. A caller that fills `sushitrack_params_t` in code and passes it to
the create call bypasses the file and gets the compiled defaults, where those three are off. In
Python, `Tracker(config="auto")` resolves the file through the library, and keyword overrides win
over the file. [Configuration](../reference/CONFIGURATION.md) lists every parameter with its
default.

## How does SushiTrack compare with ByteTrack and OC-SORT?

[Benchmarks](../reference/BENCHMARKS.md) records two runs on the MOT17 and MOT20 train sets and on
DanceTrack against the bundled ByteTrack and OC-SORT references. In the linear Kalman run
(`kalman_type=0`, with the tentative lifecycle, OCM and ORU on and ReID off) the overall figures
are:

| Tracker | HOTA | Latency (µs/frame) |
| --- | --- | --- |
| SushiTrack | 45.7 % | 57 |
| ByteTrack | 40.8 % | 103 |
| OC-SORT | 44.3 % | 223 |

On DanceTrack alone, OC-SORT scores 45.6 % HOTA in that run and SushiTrack 44.9 %. The second run
changes only `kalman_type` to 2, the IMM estimator: SushiTrack reaches 45.8 % overall HOTA at
218 µs per frame. Latency is the mean per-frame `update()` time, single-threaded, excluding I/O.
The date, the commit and the machine of the runs were not recorded.

`st eval --compare` runs all three trackers side by side and prints the same metrics table; see
the [evaluation pipeline](EVALUATION.md).

## How does SushiTrack relate to the other Sushi Systems parts?

The `st` CLI is built on `sushicore`, the Sushi Systems package that gives every Sushi CLI (`sr`,
`se`, `sa`, `sb`, `sd`, `hub`) its console, configuration layering and build driver. `st setup`,
`st doctor`, `st link` and `st unlink` come from `sushicore` and behave the same in each of them.

Inside a SushiStack workspace, `hub add sushitrack` installs the CLI, and `st link` and
`st unlink` register a checkout in a workspace or remove it. `cli/sushistack.deps.toml` is the
SushiStack dependency fragment; it names GoogleTest, the only port SushiTrack does not vendor.
`st setup` provisions what that fragment lists.

The library itself links Eigen and JSON for Modern C++. The Python binding depends on the compiled
library and on nothing else in the repository.

## What does SushiTrack not do?

The manual states these limits:

- It does not compute appearance embeddings. With `enable_reid=1`, each detection carries an
  external embedding as a seventh tuple element.
- It associates across all detections regardless of `label`. Partition the detections before
  `update` if you need per-class tracking.
- A single tracker instance is not designed for concurrent `update` calls. Construct one instance
  per stream.
- The C API does not report which input detection produced each track.
- No Ultralytics adapter is shipped. [Ultralytics integration](ULTRALYTICS_INTEGRATION.md) is a
  recipe whose snippets you copy into your own project.
- No built Docker image is published.

Contributions from outside Sushi Systems are not accepted yet. Recorded defects are in
[Known issues](../reference/KNOWN_ISSUES.md).

## How is SushiTrack licensed?

SushiTrack is source-available under the PolyForm Noncommercial License 1.0.0, free for
non-commercial use. [`LICENSE`](../../LICENSE) is the binding text and lists the permitted
purposes: personal, non-commercial use, and use by the non-commercial organisations it names.

Any commercial purpose needs a separate, paid licence from Sushi Systems. That includes use inside
a company, use in paid client work, and use in a product or service that is sold. To ask for one,
write to hello@sushisystems.io with what you want to build and who will use it; see
[`COMMERCIAL.md`](../../COMMERCIAL.md).

Third-party code keeps its own licence, listed in [`NOTICE.md`](../../NOTICE.md). The MOT17,
MOT20 and DanceTrack sequences are not in the repository; each machine downloads them, under their
own licences, as the [Data layout](../reference/DATA_LAYOUT.md) describes.
