# Skill alignment report

One session ran the plan inline on `feature/skill_alignment`, on Opus with no subagent. Every
step is a commit of its own; `PLAN.md` lists them by subject.

## Result

| Check | Command | Outcome |
| --- | --- | --- |
| Every shared checker and its tests | `python tools/check_all.py .` | `clean: 10 checkers, tests` |
| Clean build, 4 jobs | `st build --clean -j 4` | `[SUCCESS] Build completed (sushitrack, tracker_bridge, mot_engine, ...)`, no compiler warning |
| C++ tests, small and medium | `st test` | `100% tests passed, 0 tests failed out of 389` (373 small, 22 medium; 6 disabled since before) |
| CLI tests | `st test --cli` | `128 passed` |
| Benchmark against the recorded run | `st eval --scenario MOT17-02-DPM --compare` | SushiTrack HOTA 19.2, IDSW 39, FP 204, FN 15677, as `docs/reference/BENCHMARKS.md` records |

Before the work, 552 C++ tests passed: 299 unit, 31 integration and 228 regression runs. The 228
regression runs were a smoke test of three trackers over the benchmark data, not of SushiTrack's
promise; they left the test runner with the benchmark program, and `test_mot_runner.cpp` and the
adapter tests took their place. 49 tests cover the 18 headers that had none, and 24 cover the
application headers.

## What changed, by skill

| Skill | Change |
| --- | --- |
| `project-tools` | The ten shared checkers, `check_all.py` and `settings.py`; CI runs `check_all.py` |
| `documentation` | Backlog and known issues in the entry format; manual true against the source; two pages split under the ceiling; root README in the skill's section order |
| `repository-layout` | `.config/`, `assets/`, `cmake/`, `applications/`; headers under `include/SushiTrack/<area>/`, one type each; tests mirror the headers |
| `cpp-code-style` | No `I` prefix, `snake_case` functions and members, `K_` constants, `final`, `explicit`, `[[nodiscard]]`, grouped includes, `.clang-format` applied |
| `logging` | SushiLog front with `K_SUSHITRACK_LOG_CATEGORY`; per-frame lines at TRACE; `SUSHITRACK_THROW` only throws; the C log callback removed |
| `dependencies` | SushiLog pinned in `cli/sushistack.deps.toml`; every deploy package carries `LICENSE`, `NOTICE.md` and the licence texts |
| `testing` | Every suite sized `Small_`/`Medium_`; `st test --label`; quarantine label; CLI tests sized with `pytestmark` and timed out by size |
| `performance` | The benchmark is a program under `applications/benchmarks/` that decides nothing |
| `source-comments` | Every new file carries the license block and an `@file` block; reasoning moved from CMake comments to `docs/architecture/BUILD_SYSTEM.md` |

The owner decided four questions at the start, recorded in `SPEC.md`, and two during the work:
the C log callback goes, and the benchmark data splits into one folder per dataset under one
engine.

## SOLID breaches and dependencies

Made and removed in this work:

- `SushiTrackAdapter` reached into the C layer's private `sushitrack_tracker_impl_t` through a
  `reinterpret_cast` and kept a copy of its layout. It drives the public C API now, and the
  bridge links the shared library.
- The Python evaluator and inference scripts appended `cli/` to `sys.path` to import the `st`
  console, while the CLI imports the evaluator. Each prints through its own `messages.py` now.
- Unneeded: `LAPJVSolver::exec_lapjv` summed a total cost no caller read, every frame; the
  `c_types.hpp` aliases had no user; `tools/common/` was a stale copy of `tools/checking/`.

Kept, with what each costs:

- `KalmanFilterRegistry`, `NullGateValidator::get_instance` and `TrackerRegistry` are
  singletons; `cpp-code-style` wants dependencies through the constructor. Changing the registry
  is a change to how filters plug in, which is the owner's call.
- `logger.hpp` and `kalman_filter_registry.hpp` are public headers that include SushiLog's, so a
  C++ consumer of those two headers needs SushiLog's include path. The C API does not.
- `messages.py` exists twice, in `applications/inference/` and in the evaluator: two
  applications, no module to share it from in a one-module repository.
- `applications/tracker_bridge/` is a library that two applications use. In a multi-module
  layout it would be a module.

Seen and not changed:

- The C API's catch-all handlers lost their fault injection: a throwing log sink cannot reach
  them any more, and no other test reaches them.
- The gating log test can only check "at most one line": the line is TRACE, which a release build
  compiles out.
- `check_test_mirror.py` does not check `cli/tests/` or the Python files under `applications/`;
  the CLI mirror was built by hand and the Python applications have no tests.
- Not every private member carries a Doxygen block; no checker looks and this work did not
  audit it.

## Notes for SushiSkills

- `cpp-code-style` asks for a blank line inside an empty body, and the `.clang-format` the
  repositories share keeps `KeepEmptyLinesAtTheStartOfBlocks: false`, which removes it.
- `.docs_truth_allow` entries are matched whole: `^package/` excuses nothing, `^package/.*$`
  does. The skill's wording reads as a search.
- `copy_shared_tools.py` copied the checkers with SushiSkills' license block; the first run of
  `write_license_block.py` in the repository rewrote them.

## Backlog fact check

Step 2 rewrote the backlog and the known issues from the old lists, checking against the tree the
entries a later step would touch (the CI workflow, the sushicore pins, the ignore files); the
entries this work did not touch were carried over unchecked. Closed by this work: RW-1, RW-8, RW-14,
RW-20; KI-2, KI-9, KI-11, KI-12, KI-14, KI-15, KI-16, KI-17. KI-13 narrowed to `bindings/`, which
waits on RW-3. Added from other repositories: RW-20 (sushiweb's licence note, found stale in part:
the ported files kept their notices, the packages did not) and RW-21 (sushistack RW-10).

## History rewrite

On the owner's word, `git filter-repo` replaced the project's former product and company name in
every commit message, file and path of `main` and `feature/skill_alignment`, and wrote the
former company's copyright lines as `Mustafa Garip & Sushi Systems`, whose rights the owner
confirmed. Every commit hash changed and both branches were force-pushed. A tree comparison of
each branch tip before and after, with the replacements applied to the old side, found 0 files
missing, added or different (472 on `main`, 604 on the branch). The old commits stay reachable on
GitHub by hash until GitHub collects them, and in every clone made before the rewrite.

## Local state

The submodules' git folders stay under `.git/modules/3rdparty/` in this clone, pointed at by the
renamed `.gitmodules` sections; a fresh clone uses `.git/modules/third_party/`. Nothing else
outside the tree was written; the scratch scripts of this session sit in its scratchpad.
