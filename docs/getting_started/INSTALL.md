# Installing

SushiTrack installs in two ways. Both give the same checkout and the same `st` command, and
neither needs the other: SushiHub is not required to build SushiTrack, and a checkout made
without it joins a SushiHub workspace later without any change.

## Requirements

| Component | Version |
|---|---|
| C++ compiler | C++17 (MSVC 19.20+, GCC 9+, Clang 10+) |
| CMake | ≥ 3.14 |
| Eigen | vendored at `third_party/eigen` |
| SushiLog | the tag `cli/sushistack.deps.toml` pins; `st build` builds its package from a checkout beside this one at `../sushilog` or from the one `SUSHILOG_DIR` names, and reuses it afterwards |
| SushiProfiler | the tag `cli/sushistack.deps.toml` pins; `st build` builds its package the same way, from `../sushiprofiler` or the checkout `SUSHIPROFILER_DIR` names |
| GoogleTest | provided via conda or system package |
| Ninja | provided via conda |
| Python | 3.11 |
| Docker (optional) | Engine ≥ 20.10 with NVIDIA Container Toolkit for CUDA inference |

Python runtime dependencies are defined by `.config/conda/environment.yml` (the source of
truth): `numpy==2.4.4`, `pandas==3.0.2`, `scipy==1.17.1`, `pytest==9.0.3`, `absl-py==2.4.0`, `matplotlib==3.10.9`, `opencv-python==4.13.0.92`, `pillow==12.2.0`, `pycocotools==2.0.11`, `rich==15.0.0`, `tqdm==4.67.3`. PyTorch (`torch`/`torchvision`/`torchaudio`) is installed from the conda `pytorch` channel with `pytorch-cuda=12.1`. The `.config/docker/Dockerfile` installs the same pip set without `sushicore`, which the CLI install resolves, and pins PyTorch to `torch==2.5.1 torchvision==0.20.1 torchaudio==2.5.1` from the cu121 wheel index.

The environment file and the Dockerfile name no `sushicore`: `cli/pyproject.toml` holds the one
pin, and the CLI install resolves it.

Compiler flags: MSVC compiles with `/W3 /O2 /Ot /fp:fast /arch:AVX2 /DNDEBUG`; GCC/Clang compile with `-Wall -O3 -ffast-math -funroll-loops -DNDEBUG` (plus `-mavx2` on x86_64, mirroring the MSVC flag). Library is built `SHARED` with hidden-by-default symbol visibility on every platform; only functions carrying `SUSHITRACK_API` (`include/SushiTrack/c_api/sushitrack_export.hpp`) are exported, via `__declspec(dllexport)`/`dllimport` on Windows and `__attribute__((visibility("default")))` on GCC/Clang. Outputs are unified under `${CMAKE_BINARY_DIR}/bin` and `${CMAKE_BINARY_DIR}/lib`.

## Standalone

The CLI is a small package, built on `sushicore`, the console, theme and config-layering
foundation shared with every other Sushi developer CLI (`sr`, `se`, `sa`, `sb`, `sd`, `hub`).
Clone SushiTrack, fetch its submodules and install the CLI; `st setup` fetches SushiLog and
SushiProfiler at the tags the fragment pins.

```bash
git clone https://github.com/SushiSystems/SushiTrack.git
cd SushiTrack
git submodule update --init
pipx install cli/                 # or: pip install -e cli
st setup
st build
st test
```

`cli/pyproject.toml` declares `sushicore` as an ordinary dependency, next to `typer` and `rich`,
and pip resolves it from PyPI at the version the file names.

## With SushiHub

```bash
hub add sushitrack            # clone it into the workspace
hub install-cli sushitrack    # put st on the path
hub link sushitrack <path>    # adopt a checkout you already have, instead of hub add
```

`hub link` records where the checkout is and changes nothing in it. `hub install` provisions
the toolchain and the libraries of every module in the workspace once. After either route, `st
build` and `st test` are the commands above.

Once installed, `st --help` (or its full-name alias, `sushitrack --help`) lists every
command and `st --version` prints the installed version of `sushitrack-cli`.

## Conda environment

```bash
conda env create -f .config/conda/environment.yml
conda activate sushitrack
```

`.config/conda/environment.yml` installs Python 3.11, CMake, Ninja, GoogleTest, PyTorch from the conda `pytorch` channel (`pytorch-cuda=12.1`), and the remaining pip dependency set (numpy/pandas/scipy/opencv/sushicore/rich/pytest/etc.).

## Docker

```bash
st container build   # builds the image from .config/docker/Dockerfile (CUDA 12.1, Ubuntu 22.04)
st container run     # docker run -it --rm -v $(pwd):/workspace/sushitrack sushitrack bash
```

The Dockerfile pins the same wheel versions as `.config/conda/environment.yml`, except `sushicore`, which it leaves to the CLI install. The project root is bind-mounted at `/workspace/sushitrack`; output files written inside the container are immediately visible on the host. The image installs the CLI with `pip3 install`, which puts `st` and `sushitrack` on `PATH`, so `st` works interactively and as `docker run ... st build` alike.
