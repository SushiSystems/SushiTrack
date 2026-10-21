# Remaining work

Next id: RW-26

The single backlog of this repository. Defects that stand in the tree are in
[Known issues](../reference/KNOWN_ISSUES.md); each one names the entry here that closes it.

## Decisions waiting for the owner

### RW-3 Decide where the Python binding lives
- State: decision
- Done when: SushiSkills' `repository-layout` names a home for a language binding and
  `bindings/python/` sits there
- Where: `bindings/python/`

The Python binding sits in a root folder the layout does not name. `bindings/` beside `cli/` and
`applications/` in the root. Cause: `repository-layout` names no home for a language binding.

### RW-5 Decide which of AGENTS.md and CLAUDE.md is the source of the agent instructions
- State: decision
- Done when: one file holds the directives and the other points at it

### RW-6 Decide whether contributions from outside are accepted
- State: decision
- Done when: the owner has named the contributor agreement, or ruled contributions out

### RW-7 Decide whether the SushiTrack migration is closed
- State: decision
- Done when: `docs/design/SUSHITRACK_MIGRATION/README.md` carries a `Shipped` status, or a phase
  with work left
- Source: `docs/design/SUSHITRACK_MIGRATION/README.md`

## Tracker

### RW-10 Suppress duplicates within the tracked pool
- State: unmeasured
- Done when: a benchmark run shows `remove_duplicate_tracklets` over the tracked pool drops the
  same-object duplicate identities of dense scenes without losing HOTA

### RW-11 Gate the IMM on the mixture innovation covariance
- State: unmeasured
- Done when: a benchmark run compares the gate on the mode-weighted covariance with the gate
  on the collapsed 8-D estimate

### RW-12 Run the coordinated-turn update only under a manoeuvre
- State: unmeasured
- Done when: a benchmark run shows the IMM cost per frame falls with tracking output unchanged

## Inference

### RW-23 Run the inference application on SushiAI and OpenCV
- State: open
- Done when: `st infer` detects with a SushiAI model and reads and draws frames with OpenCV,
  with no PyTorch or YOLOX import left in `applications/inference/`

## Continuous integration

### RW-24 Run the MOT benchmarks in the nightly workflow
- State: blocked
- Blocked by: external: no runner holds the MOT17, MOT20 and DanceTrack sequences, which are not in git
- Done when: `nightly.yml` runs `st eval` on a runner that holds the sequences and records the figures
- Where: `.github/workflows/nightly.yml`

## Install and release

### RW-22 Release v0.1.0 with the inference on SushiAI and OpenCV
- State: blocked
- Blocked by: RW-17, RW-23
- Done when: the `chore(release): v0.1.0` commit carries the tag `v0.1.0`

### RW-16 Take sushicore from one source in every install path
- State: open
- Done when: `.config/conda/environment.yml`, the Dockerfile and `cli/pyproject.toml` name the same sushicore
  version and the same source
- Where: `.config/docker/Dockerfile`

The image build names no source for sushicore 0.7.0. The image build fails at the CLI install step.
Cause: the image installs the CLI with `pip3 install` and no index or path for sushicore.

### RW-17 Generate every version number from CMakeLists.txt
- State: open
- Done when: the binding and the CLI read the version `project()` declares, and
  `__version__ = "2.0.0"` is gone
- Where: `CMakeLists.txt`, `bindings/python/pyproject.toml`, `cli/pyproject.toml`,
  `cli/sushitrack_cli/__init__.py`

Four version numbers disagree. `st --version` and the library report different numbers. Cause: the
library and the binding say 1.0.0, the CLI package 0.1.0 and its `__version__` 2.0.0, each typed by
hand.

### RW-18 Check for GoogleTest in st doctor
- State: open
- Done when: the GoogleTest fragment in `cli/sushistack.deps.toml` carries a check command
  and describes the default `st build` truly

## Documentation

### RW-19 Record the machine, commit and date of the next benchmark run
- State: unmeasured
- Done when: `docs/reference/BENCHMARKS.md` names them beside every table

## Documentation architecture

### RW-25 Write the About line of each design topic by hand
- State: open
- Done when: each topic has an `**About:**` line of one sentence written for a cold reader

The topic model generated the lines from the first sentence of each document or from its title.
