# Known issues

Next id: KI-18

Defects that stand in the tree. The work that closes each one is in
[Remaining work](../design/REMAINING_WORK.md).

## C ABI

### KI-1 A failed update reads as a frame with no tracks
- Origin: own
- Where: `source/c_api/sushitrack_c.cpp`, `include/SushiTrack/c_api/sushitrack_c.h`
- Cause: `sushitrack_update` and `sushitrack_get_tracklets` return 0 on a caught exception and
  on a rejected argument, and the ABI has no status return or last-error call on them
- Recognise: a frame with detections returns 0 tracks and the log holds an ERROR line
- Workaround: install a log callback and treat an ERROR line during the call as a failure
- Fix: RW-13

### KI-3 A late failure in sushitrack_create_ex hands back a handle that is then dropped
- Origin: own
- Where: `source/c_api/sushitrack_c.cpp`
- Cause: `*out_tracker` is set before the last log line, which can throw; a throwing `Tracker`
  constructor leaks the implementation object
- Recognise: `SUSHITRACK_RUNTIME_ERROR` from `sushitrack_create_ex` with `*out_tracker` not null
- Workaround: none
- Fix: RW-15

## Installing and versions

### KI-4 An environment built from environment.yml cannot run st
- Origin: own
- Where: `.config/conda/environment.yml`, `cli/pyproject.toml`
- Cause: the environment pins `sushicore==0.4.0` and the CLI requires `sushicore>=0.7.0`
- Recognise: pip reports a resolution conflict on sushicore when the CLI is installed
- Workaround: install sushicore 0.7.0 into the environment by hand
- Fix: RW-16

### KI-5 The image build names no source for sushicore 0.7.0
- Origin: own
- Where: `.config/docker/Dockerfile`
- Cause: the image installs the CLI with `pip3 install` and no index or path for sushicore
- Recognise: the image build fails at the CLI install step
- Workaround: none
- Fix: RW-16

### KI-6 Four version numbers disagree
- Origin: own
- Where: `CMakeLists.txt`, `bindings/python/pyproject.toml`, `cli/pyproject.toml`,
  `cli/sushitrack_cli/__init__.py`
- Cause: the library and the binding say 1.0.0, the CLI package 0.1.0 and its `__version__`
  2.0.0, each typed by hand
- Recognise: `st --version` and the library report different numbers
- Workaround: none
- Fix: RW-17

### KI-7 The changelog has a v1.0.0 section that no tag marks
- Origin: own
- Where: `docs/reference/CHANGELOG.md`
- Cause: the section was written for a release that was never tagged or published
- Recognise: `git tag` lists nothing
- Workaround: none
- Fix: RW-2

### KI-8 st doctor never looks for GoogleTest
- Origin: own
- Where: `cli/sushistack.deps.toml`
- Cause: the fragment carries no check command, and says the test targets are off by default
  while `st build` builds them unless `--no-test` is given
- Recognise: `st doctor` passes and `st build` fails at `find_package(GTest)`
- Workaround: `st build --no-test`
- Fix: RW-18

### KI-17 Deploy packages ship ported MIT code without its notice
- Origin: own
- Where: `cli/sushitrack_cli/services/deploy.py`
- Cause: the packages copy the headers, the libraries and the configuration, and no licence file
- Recognise: `package/` holds no `LICENSE` or `NOTICE.md` after `st deploy`
- Workaround: copy `LICENSE`, `NOTICE.md` and `third_party/bytetrack/LICENSE` into the package by hand
- Fix: RW-20

## Repository

### KI-10 234 MB of benchmark annotations are tracked in git
- Origin: own
- Where: `applications/benchmarks/mot_evaluation/data/`
- Cause: the MOT17, MOT20 and DanceTrack sequences were committed beside the evaluator
- Recognise: a clone takes the 234 MB
- Workaround: `git clone --filter=blob:none`
- Fix: RW-4

### KI-11 The ignore files name paths that do not exist and miss ones that do
- Origin: own
- Where: `.gitignore`, `.config/docker/Dockerfile.dockerignore`
- Cause: `.sushitrack_env.json` and `Dockerfile.cli` are named; `build-asan/`, `.worktrees/`
  and `*.egg-info` are not
- Recognise: `git status` lists a worktree or an asan build tree
- Workaround: none
- Fix: RW-1

### KI-12 The submodules and two strings still say 3rdparty
- Origin: own
- Where: `.gitmodules`, `cli/sushitrack_cli/config.py`, `cli/sushitrack_cli/services/evaluate.py`
- Cause: the folder was renamed to `third_party/` and the names were not
- Recognise: `git submodule status` names `3rdparty/...` sections
- Workaround: none
- Fix: RW-1

### KI-13 The layout of the tree departs from repository-layout
- Origin: own
- Where: repository root, `CMakeLists.txt`, `include/SushiTrack/`, `source/`, `tests/`
- Cause: configuration files sit in the root, the root build file holds logic, headers are flat
  and hold more than one type, and the test tree is split by kind instead of mirroring the source
- Recognise: `python tools/testing/check_test_mirror.py .` reports findings
- Workaround: none
- Fix: RW-1

### KI-14 Helper scripts sit inside the CLI package
- Origin: own
- Where: `cli/misc/`
- Cause: three scripts that clean predictions and stage TrackEval data sit beside the CLI
  package instead of being `st` commands or tools
- Recognise: `cli/misc/` holds a `.sh`, a `.bat` and a `.ps1`
- Workaround: none
- Fix: RW-1

## Documentation

### KI-15 A fix message and a dependency description cite the old one-file manual
- Origin: own
- Where: `cli/sushitrack_cli/services/checks.py`, `cli/sushistack.deps.toml`
- Cause: the sections moved to `docs/reference/DATA_LAYOUT.md` and
  `docs/getting_started/INSTALL.md`
- Recognise: the cited section is not in `docs/README.md`
- Workaround: read the two pages named above
- Fix: RW-1

### KI-16 The manual names symbols and paths the source does not hold
- Origin: own
- Where: `docs/architecture/OVERVIEW.md`, `docs/reference/CLI.md`,
  `docs/reference/CONFIGURATION.md`, `docs/guides/TESTING.md`, `docs/CONTRIBUTING.md`
- Cause: the pages were carried over from the old manual without a line-by-line check
- Recognise: `python tools/documentation/check_docs_truth.py .` reports findings
- Workaround: the source is authoritative
- Fix: RW-1
