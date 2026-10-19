# Skill alignment plan

Run inline by one session on `feature/skill_alignment`. Each step is one commit and leaves
`st build` and the tests green. Steps 7 and 8 became one commit: switching the CTest labels from
kinds to sizes without switching `st test` would have left a commit whose `--suite unit` selects
nothing. Steps 6b, 9b and 13 were asked for during the work.

| Step | Commit | Done when |
| --- | --- | --- |
| 1 | `build(tools): copy the ten shared checkers from SushiSkills` | `check_all.py` runs; the checker tests pass |
| 2 | `docs: rewrite the backlog and the known issues in the backlog format` | `check_backlog.py` and `check_docs_layout.py` pass |
| 3 | `build: move configuration files under .config and assets` | the root holds only what the layout names, `bindings/` aside |
| 4 | `build: move the build logic of the root CMakeLists.txt into cmake/` | the root file holds `project()`, options and `add_subdirectory` |
| 5 | `refactor(core)!: split the headers by area, one type per header` | every header holds one primary symbol under `include/SushiTrack/<area>/` |
| 6 | `refactor(core)!: apply the C++ naming and formatting rules` | no `I` prefix, `snake_case` functions and members, `K_` constants, `.clang-format` stable |
| 6b | `feat(logging)!: log through SushiLog` | the library logs through SushiLog with its category; `check_logging.py` passes for C++ |
| 7, 8 | `test!: mirror the source tree and select tests by size` | `check_test_mirror.py` and `check_test_sizes.py` pass for C++; `st test --label` works |
| 9 | `refactor(applications)!: move the evaluation and inference programs to applications/` | `st eval` reproduces the recorded MOT17-02-DPM scores |
| 9b | `refactor(applications): split the benchmark data into mot17, mot20 and dancetrack` | `st eval --benchmark` runs and scores each benchmark through one engine |
| 10 | `refactor(cli)!: mirror the CLI tests and ship the notices in every package` | CLI tests mirrored and sized; `check_logging.py` passes |
| 11 | `docs: make the manual true against the source` | `check_docs_truth.py` passes |
| 12 | `docs: close the skill alignment work` | `REPORT.md`, changelog, backlog updated; `check_all.py` clean |
| 13 | History rewrite, no commit of its own | no commit message or file in any commit holds the old project name |
