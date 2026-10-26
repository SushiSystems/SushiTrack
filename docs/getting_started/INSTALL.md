# Installing

## Requirements

| Component | Version |
|---|---|
| C++ compiler | C++17 (MSVC 19.20+, GCC 9+, Clang 10+) |
| CMake | ≥ 3.14 |
| Eigen | vendored at `third_party/eigen` |
| SushiLog | `v0.2.3`, pinned in `cli/sushistack.deps.toml`; `st build` builds its package from a checkout beside this one at `../sushilog` or from the one `SUSHILOG_DIR` names, and reuses it afterwards |
| SushiProfiler | `v0.3.0`, pinned in `cli/sushistack.deps.toml`; `st build` builds its package the same way, from `../sushiprofiler` or the checkout `SUSHIPROFILER_DIR` names |
| GoogleTest | provided via conda or system package |
| Ninja | provided via conda |
| Python | 3.11 |
| Docker (optional) | Engine ≥ 20.10 with NVIDIA Container Toolkit for CUDA inference |

Python runtime dependencies are defined by `.config/conda/environment.yml` (the source of
truth): `numpy==2.4.4`, `pandas==3.0.2`, `scipy==1.17.1`, `pytest==9.0.3`, `absl-py==2.4.0`, `matplotlib==3.10.9`, `opencv-python==4.13.0.92`, `pillow==12.2.0`, `pycocotools==2.0.11`, `rich==15.0.0`, `sushicore==0.4.0`, `tqdm==4.67.3`. PyTorch (`torch`/`torchvision`/`torchaudio`) is installed from the conda `pytorch` channel with `pytorch-cuda=12.1`. The `.config/docker/Dockerfile` installs the same pip set without `sushicore`, which the CLI install resolves, and pins PyTorch to `torch==2.5.1 torchvision==0.20.1 torchaudio==2.5.1` from the cu121 wheel index.

`.config/conda/environment.yml` pins `sushicore==0.4.0`, while the CLI requires `sushicore>=0.7.0`. An
environment built from the file needs the newer `sushicore` installed by hand, as shown under
[CLI (pipx)](#cli-pipx); the mismatch is in [Known issues](../reference/KNOWN_ISSUES.md).

Compiler flags: MSVC compiles with `/W3 /O2 /Ot /fp:fast /arch:AVX2 /DNDEBUG`; GCC/Clang compile with `-Wall -O3 -ffast-math -funroll-loops -DNDEBUG` (plus `-mavx2` on x86_64, mirroring the MSVC flag). Library is built `SHARED` with hidden-by-default symbol visibility on every platform; only functions carrying `SUSHITRACK_API` (`include/SushiTrack/c_api/sushitrack_export.hpp`) are exported, via `__declspec(dllexport)`/`dllimport` on Windows and `__attribute__((visibility("default")))` on GCC/Clang. Outputs are unified under `${CMAKE_BINARY_DIR}/bin` and `${CMAKE_BINARY_DIR}/lib`.

## CLI (pipx)

The CLI is a small pipx-installed package, built on `sushicore`, the console,
theme and config-layering foundation shared with every other Sushi* developer
CLI (`sr`, `se`, `sa`, `sb`, `sd`, `hub`). Inside a SushiStack workspace,
`hub add sushitrack` installs it automatically; standalone, install it by hand:

```bash
pipx install cli/
```

`cli/pyproject.toml` declares `sushicore>=0.7.0` as an ordinary dependency, next to
`typer>=0.12` and `rich>=13.0`. PyPI carries sushicore 0.1.0 to 0.4.0, so until 0.7.0 is
published `pipx install cli/` cannot resolve that requirement. Until then, install
sushicore from a checkout into one environment, then the CLI beside it:

```bash
pip install -e /path/to/sushicore
pip install -e cli
```

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
