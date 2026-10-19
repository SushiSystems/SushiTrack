# Known issues

Next id: KI-18

Defects that stand in the tree. The work that closes each one is in
[Remaining work](../design/REMAINING_WORK.md).

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
- Cause: the GoogleTest fragment carries no check command
- Recognise: `st doctor` passes and `st build` fails at `find_package(GTest)`
- Workaround: `st build --no-test`
- Fix: RW-18

## Repository

### KI-10 234 MB of benchmark annotations are tracked in git
- Origin: own
- Where: `applications/benchmarks/mot17/data/`, `applications/benchmarks/mot20/data/`,
  `applications/benchmarks/dancetrack/data/`
- Cause: the MOT17, MOT20 and DanceTrack sequences were committed beside the evaluator
- Recognise: a clone takes the 234 MB
- Workaround: `git clone --filter=blob:none`
- Fix: RW-4

### KI-13 The Python binding sits in a root folder the layout does not name
- Origin: own
- Where: `bindings/python/`
- Cause: `repository-layout` names no home for a language binding
- Recognise: `bindings/` beside `cli/` and `applications/` in the root
- Workaround: none
- Fix: RW-3
