# Remaining work

Next id: RW-20

The single backlog of this repository. Defects that stand in the tree are in
[Known issues](../reference/KNOWN_ISSUES.md); each one names the entry here that closes it.

## Next

- RW-1 Every finding of the shared checkers in this tree is closed by it

## Decisions waiting for the owner

### RW-2 Decide whether v1.0.0 was a release
- State: decision
- Done when: the commit is tagged `v1.0.0`, or the changelog section is renamed
- Source: `docs/agent/2026_10_05_ESTATE_AUDIT/REPORT.md`

### RW-3 Decide where the Python binding lives
- State: decision
- Done when: SushiSkills' `repository-layout` names a home for a language binding and
  `bindings/python/` sits there

### RW-4 Decide whether the benchmark data leaves git
- State: decision
- Done when: the MOT and DanceTrack sequences are fetched on demand or kept in Git LFS, or the
  owner rules that they stay tracked

### RW-5 Decide which of AGENTS.md and CLAUDE.md is the source of the agent instructions
- State: decision
- Done when: one file holds the directives and the other points at it

### RW-6 Decide whether contributions from outside are accepted
- State: decision
- Done when: the owner has named the contributor agreement, or ruled contributions out

### RW-7 Decide whether the SushiTrack migration is closed
- State: decision
- Done when: `docs/design/SUSHITRACK_MIGRATION.md` carries a `Shipped` status, or a phase
  with work left
- Source: `docs/design/SUSHITRACK_MIGRATION.md` §5

## Repository

### RW-1 Align the repository with SushiSkills
- State: open
- Done when: `python tools/check_all.py .` prints nothing, and `st test --label small --label
  medium` passes
- Source: `docs/agent/2026_10_19_SKILL_ALIGNMENT/PLAN.md`

### RW-8 Write test discovery under build/
- State: open
- Done when: no `cmake_test_discovery_*.json` file appears in the repository root after
  `st test`, and its ignore rules are gone
- Source: `docs/design/SUSHITRACK_MIGRATION.md` §5

### RW-9 Run st build and st test in CI on Windows and Linux
- State: open
- Done when: `.github/workflows/ci.yml` builds and runs the small and medium tests on both
  systems

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

## C ABI

### RW-13 Report a failure from sushitrack_update and sushitrack_get_tracklets
- State: open
- Done when: both calls return a `sushitrack_status_t` a caller can tell from a frame with no
  tracks

### RW-14 Keep a throwing log callback inside the C boundary
- State: open
- Done when: a test whose callback throws at ERROR level sees the C entry point return its
  failure value

### RW-15 Release the tracker when sushitrack_create_ex fails late
- State: open
- Done when: `sushitrack_create_ex` leaves `*out_tracker` null on every failure and a throwing
  `Tracker` constructor leaks nothing

## Install and release

### RW-16 Take sushicore from one source in every install path
- State: open
- Done when: `environment.yml`, the Dockerfile and `cli/pyproject.toml` name the same sushicore
  version and the same source

### RW-17 Generate every version number from CMakeLists.txt
- State: open
- Done when: the binding and the CLI read the version `project()` declares, and
  `__version__ = "2.0.0"` is gone

### RW-18 Check for GoogleTest in st doctor
- State: open
- Done when: the GoogleTest fragment in `cli/sushistack.deps.toml` carries a check command
  and describes the default `st build` truly

## Documentation

### RW-19 Record the machine, commit and date of the next benchmark run
- State: unmeasured
- Done when: `docs/reference/BENCHMARKS.md` names them beside every table
