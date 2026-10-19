# Skill alignment plan

Run inline by one session on `feature/skill_alignment`. Each step is one commit and leaves
`st build` and the tests green.

| Step | Commit | Done when |
| --- | --- | --- |
| 1 | `build(tools): copy the ten shared checkers from SushiSkills` | `check_all.py` runs; the checker tests pass |
| 2 | `docs: rewrite the backlog and the known issues in the backlog format` | `check_backlog.py` and `check_docs_layout.py` pass |
| 3 | `build: move configuration files under .config and assets` | the root holds only what the layout names, `bindings/` aside |
| 4 | `build: move the build logic of the root CMakeLists.txt into cmake/` | the root file holds `project()`, options and `add_subdirectory` |
| 5 | `refactor(include)!: split the headers by area, one type per header` | every header holds one primary symbol under `include/SushiTrack/<area>/` |
| 6 | `refactor!: apply the C++ naming rules` | no `I` prefix, `snake_case` functions and members, `K_` constants |
| 7 | `test: mirror the source tree and give every test a size` | `check_test_mirror.py` and `check_test_sizes.py` pass |
| 8 | `feat(cli)!: select tests by size label` | `st test --label` works; `--suite` is gone |
| 9 | `refactor: move the evaluation and inference programs to applications/` | `st eval` and `st infer` find them there |
| 10 | `refactor(cli): mirror the CLI tests and route output through the console` | `check_logging.py` passes; CLI tests sized |
| 11 | `docs: make the manual true against the source` | `check_docs_truth.py` passes |
| 12 | `docs: close the skill alignment work` | `REPORT.md`, changelog, backlog updated; `check_all.py` clean |
