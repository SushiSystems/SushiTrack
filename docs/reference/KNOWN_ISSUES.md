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

### KI-8 st doctor never looks for GoogleTest
- Origin: own
- Where: `cli/sushistack.deps.toml`
- Cause: the GoogleTest fragment carries no check command
- Recognise: `st doctor` passes and `st build` fails at `find_package(GTest)`
- Workaround: `st build --no-test`
- Fix: RW-18

## Repository

### KI-10 A clone downloads about 45 MiB of benchmark annotations that the tree no longer holds
- Origin: own
- Where: `applications/benchmarks/`, in the git history of each benchmark's data folder
- Cause: the MOT17, MOT20 and DanceTrack sequences were committed before they left git, and the
  history keeps them
- Recognise: a full clone fetches 326 annotation blobs, about 45 MiB packed and 164 MiB unpacked
- Workaround: `git clone --filter=blob:none`
- Fix: none, the owner kept the history when the data left git
