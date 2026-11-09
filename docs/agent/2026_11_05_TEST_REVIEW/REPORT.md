# Test review: report

**Status:** Open — every test has a verdict; the plan's jobs are next

Every test in the repository has a verdict from `skills/testing/REVIEWING.md` of SushiSkills, reviewed
on 2026-11-05 as part of J15 (`docs/agent/2026_11_03_INDUSTRIAL_TESTING/` in SushiSkills). The review read
the code and ran nothing. Each record names the files it read. The tests of the shared checkers under
`tools/` are reviewed once, in SushiSkills; this report covers this repository's own tests and its own
checkers' tests.

## What the review found

- **Process-wide state from small tests.** `SushiTrackAdapter()` reads `SUSHITRACK_CONFIG`;
  `TrackerRegistry` and `KalmanFilterRegistry::get_instance`, read by `GenericKalmanFactory`, are
  process-wide singletons; `CapturedLog` installs a sink on the process-wide logger in 19 files. Each
  needs a seam: a parsed stream, an owned registry, explicit adapter parameters, and SushiLog's sink
  seam.
- **The CLI tests** monkeypatch the repository's own functions (`load_build_env`, `build_svc.build`,
  `diag.config_show`, `cli.main`) through runner fixtures in `test_build.py` and
  `test_library_packages.py`, about 25 tests, and read private names (`_lazy`, `_warn`, `_coerce`,
  `_run_benchmark`, `_use_records`, `_template`, `_copy_notices`, `_snapshot`).
- **Loops** checking each state component (22 of 68 non-keeps in one chunk).
- **The oracle is the code:** 11 calculator tests compare `calculate_single` with the batch path; 11
  config tests run an in-test copy of the loader instead of `sushitrack_load_params_from_json`.
- Fixed temporary names collide under parallel shards: `sushitrack_mot_runner_test`,
  `sushitrack_frame_recorder_test`, `sushitrack_json_settings_test.json`, `test_mot_sequence.cpp`.
- Three test files include `source/private_include/SushiTrack/profiling/profiling.hpp`; they need a
  public profiler macro header.
- No `tests/support/tolerance.hpp`.

## Defects the review found

- A negative frame rate is documented as rejected, but `sushitrack_c.cpp` lines 185 to 189 clamp it
  to 1.
- Three TRACE gating-log tests accept zero lines; the NonFinite LAPJV test never asserts that its NaN pair
  stays unmatched.
- `Config.NothingNamed` depends on the temp folder's ancestors; the config helpers leave files and
  `SUSHITRACK_CONFIG` set on Windows (`_putenv_s` with an empty value).
- Weak checks: `test_bool_field`, `test_the_readmes`, `test_the_runtime_library_names`,
  `test_a_package_that_cannot_be_chosen`.
- `docs/guides/TESTING.md` defines sizes against the skill (60 s small, files allowed).
- `tools/testing/check_test_additions.py` and its test are a leftover: SushiSkills removed the checker in
  phase B, and the copier never removes a file upstream dropped. Both are deleted.
- `tools/tests/test_test_budget_settings.py` skips here, since the repository has no `skills/` folder, so
  its promise is checked nowhere; it also defines `read_budget` and `differences` in the test file.

## Decisions

The owner delegated these decisions on 2026-11-05. Each ruling below is final for the plan; its reason follows it.

1. A negative frame rate is rejected, as the documentation says, and the code changes. A silent clamp to 1 hides the caller's error.
2. The seams are approved: a stream to parse in place of a file, owned `TrackerRegistry` and `KalmanFilterRegistry` passed in, and explicit adapter parameters, with `SUSHITRACK_CONFIG` read only at the CLI edge. Every seam is added as a parameter or overload whose default keeps today's behaviour, so existing callers compile unchanged and the change is a minor version. A value passed in is preferred to a reset of process-wide state.
3. SushiLog's seam is a scoped in-memory sink: a guard that swaps the process-wide logger's sink for one test and restores it on exit. Routing a logger through every call site in every repository would cost more than it returns, and small tests run one at a time in their process.
4. A named constant holding a hand-typed epsilon is not a typed tolerance. A tolerance names its kind (absolute, relative or ulps) and its derivation, so every such constant is redefined through `tolerance.hpp`, keeping its name. The rewrites stay rewrites, and they are mechanical.
5. T2's seams, ruled by the orchestrator under decision 2 on 2026-11-09. `KalmanFilterRegistry` and `TrackerRegistry` get public constructors; `GenericKalmanFactory` takes a registry, and `SushiTrack::create_tracker` (`include/SushiTrack/c_api/create_tracker.hpp`) builds what `sushitrack_create_ex` builds from a registry the caller owns. `SushiTrack::load_params`, `parse_json_settings`, `parse_mot_detections` and `parse_frame_rate` read a stream. `SushiTrackAdapter` takes `sushitrack_params_t`, `ByteTrackAdapter` and `OCSORTAdapter` take a parsed settings document, and `build_tracker` and `register_default_trackers` take a `TrackerRegistry`. Each old signature keeps its behaviour and calls the new one.
6. `sushitrack_load_params_from_json(NULL, ...)` still reads `SUSHITRACK_CONFIG`. Decision 2 asks for both a default that keeps today's behaviour and the variable read only at the CLI edge; the C API's documented resolution order is today's behaviour, so the default keeps it, and no small test reaches it: the one that did, `CreateWithNullParamsAutoLoadsConfig`, is relabelled medium until M3 rewrites it.
7. A small test that builds a tracker through `sushitrack_create` reads the process-wide `KalmanFilterRegistry` and is not counted against T2's acceptance. After T2 no test writes that registry, so it holds the same three creators in every test, and the review flagged none of those tests. Every test whose promise depends on the registry's contents uses a registry it owns.
8. The test builders the seams need, `default_params` and `library_kalman_registry`, are fixtures and live in `tests/support/`. They are not in-memory adapters: each builds a real library object.
9. T3 pins SushiLog v0.3.1, the release that ships `LogCapture`. `CapturedLog` keeps its name and its counting helpers and holds a `LogCapture`; it returns `SushiLog::CapturedRecord`, so the copy SushiTrack declared is gone. It puts back the level each category had, where it set WARN before. The throwing sink of `test_error_handling.cpp` is not a capture and stays for M3, which resizes those tests to medium.
10. T4 moves `profiling.hpp` from `source/private_include/` to `include/SushiTrack/profiling/`, keeping its include path, rather than writing a second public header beside a private one. Six test files included it, not three. No other public header includes it, so a consumer of the library still compiles without SushiProfiler; a target that includes it calls `sushitrack_link_profiling`, which now adds only the generated config folder.

11. T6 fixes each defect that sits in one test or one page, and backlogs the two that need a seam or a build change. The NaN pair, the gating-log lines, the four weak checks and the temp-folder leaks are fixed; the negative frame rate is refused as decision 1 says; `docs/guides/TESTING.md` now carries the skill's sizes. RW-36 holds the CTest timeouts (the build still sets 60, 300 and 900 s) and RW-37 the configuration search seam that `NothingNamedAndNothingFoundSucceedsAtInfo` needs. `tools/tests/test_test_budget_settings.py` and `K_TEST_BUDGET` are deleted instead of rewritten: the test always skips here, `K_TEST_BUDGET` served nothing else, and M12's records for that file are void.
12. Rulings of the orchestrator for the M jobs, 2026-11-10, under decision 2. (a) A record that needs a missing seam gets it: a port with an in-memory adapter beside the production one in production source, default behaviour unchanged. Never `tests/support`, and no test stays medium for want of a seam. (b) A header that loses its last test keeps `compile_<name>.cpp` with `static_assert`s, registered as a compile check. (c) A bare tolerance becomes a `tolerance.hpp` bound derived from unit roundoff. (d) Coverage is never lost: a claim a deleted or merged test held is asserted somewhere else. (e) A folder README or page changes in the same commit as each changed test; one commit per file.

## Dispatches

dispatch: J15 review, 6 chunks — haiku high, 1 round each, passed, rung 1

## Totals

| Tests | Keep | Rewrite | Resize | Split | Merge | Delete | Characterize | Add |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 594 | 180 | 244 | 34 | 23 | 47 | 66 | 0 | 35 |

Kept as they stand: 180 of 594 (30%).

## Verdicts by question

| Verdict | Question | What it means | Tests |
| --- | --- | --- | --- |
| rewrite | 1 | a forbidden kind | 182 |
| merge | 4 | a duplicate | 47 |
| delete | 1 | a forbidden kind | 40 |
| resize | 8 | wrong size | 34 |
| rewrite | 2 | cannot fail | 24 |
| split | 3 | no single promise | 23 |
| delete | 4 | a duplicate | 21 |
| rewrite | 7 | a long-test sign | 15 |
| rewrite | 6 | oracle is the code | 13 |
| rewrite | 8 | wrong size | 8 |
| delete | 3 | no single promise | 5 |

## Modules

| Module | Files | Tests | Keep | Rewrite | Resize | Split | Merge | Delete | Characterize |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `tracking` | 25 | 151 | 26 | 63 | 4 | 5 | 36 | 17 | 0 |
| `cli/services` | 12 | 74 | 19 | 44 | 5 | 2 | 0 | 4 | 0 |
| `filtering` | 12 | 43 | 5 | 23 | 3 | 4 | 0 | 8 | 0 |
| `applications` | 14 | 38 | 2 | 21 | 2 | 7 | 1 | 5 | 0 |
| `association` | 17 | 91 | 58 | 20 | 1 | 0 | 6 | 6 | 0 |
| `c_api` | 5 | 49 | 19 | 11 | 9 | 3 | 1 | 6 | 0 |
| `cli` | 8 | 57 | 31 | 21 | 3 | 0 | 0 | 2 | 0 |
| `configuration` | 2 | 25 | 5 | 19 | 0 | 0 | 0 | 1 | 0 |
| `detection` | 2 | 20 | 7 | 3 | 0 | 0 | 3 | 7 | 0 |
| `cli/test_cli` | 2 | 20 | 8 | 8 | 0 | 0 | 0 | 4 | 0 |
| `logging` | 3 | 10 | 0 | 4 | 5 | 0 | 0 | 1 | 0 |
| `tools` | 2 | 9 | 0 | 1 | 2 | 1 | 0 | 5 | 0 |
| `profiling` | 4 | 7 | 0 | 6 | 0 | 1 | 0 | 0 | 0 |

## Records

One line per test: `file::test | verdict | question | reason | files read`.

### `cli/tests/services/test_artifacts.py`

```
test_find_file_prefers_the_conventional_bin_dirs | keep | 9 | row: the conventional bin dir wins over a deeper copy | cli/tests/services/test_artifacts.py; cli/sushitrack_cli/services/artifacts.py
test_find_file_falls_back_to_a_recursive_search | keep | 9 | row: a file outside the conventional dirs is found recursively | cli/tests/services/test_artifacts.py; cli/sushitrack_cli/services/artifacts.py
test_find_file_answers_none_without_a_build_tree | keep | 9 | row: a missing tree gives None; the check reads the disk, so medium | cli/tests/services/test_artifacts.py; cli/sushitrack_cli/services/artifacts.py
test_the_index_skips_git_and_walks_deeper_than_four | split | 3 | "and" joins the deep walk and the .git prune, which can break apart; one test each | cli/tests/services/test_artifacts.py; cli/sushitrack_cli/services/artifacts.py
test_match_prefers_an_exact_name | keep | 9 | row: an exact stem wins over a substring | cli/tests/services/test_artifacts.py; cli/sushitrack_cli/services/artifacts.py
test_find_binary_adds_the_platform_suffix | keep | 9 | row: find_binary adds the platform suffix | cli/tests/services/test_artifacts.py; cli/sushitrack_cli/services/artifacts.py
test_import_lib_name_names_the_link_library | rewrite | 1 | forbidden kind: control flow in body (a ternary on IS_WINDOWS picks the expected value, mirroring the code); rewrite with a row table per platform | cli/tests/services/test_artifacts.py; cli/sushitrack_cli/services/artifacts.py
```

### `cli/tests/services/test_build.py`

```
test_a_pinned_sushi_library_reaches_the_configure | rewrite | 1 | forbidden kind: the runner fixture replaces the repo-owned build_svc.load_build_env; the seam belongs at sushicore's snapshot_vcvars | cli/tests/services/test_build.py; cli/sushitrack_cli/services/build.py; cli/sushitrack_cli/env.py; cli/sushitrack_cli/library_packages.py
test_a_sanitizer_reaches_the_configure_and_builds_its_own_tree | rewrite | 1 | same fixture; once fixed, split (Q3): define and stamp; own tree and plain tree untouched | cli/tests/services/test_build.py; cli/sushitrack_cli/services/build.py; cli/sushitrack_cli/env.py
test_the_default_build_type_is_debug | rewrite | 1 | same fixture | cli/tests/services/test_build.py; cli/sushitrack_cli/services/build.py; cli/sushitrack_cli/env.py
test_each_build_type_has_a_tree_of_its_own | rewrite | 1 | same fixture | cli/tests/services/test_build.py; cli/sushitrack_cli/services/build.py; cli/sushitrack_cli/env.py
test_another_build_type_is_refused_in_an_existing_tree | rewrite | 1 | same fixture | cli/tests/services/test_build.py; cli/sushitrack_cli/services/build.py; cli/sushitrack_cli/env.py
test_a_define_cannot_set_the_build_type | rewrite | 1 | same fixture | cli/tests/services/test_build.py; cli/sushitrack_cli/services/build.py; cli/sushitrack_cli/env.py
test_a_stopped_gate_refuses_the_build | rewrite | 1 | same fixture | cli/tests/services/test_build.py; cli/sushitrack_cli/services/build.py; cli/sushitrack_cli/env.py
test_the_configure_carries_both_launcher_entries | delete | 1 | forbidden kind: the runner fixture stub; the promise is tested by test_a_fresh_tree_is_configured_then_built, which checks the same launcher entries in the full argv (Q4 sibling) | cli/tests/services/test_build.py; cli/sushitrack_cli/services/build.py
test_the_launcher_runs_host_compiles_through_the_configured_cache | rewrite | 1 | same fixture | cli/tests/services/test_build.py; cli/sushitrack_cli/services/build.py; cli/sushitrack_cli/env.py
test_the_compile_takes_the_hardware_threads_without_jobs | rewrite | 1 | same fixture; once fixed, a row with test_jobs_overrides_the_hardware_threads (Q4) | cli/tests/services/test_build.py; cli/sushitrack_cli/services/build.py; cli/sushitrack_cli/env.py
test_jobs_overrides_the_hardware_threads | rewrite | 1 | same fixture; once fixed, a row with the test above (Q4) | cli/tests/services/test_build.py; cli/sushitrack_cli/services/build.py; cli/sushitrack_cli/env.py
test_a_report_is_printed_and_written | rewrite | 1 | same fixture; "printed" is not checked (Q2): the report line is never asserted | cli/tests/services/test_build.py; cli/sushitrack_cli/services/build.py; cli/sushitrack_cli/env.py
test_the_default_generator_follows_the_host | resize | 8 | pure: generator() over a Config; move to a small file | cli/tests/services/test_build.py; cli/sushitrack_cli/services/build.py
test_a_configured_generator_wins | resize | 8 | pure: generator() over a Config; move to a small file | cli/tests/services/test_build.py; cli/sushitrack_cli/services/build.py
test_a_fresh_tree_is_configured_then_built | rewrite | 1 | same fixture; the promise holds and the argv is exact | cli/tests/services/test_build.py; cli/sushitrack_cli/services/build.py; cli/sushitrack_cli/env.py
test_a_matching_stamp_skips_configure | rewrite | 1 | same fixture | cli/tests/services/test_build.py; cli/sushitrack_cli/services/build.py; cli/sushitrack_cli/env.py
test_a_visual_studio_tree_is_not_reconfigured_for_a_missing_makefile | rewrite | 1 | same fixture | cli/tests/services/test_build.py; cli/sushitrack_cli/services/build.py; cli/sushitrack_cli/env.py
test_an_empty_generator_needs_no_makefile | rewrite | 1 | same fixture | cli/tests/services/test_build.py; cli/sushitrack_cli/services/build.py; cli/sushitrack_cli/env.py
test_a_stamp_mismatch_reconfigures | rewrite | 1 | same fixture | cli/tests/services/test_build.py; cli/sushitrack_cli/services/build.py; cli/sushitrack_cli/env.py
test_a_stale_tree_reconfigures | rewrite | 1 | same fixture | cli/tests/services/test_build.py; cli/sushitrack_cli/services/build.py; cli/sushitrack_cli/env.py
test_clean_removes_the_tree_before_configuring | rewrite | 1 | same fixture | cli/tests/services/test_build.py; cli/sushitrack_cli/services/build.py; cli/sushitrack_cli/env.py
test_build_stops_at_a_tree_it_could_not_remove | rewrite | 1 | same fixture; the stdlib shutil.rmtree stub is allowed | cli/tests/services/test_build.py; cli/sushitrack_cli/services/build.py; cli/sushitrack_cli/env.py
test_clean_reports_a_tree_it_could_not_remove | keep | 9 | row: clean returns 1 and names the tree it could not remove; recorder is the console seam the module provides | cli/tests/services/test_build.py; cli/sushitrack_cli/services/build.py; cli/tests/conftest.py
test_clean_removes_every_tree_and_the_package | keep | 9 | row: clean removes every tree and the package | cli/tests/services/test_build.py; cli/sushitrack_cli/services/build.py
test_build_type_values_match_build_types | resize | 8 | pure: the BuildType enum against the tuple; move to a small file | cli/tests/services/test_build.py; cli/sushitrack_cli/services/build.py
```

### `cli/tests/services/test_checks.py`

```
test_groups_cover_build_infer_and_eval | keep | 9 | row: checks group as build, infer and eval | cli/tests/services/test_checks.py; cli/sushitrack_cli/services/checks.py
test_infer_group_names_the_spec_modules | keep | 9 | row: the infer group names the spec modules; the list is the public doctor output | cli/tests/services/test_checks.py; cli/sushitrack_cli/services/checks.py
test_eigen_is_required_and_found | keep | 9 | row: eigen is required and found under third_party | cli/tests/services/test_checks.py; cli/sushitrack_cli/services/checks.py
test_missing_python_module_fix_names_the_pip_package | keep | 9 | row: a missing module's fix names its pip package | cli/tests/services/test_checks.py; cli/sushitrack_cli/services/checks.py
test_missing_datasets_fail_with_a_pointer_to_the_data_layout | keep | 9 | row: missing datasets fail with the data-layout pointer | cli/tests/services/test_checks.py; cli/sushitrack_cli/services/checks.py
test_datasets_found_when_a_sequence_has_ground_truth | keep | 9 | row: datasets found when a sequence has ground truth | cli/tests/services/test_checks.py; cli/sushitrack_cli/services/checks.py
test_missing_docker_fix_follows_the_platform | keep | 9 | row: the docker fix follows the platform | cli/tests/services/test_checks.py; cli/sushitrack_cli/services/checks.py
test_doctor_for_infer_runs_only_infer | keep | 9 | row: doctor for infer runs only the infer group | cli/tests/services/test_checks.py; cli/sushitrack_cli/services/checks.py
```

### `cli/tests/services/test_deploy.py`

```
test_the_c_example_includes_each_header_on_its_own_line | rewrite | 1 | forbidden kind: calls the private _template; rewrite through the public deploy output | cli/tests/services/test_deploy.py; cli/sushitrack_cli/services/deploy.py; cli/sushitrack_cli/templates/example.c.in
test_the_python_example_parses | rewrite | 1 | forbidden kind: no assertion; ast.parse raises on failure, an implicit oracle reserved for fuzz and stress tests; also calls the private _template; assert a named property of the parsed example | cli/tests/services/test_deploy.py; cli/sushitrack_cli/services/deploy.py; cli/sushitrack_cli/templates/example.py.in
test_the_readmes_name_every_runtime_library | rewrite | 1 | forbidden kind: a for statement in the body; calls the private _template; the sentinels "r" and "s" are letters any text contains, so the check cannot fail (Q2) | cli/tests/services/test_deploy.py; cli/sushitrack_cli/services/deploy.py; cli/sushitrack_cli/templates/cpp_readme.md.in; cli/sushitrack_cli/templates/python_readme.md.in
test_notices_reach_the_package | rewrite | 1 | forbidden kind: for statements in the body; calls the private _copy_notices | cli/tests/services/test_deploy.py; cli/sushitrack_cli/services/deploy.py
test_a_missing_notice_stops_the_package | rewrite | 1 | forbidden kind: calls the private _copy_notices; rewrite through the public deploy entry | cli/tests/services/test_deploy.py; cli/sushitrack_cli/services/deploy.py
test_deploy_kind_values_match_kinds | resize | 8 | pure: enum values against KINDS; move to a small file | cli/tests/services/test_deploy.py; cli/sushitrack_cli/services/deploy.py
test_the_runtime_library_names_follow_the_platform | rewrite | 2 | membership in two names passes for either name on either platform (Q2); the promise has no other test; assert the name per platform in a row table | cli/tests/services/test_deploy.py; cli/sushitrack_cli/services/artifacts.py
```

### `cli/tests/services/test_diag.py`

```
test_git_missing_answers_none | rewrite | 1 | forbidden kind: calls the private _git_describe; rewrite through the public status report | cli/tests/services/test_diag.py; cli/sushitrack_cli/services/diag.py
test_git_describe_joins_branch_commit_and_dirt | rewrite | 1 | forbidden kind: calls the private _git_describe | cli/tests/services/test_diag.py; cli/sushitrack_cli/services/diag.py
test_env_dump_reads_the_tool_settings | rewrite | 1 | forbidden kind: replaces the repo-owned diag.load_build_env | cli/tests/services/test_diag.py; cli/sushitrack_cli/services/diag.py; cli/sushitrack_cli/env.py
test_diagnostics_reads_the_context_once_per_report | rewrite | 1 | forbidden kind: replaces the repo-owned diag.config_show and diag.env_dump | cli/tests/services/test_diag.py; cli/sushitrack_cli/services/diag.py
```

### `cli/tests/services/test_docker.py`

```
test_the_service_uses_the_shared_runner | delete | 1 | forbidden kind: change detector (asserts which object spawns the children); the exit-code promise is unchecked, see Adds | cli/tests/services/test_docker.py; cli/sushitrack_cli/services/docker.py; cli/sushitrack_cli/process.py
```

### `cli/tests/services/test_evaluate.py`

```
test_the_service_uses_the_shared_runner | delete | 1 | forbidden kind: change detector (asserts which object spawns the children) | cli/tests/services/test_evaluate.py; cli/sushitrack_cli/process.py
```

### `cli/tests/services/test_infer.py`

```
test_the_service_uses_the_shared_runner | delete | 1 | forbidden kind: change detector (asserts which object spawns the children) | cli/tests/services/test_infer.py; cli/sushitrack_cli/process.py
```

### `cli/tests/services/test_label.py`

```
test_run_benchmark_hands_the_tool_settings_to_the_build_env | rewrite | 1 | forbidden kind: calls the private _run_benchmark and replaces the repo-owned find_binary and load_build_env | cli/tests/services/test_label.py; cli/sushitrack_cli/services/label.py; cli/sushitrack_cli/env.py
test_benchmark_capture_keeps_streams_apart | rewrite | 1 | forbidden kind: calls the private _run_benchmark; replaces repo-owned find_binary and load_build_env | cli/tests/services/test_label.py; cli/sushitrack_cli/services/label.py
test_benchmark_args_name_every_tracker_and_sequence | rewrite | 1 | forbidden kind: calls the private _benchmark_args | cli/tests/services/test_label.py; cli/sushitrack_cli/services/label.py
test_eval_fails_when_no_sequence_matches | keep | 9 | row: evaluate returns 1 and says no sequence matched | cli/tests/services/test_label.py; cli/sushitrack_cli/services/label.py
test_eval_refuses_an_unknown_benchmark | keep | 9 | row: an unknown benchmark returns 2 and names the available ones | cli/tests/services/test_label.py; cli/sushitrack_cli/services/label.py
test_clean_predictions_removes_every_pred_folder | keep | 9 | row: clean_predictions removes each pred folder and keeps detections | cli/tests/services/test_label.py; cli/sushitrack_cli/services/label.py
test_parse_perf_warns_on_a_line_it_cannot_read | rewrite | 1 | forbidden kind: calls the private _parse_perf; rewrite through evaluate with the runner's output stubbed | cli/tests/services/test_label.py; cli/sushitrack_cli/services/label.py
test_benchmark_args_pass_the_profiles_folder_when_given | rewrite | 1 | forbidden kind: calls the private _benchmark_args | cli/tests/services/test_label.py; cli/sushitrack_cli/services/label.py
test_a_profile_record_replaces_the_perf_figure_and_prints_its_quartiles | rewrite | 1 | forbidden kind: calls the private _use_records | cli/tests/services/test_label.py; cli/sushitrack_cli/services/label.py; cli/sushitrack_cli/services/profile_record.py
test_a_pair_with_neither_record_nor_perf_has_no_timing | rewrite | 1 | forbidden kind: calls the private _use_records | cli/tests/services/test_label.py; cli/sushitrack_cli/services/label.py
```

### `cli/tests/services/test_profile_record.py`

```
test_the_median_and_quartiles_come_from_the_frame_periods | rewrite | 1 | forbidden kind: a small test writes and reads files; split the parse from the read so the parse is tested on values | cli/tests/services/test_profile_record.py; cli/sushitrack_cli/services/profile_record.py
test_warm_up_periods_and_other_timelines_are_left_out | rewrite | 1 | forbidden kind: a small test writes and reads files; the parse seam as above | cli/tests/services/test_profile_record.py; cli/sushitrack_cli/services/profile_record.py
test_a_missing_file_a_wrong_version_and_an_empty_record_give_none | rewrite | 1 | forbidden kind: a small test reads a missing and written files; the parse seam for version and empty, a medium test for the missing file | cli/tests/services/test_profile_record.py; cli/sushitrack_cli/services/profile_record.py
test_text_that_is_not_json_gives_none | rewrite | 1 | forbidden kind: a small test writes and reads a file; the non-JSON case is a medium test of the read | cli/tests/services/test_profile_record.py; cli/sushitrack_cli/services/profile_record.py
```

### `cli/tests/services/test_runner.py`

```
test_run_matches_through_the_shared_index | rewrite | 1 | forbidden kind: replaces the repo-owned runner.load_build_env | cli/tests/services/test_runner.py; cli/sushitrack_cli/services/runner.py; cli/sushitrack_cli/env.py
```

### `cli/tests/services/test_tests.py`

```
test_ctest_gets_a_config_only_for_a_multi_config_tree | rewrite | 1 | forbidden kind: replaces the repo-owned test_svc.load_build_env (rec fixture); control flow in body (a ternary builds the compared value) | cli/tests/services/test_tests.py; cli/sushitrack_cli/services/tests.py; cli/sushitrack_cli/env.py
test_the_default_tree_is_the_one_of_the_configured_build_type | resize | 8 | pure: build_tree() path arithmetic; move to a small file | cli/tests/services/test_tests.py; cli/sushitrack_cli/services/tests.py; cli/sushitrack_cli/services/build.py
test_a_sanitizer_names_its_own_tree_and_the_two_builds_it_offers | split | 3 | "and" joins the names and the tree path, which can break apart; one test each | cli/tests/services/test_tests.py; cli/sushitrack_cli/services/tests.py
test_a_sanitizer_build_is_the_st_build_of_that_sanitizer | rewrite | 1 | forbidden kind: replaces the repo-owned build_svc.build; assert the sanitizer define on the recorded configure instead | cli/tests/services/test_tests.py; cli/sushitrack_cli/services/tests.py; cli/sushitrack_cli/services/build.py
test_a_stale_tree_is_refused_with_the_clean_hint | keep | 9 | row: a stale tree is refused with the clean hint | cli/tests/services/test_tests.py; cli/sushitrack_cli/services/tests.py
```

### `cli/tests/test_aliases.py`

```
test_each_old_spelling_names_its_replacement_and_the_shared_date | keep | 9 | row: aliases: each old spelling maps to its new one with the shared last day | cli/tests/test_aliases.py; cli/sushitrack_cli/aliases.py
test_no_alias_has_expired_today | rewrite | 1 | forbidden kind: a small test reads the real clock (date.today()); rewrite with fixed dates either side of K_REMOVED_AFTER | cli/tests/test_aliases.py; cli/sushitrack_cli/aliases.py
```

### `cli/tests/test_cli/test_help_screen.py`

```
test_root_help_lists_the_three_groups_in_order | rewrite | 1 | forbidden kind: a small test renders help, which builds the console and reads the checkout's config through sushicore's LazyConsole; the size is medium | cli/tests/test_cli/test_help_screen.py; cli/sushitrack_cli/cli.py; cli/sushitrack_cli/console.py
test_every_command_sits_under_its_own_group | rewrite | 1 | forbidden kind: as above (small test reads checkout config) | cli/tests/test_cli/test_help_screen.py; cli/sushitrack_cli/cli.py; cli/sushitrack_cli/console.py
test_a_bare_invocation_prints_the_same_page | rewrite | 1 | forbidden kind: as above (small test reads checkout config) | cli/tests/test_cli/test_help_screen.py; cli/sushitrack_cli/cli.py; cli/sushitrack_cli/console.py
test_the_container_sub_app_draws_its_own_page_under_both_words | rewrite | 1 | forbidden kind: as above (small test reads checkout config) | cli/tests/test_cli/test_help_screen.py; cli/sushitrack_cli/cli.py; cli/sushitrack_cli/console.py
test_main_exits_zero_on_a_bare_invocation | delete | 1 | forbidden kind: a small test reads checkout config; the promise is asserted by test_surface.py::test_main_turns_a_bare_invocation_into_the_help_page | cli/tests/test_cli/test_help_screen.py; cli/tests/test_cli/test_surface.py; cli/sushitrack_cli/cli.py
test_main_exits_zero_on_help | delete | 1 | forbidden kind: a small test reads checkout config; exit 0 on --help is asserted by test_root_help_lists_the_three_groups_in_order (in _run) | cli/tests/test_cli/test_help_screen.py; cli/sushitrack_cli/cli.py
test_main_exits_nonzero_on_an_unknown_command | delete | 1 | forbidden kind: a small test reads checkout config; the promise is asserted by test_surface.py::test_main_exits_nonzero_on_an_unknown_command | cli/tests/test_cli/test_help_screen.py; cli/tests/test_cli/test_surface.py; cli/sushitrack_cli/cli.py
test_shared_provision_commands_are_listed | delete | 1 | forbidden kind: a small test reads checkout config; token presence is weaker than the exact list asserted by test_surface.py::test_describe_lists_every_visible_command_and_no_hidden_one | cli/tests/test_cli/test_help_screen.py; cli/tests/test_cli/test_surface.py
```

### `cli/tests/test_cli/test_surface.py`

```
test_version_names_the_distribution_and_its_version | keep | 9 | row: --version names the distribution and its version | cli/tests/test_cli/test_surface.py; cli/sushitrack_cli/cli.py
test_describe_lists_every_visible_command_and_no_hidden_one | keep | 9 | row: --describe lists exactly the visible commands | cli/tests/test_cli/test_surface.py; cli/sushitrack_cli/cli.py
test_test_is_the_shared_command_and_offers_the_sanitizer_and_the_own_suite | rewrite | 1 | forbidden kind: change detector (asserts the callback's module); the option promise (--sanitize, --cli, --all shown) has no other test; assert the help output | cli/tests/test_cli/test_surface.py; cli/sushitrack_cli/cli.py
test_config_and_env_are_the_shared_declarations | rewrite | 1 | forbidden kind: change detector (asserts the callback's module); the promise that config and env work has no other test; run them through the CLI | cli/tests/test_cli/test_surface.py; cli/sushitrack_cli/cli.py
test_the_docker_word_still_runs_the_container_command | rewrite | 1 | forbidden kind: replaces the repo-owned cli.context, docker_svc.build and docker_svc.run; stub the process runner instead | cli/tests/test_cli/test_surface.py; cli/sushitrack_cli/cli.py; cli/sushitrack_cli/services/docker.py
test_the_docker_word_is_absent_from_help_and_describe | keep | 9 | row: the docker word is absent from help and describe | cli/tests/test_cli/test_surface.py; cli/sushitrack_cli/cli.py
test_docs_bundle_without_a_release_is_a_usage_error | keep | 9 | row: docs bundle without --release exits 2 and names it | cli/tests/test_cli/test_surface.py; cli/sushitrack_cli/cli.py
test_bare_docs_prints_the_group_help_and_exits_two | keep | 9 | row: bare docs prints the group help and exits 2 | cli/tests/test_cli/test_surface.py; cli/sushitrack_cli/cli.py
test_docs_bundle_writes_the_archive_of_the_checkout_it_runs_in | keep | 9 | row: docs bundle writes the archive and digest of the checkout it runs in; report is cli.console, which the recorder replaces | cli/tests/test_cli/test_surface.py; cli/sushitrack_cli/cli.py; cli/tests/conftest.py
test_a_malformed_config_gives_one_line_and_exit_one | rewrite | 1 | forbidden kind: reads the private console._lazy and replaces the repo-owned console.__getattr__ | cli/tests/test_cli/test_surface.py; cli/sushitrack_cli/console.py; cli/sushitrack_cli/cli.py
test_main_turns_a_bare_invocation_into_the_help_page | keep | 9 | row: a bare invocation prints help and exits 0 | cli/tests/test_cli/test_surface.py; cli/sushitrack_cli/cli.py
test_main_exits_nonzero_on_an_unknown_command | keep | 9 | row: an unknown command exits non-zero | cli/tests/test_cli/test_surface.py; cli/sushitrack_cli/cli.py
```

### `cli/tests/test_config.py`

```
test_defaults_are_used_when_no_file_or_env_overrides | keep | 9 | row: config: defaults apply with no file or env, source recorded as default | cli/tests/test_config.py; cli/sushitrack_cli/config.py
test_config_toml_overrides_a_default | keep | 9 | row: config.toml overrides a default | cli/tests/test_config.py; cli/sushitrack_cli/config.py
test_config_local_toml_wins_over_config_toml | keep | 9 | row: config.local.toml wins over config.toml | cli/tests/test_config.py; cli/sushitrack_cli/config.py
test_an_env_override_wins_over_every_file | keep | 9 | row: env override wins over every file | cli/tests/test_config.py; cli/sushitrack_cli/config.py
test_a_platform_table_overlays_the_current_platform_only | rewrite | 1 | forbidden kind: control flow in body (a ternary chooses the other platform as input); rewrite as rows over platform names | cli/tests/test_config.py; cli/sushitrack_cli/config.py
test_a_bool_field_coerces_common_truthy_env_spellings | rewrite | 2 | name promises truthy spellings, body checks only "no"; _coerce's truthy branch has no other test; rewrite as rows of truthy and falsy spellings | cli/tests/test_config.py; cli/sushitrack_cli/config.py
test_tool_settings_carry_the_platform_generator | rewrite | 1 | forbidden kind: a ternary builds the expected value; the test reads the checkout's cli/config.toml, so it tests the file, not the code; rewrite with a platform parameter | cli/tests/test_config.py; cli/sushitrack_cli/config.py; cli/config.toml
test_legacy_environment_names_still_override | keep | 9 | row: ST_CMAKE, ST_CTEST, ST_GENERATOR reach the tool settings | cli/tests/test_config.py; cli/sushitrack_cli/config.py
test_config_no_longer_has_build_tool_fields | delete | 1 | forbidden kind: change detector (dataclass field names); the moved-key promise is asserted by test_load_config_warns_once_per_legacy_key | cli/tests/test_config.py; cli/sushitrack_cli/config.py
test_tool_settings_are_a_provision_config | rewrite | 5 | change detector on inheritance; the docstring promise (every provision field on st's settings) has no other test; rewrite to load settings and read a provision field | cli/tests/test_config.py; cli/sushitrack_cli/config.py
test_legacy_tool_keys_name_each_old_key_and_its_replacement | resize | 8 | pure function over a dict; the smallest size is small; move to a small file | cli/tests/test_config.py; cli/sushitrack_cli/config.py
test_load_config_warns_once_per_legacy_key | rewrite | 1 | forbidden kind: replaces the repo's private _warn; assert through the console recorder | cli/tests/test_config.py; cli/sushitrack_cli/config.py; cli/tests/conftest.py
test_a_key_no_setting_reads_is_warned_and_ignored | rewrite | 1 | forbidden kind: replaces the private _warn; assert through the console recorder | cli/tests/test_config.py; cli/sushitrack_cli/config.py; cli/tests/conftest.py
test_an_integer_field_that_cannot_read_its_value_keeps_it_and_warns | rewrite | 1 | forbidden kind: calls private _coerce and replaces _warn; rewrite through load_config with ST_JOBS set to a non-integer | cli/tests/test_config.py; cli/sushitrack_cli/config.py
```

### `cli/tests/test_console.py`

```
test_import_does_not_build_console | rewrite | 1 | forbidden kind: reads the private _lazy.built; assert the lazy build through a public seam | cli/tests/test_console.py; cli/sushitrack_cli/console.py
test_import_outside_project_does_not_raise | delete | 1 | forbidden kind: no assertion; the promise is asserted by test_output_helper_falls_back_outside_project | cli/tests/test_console.py; cli/sushitrack_cli/console.py
test_output_helper_falls_back_outside_project | rewrite | 1 | forbidden kind: reads the private _lazy; rewrite to record the output through the console recorder | cli/tests/test_console.py; cli/sushitrack_cli/console.py; cli/tests/conftest.py
test_sources_inside_project_are_this_repos_config | rewrite | 1 | forbidden kind: reads the private _lazy.sources; rewrite through the public config layers | cli/tests/test_console.py; cli/sushitrack_cli/console.py; cli/sushitrack_cli/config.py
test_console_attribute_resolves | keep | 9 | row: console attribute resolves lazily; the oracle "is not None" is weak, a check of the type returned by current() would be stronger | cli/tests/test_console.py; cli/sushitrack_cli/console.py
test_unknown_attribute_raises_attribute_error | keep | 9 | row: an unknown module attribute raises AttributeError | cli/tests/test_console.py; cli/sushitrack_cli/console.py
test_current_returns_the_console_the_lazy_holder_builds | rewrite | 1 | forbidden kind: reads the private _lazy.get and _lazy.built | cli/tests/test_console.py; cli/sushitrack_cli/console.py
```

### `cli/tests/test_env.py`

```
test_off_windows_the_current_environment_is_used_as_is | keep | 9 | row: off Windows the environment is os.environ | cli/tests/test_env.py; cli/sushitrack_cli/env.py
test_a_cached_snapshot_is_read_back | keep | 9 | row: a cached snapshot under the same key is read | cli/tests/test_env.py; cli/sushitrack_cli/env.py
test_a_cache_from_another_vcvars_is_ignored | keep | 9 | row: a cache under another key is rebuilt | cli/tests/test_env.py; cli/sushitrack_cli/env.py
test_an_unreadable_cache_is_rebuilt_rather_than_fatal | keep | 9 | row: an unreadable cache is rebuilt | cli/tests/test_env.py; cli/sushitrack_cli/env.py
test_a_fresh_snapshot_is_written_to_the_cache | keep | 9 | row: a fresh snapshot is cached under the current key | cli/tests/test_env.py; cli/sushitrack_cli/env.py
test_no_snapshot_means_the_environment_is_left_alone | keep | 9 | row: a failed snapshot leaves the environment and writes no cache | cli/tests/test_env.py; cli/sushitrack_cli/env.py
test_a_developer_shell_is_not_snapshotted_again | keep | 9 | row: VSINSTALLDIR set skips the snapshot | cli/tests/test_env.py; cli/sushitrack_cli/env.py
test_no_visual_studio_warns_and_uses_the_current_env | keep | 9 | row: no Visual Studio warns and keeps the environment | cli/tests/test_env.py; cli/sushitrack_cli/env.py
test_a_configured_vcvars_wins_over_detection | keep | 9 | row: a configured vcvars path wins over detection | cli/tests/test_env.py; cli/sushitrack_cli/env.py
test_a_cache_folder_that_cannot_be_made_warns_and_still_returns_the_snapshot | rewrite | 1 | forbidden kind: replaces env._snapshot, a private function of this repository; stub sushicore's snapshot_vcvars instead | cli/tests/test_env.py; cli/sushitrack_cli/env.py
```

### `cli/tests/test_errors.py`

```
test_every_cli_error_derives_from_the_base_the_entry_point_catches | keep | 9 | row: every st error is a SushiCoreError the entry point catches (public exception types) | cli/tests/test_errors.py; cli/sushitrack_cli/errors.py
test_an_unknown_build_type_names_the_value_and_the_choices | keep | 9 | row: unknown build type names the value and the choices | cli/tests/test_errors.py; cli/sushitrack_cli/services/build.py
test_an_unknown_tracker_names_the_value_and_the_choices | keep | 9 | row: unknown tracker names the value and the choices | cli/tests/test_errors.py; cli/sushitrack_cli/services/label.py
```

### `cli/tests/test_library_packages.py`

```
test_the_build_asks_for_the_closure_with_the_compiler_and_the_type | rewrite | 1 | forbidden kind: the runner fixture replaces the repo-owned build_svc.load_build_env; the seam belongs at sushicore's snapshot_vcvars | cli/tests/test_library_packages.py; cli/sushitrack_cli/services/build.py; cli/sushitrack_cli/env.py; cli/sushitrack_cli/library_packages.py
test_a_minimum_size_build_links_release_packages | rewrite | 1 | same fixture; once fixed, a row of the test above with another input (Q4) | cli/tests/test_library_packages.py; cli/sushitrack_cli/services/build.py; cli/sushitrack_cli/env.py
test_the_configure_passes_each_package_the_compiler_and_no_checkout_directory | rewrite | 1 | same fixture; once fixed, split (Q3): package dirs and compiler; stamp; no legacy define | cli/tests/test_library_packages.py; cli/sushitrack_cli/services/build.py; cli/sushitrack_cli/env.py
test_the_run_dirs_are_recorded_in_the_tree_after_a_configure | rewrite | 1 | same fixture | cli/tests/test_library_packages.py; cli/sushitrack_cli/services/build.py; cli/sushitrack_cli/env.py; cli/sushitrack_cli/library_packages.py
test_a_tree_that_is_not_reconfigured_still_records_the_run_dirs | rewrite | 1 | same fixture | cli/tests/test_library_packages.py; cli/sushitrack_cli/services/build.py; cli/sushitrack_cli/env.py; cli/sushitrack_cli/library_packages.py
test_a_package_that_cannot_be_chosen_ends_the_build_in_one_error_line | rewrite | 1 | same fixture; the one-line promise is not checked (Q2), only that the message is printed | cli/tests/test_library_packages.py; cli/sushitrack_cli/services/build.py; cli/sushitrack_cli/env.py
test_the_run_dirs_lead_the_search_path_of_every_command_that_runs_the_tree | keep | 9 | row: run dirs lead LD_LIBRARY_PATH through env.load_build_env | cli/tests/test_library_packages.py; cli/sushitrack_cli/env.py; cli/sushitrack_cli/library_packages.py
test_the_windows_run_dirs_lead_path | resize | 8 | pure dict operation; the smallest size is small; move to a small file | cli/tests/test_library_packages.py; cli/sushitrack_cli/library_packages.py
test_a_tree_without_a_record_adds_nothing_to_the_path | keep | 9 | row: a tree with no record yields no run dirs; reads a folder, so medium | cli/tests/test_library_packages.py; cli/sushitrack_cli/library_packages.py
test_the_compiler_the_environment_names_outranks_the_configured_one | resize | 8 | pure: ToolSettings.resolved_compiler over field values; move to a small file; the test sets the field, not the environment | cli/tests/test_library_packages.py; cli/sushitrack_cli/config.py
test_load_tool_reads_the_stack_compiler_from_the_environment | keep | 9 | row: load_tool reads SUSHISTACK_CXX; only sushicore's loaders are stubbed | cli/tests/test_library_packages.py; cli/sushitrack_cli/config.py
```

### `cli/tests/test_main.py`

```
test_running_the_package_calls_the_cli_entry | rewrite | 1 | forbidden kind: replaces the repo-owned cli.main; rewrite as a medium run of `python -m sushitrack_cli --help` through a subprocess | cli/tests/test_main.py; cli/sushitrack_cli/__main__.py; cli/sushitrack_cli/cli.py
```

### `cli/tests/test_process.py`

```
test_run_returns_the_child_exit_code | keep | 9 | row: the child exit code is returned | cli/tests/test_process.py; cli/sushitrack_cli/process.py
test_run_echoes_the_command | keep | 9 | row: the command is echoed | cli/tests/test_process.py; cli/sushitrack_cli/process.py
test_a_missing_executable_keeps_st_exit_code | keep | 9 | row: a missing executable gives 127 and says so | cli/tests/test_process.py; cli/sushitrack_cli/process.py
test_an_interrupted_child_returns_130 | keep | 9 | row: an interrupt gives 130 and a warning; stdlib subprocess.run is stubbed, and resolve_exe checks PATH on disk, so medium | cli/tests/test_process.py; cli/sushitrack_cli/process.py
test_run_drained_returns_the_child_exit_code | keep | 9 | row: run_drained returns the child exit code | cli/tests/test_process.py; cli/sushitrack_cli/process.py
test_run_drained_reports_a_missing_executable | keep | 9 | row: run_drained reports a missing executable with 127 | cli/tests/test_process.py; cli/sushitrack_cli/process.py
test_capture_returns_the_triple_and_stays_quiet | keep | 9 | row: capture returns the triple and prints nothing | cli/tests/test_process.py; cli/sushitrack_cli/process.py
test_capture_reports_a_missing_executable_in_its_stderr | keep | 9 | row: capture reports a missing executable on stderr | cli/tests/test_process.py; cli/sushitrack_cli/process.py
test_resolve_prefers_the_path_the_caller_handed_over | rewrite | 1 | forbidden kind: control flow in body (a ternary on os.name builds the file name); rewrite with the name chosen per platform in a row table | cli/tests/test_process.py; cli/sushitrack_cli/process.py
```

### `tests/applications/benchmarks/mot_engine/test_frame_recorder.cpp`

```
Small_FrameRecorderTest.EmptyFolderRecordsNothing | keep | 9 | FrameRecorder: a recorder given no folder writes nothing and finishes with an empty path, an empty error and zero dropped | tests/applications/benchmarks/mot_engine/test_frame_recorder.cpp; applications/benchmarks/mot_engine/frame_recorder.hpp; applications/benchmarks/mot_engine/frame_recorder.cpp
Medium_FrameRecorderTest.WritesOnePeriodPerFrameWithTheRunFacts | split | 3 | Name and body join results that can break apart: the file name and dropped count; one period per frame; the environment facts. Split into three medium tests, one per promise row for FrameRecorder. The body is straight line and its fixed temp folder name is a Q8 matter for the children | tests/applications/benchmarks/mot_engine/test_frame_recorder.cpp; applications/benchmarks/mot_engine/frame_recorder.hpp; applications/benchmarks/mot_engine/frame_recorder.cpp
```

### `tests/applications/benchmarks/mot_engine/test_mot_engine_log_category.cpp`

```
Small_MotEngineLogCategoryTest.NamesTheProjectAndTheModule | delete | 1 | Forbidden kind: a log category constant. No promise row: the category has no behaviour of its own. The same test shape appears in the category tests of five other modules (checked by grep, not read whole) | tests/applications/benchmarks/mot_engine/test_mot_engine_log_category.cpp; applications/benchmarks/mot_engine/mot_engine_log_category.hpp; tests/tracking/test_tracking_log_category.cpp (grep)
```

### `tests/applications/benchmarks/mot_engine/test_mot_runner.cpp`

```
Medium_MOTRunnerTest.ProcessesEveryFrameAndWritesPredictions | split | 3 | Name and body join two results that can break apart: frames_processed equals 3, and the predictions file exists. The EXPECT_GE(mean_us(), 0.0) line cannot fail, since it is a sum of non-negative durations divided by a count, and is dropped. Split into a frame-count test (promise row: each frame handed once) and a predictions-path test (promise row: predictions go to the adapter folder) | tests/applications/benchmarks/mot_engine/test_mot_runner.cpp; applications/benchmarks/mot_engine/mot_runner.hpp; applications/benchmarks/mot_engine/mot_runner.cpp; applications/benchmarks/mot_engine/mot_sequence.hpp; applications/benchmarks/mot_engine/frame_recorder.hpp
Medium_MOTRunnerTest.MissingDetectionsAreRefused | rewrite | 7 | No forbidden kind applies, but the fixture SetUp writes seq/det/det.txt and seqinfo.ini, which this test never reads; "a fixture field this test does not use" hides the cause and the setup of the absent case. Rewrite with the sequence absent and nothing written. Promise row: a missing det.txt throws runtime_error naming it. A Q8 seam question on the rewritten test is open | tests/applications/benchmarks/mot_engine/test_mot_runner.cpp; applications/benchmarks/mot_engine/mot_runner.hpp; applications/benchmarks/mot_engine/mot_runner.cpp
Medium_MOTRunnerTest.EmptyDetectionsAreSkippedWithAWarnAndNoPerfLine | split | 3 | Name and body join three results that can break apart: one WARN; zero frames; an empty PERF line. Split into a WARN test and a zero-frames test in medium. The empty PERF line is a pure function of RunResult and belongs as a row of Small_MOTRunnerPerfLineTest | tests/applications/benchmarks/mot_engine/test_mot_runner.cpp; applications/benchmarks/mot_engine/mot_runner.hpp; applications/benchmarks/mot_engine/mot_runner.cpp; tests/captured_log.hpp; applications/benchmarks/mot_engine/mot_engine_log_category.hpp
Small_MOTRunnerPerfLineTest.NamesTheTrackerTheSequenceAndTheFrameCount | keep | 9 | perf_line: a run with frames returns the PERF line. Oracle is a specified value: 80 us over 4 frames is 20 us/frame. The name has "And", but the three fields are one string under one exact oracle, so no two results can break apart. Note for the owner: the derivation of 20 is not written beside the value, as the testing skill asks | tests/applications/benchmarks/mot_engine/test_mot_runner.cpp; applications/benchmarks/mot_engine/mot_runner.hpp; applications/benchmarks/mot_engine/mot_runner.cpp
Medium_MOTRunnerTest.ProfileRecordHoldsOnePeriodPerFrameAndLeavesPredictionsAlone | split | 3 | Name joins two results that can break apart: a record with one period per frame, and predictions that profiling does not change. Split into two medium tests. The equality test keeps the plain run as its oracle (metamorphic) | tests/applications/benchmarks/mot_engine/test_mot_runner.cpp; applications/benchmarks/mot_engine/mot_runner.hpp; applications/benchmarks/mot_engine/mot_runner.cpp; applications/benchmarks/mot_engine/frame_recorder.hpp
```

### `tests/applications/benchmarks/mot_engine/test_mot_sequence.cpp`

```
Medium_MOTSequenceTest.ReadsDetectionsByFrame | split | 3 | Several promises in one body: grouping by frame and the box and score columns; the label from column 8 with default 1; the skipping of comment, blank and short rows. Split into three promise rows. Each child is parsing logic reached only through a file, so Q8 applies to the children, with a stream seam | tests/applications/benchmarks/mot_engine/test_mot_sequence.cpp; applications/benchmarks/mot_engine/mot_sequence.hpp
Medium_MOTSequenceTest.ReadsTheFrameRateOrFallsBack | split | 3 | Two outcomes that can break apart: the frameRate value is returned, and a missing file gives the fallback. Split into a parse test and a fallback test. The fallback child duplicates the promise of FrameRateFallbackIsLogged and merges with it | tests/applications/benchmarks/mot_engine/test_mot_sequence.cpp; applications/benchmarks/mot_engine/mot_sequence.hpp
Medium_MOTSequenceTest.UnreadableDetectionsLogAWarnAndReadNothing | split | 3 | Name joins an empty result and one WARN naming the file: two results that can break apart. This is the regression test for the silent return, and its WARN child is kept as the defect's test. The empty-result child is separate | tests/applications/benchmarks/mot_engine/test_mot_sequence.cpp; applications/benchmarks/mot_engine/mot_sequence.hpp; applications/benchmarks/mot_engine/mot_engine_log_category.hpp; tests/captured_log.hpp
Medium_MOTSequenceTest.SkippedRowsAreCountedInOneWarn | resize | 8 | Promise row: read_mot_detections: skipped rows are counted in one WARN naming the file and the count. One promise decided on values (the count of short rows), reached only through a file path. read_mot_detections takes only a path, in applications/benchmarks/mot_engine/mot_sequence.hpp, so no other door exists. Seam: a stream overload, an owner's decision on the signature; the test then moves to small | tests/applications/benchmarks/mot_engine/test_mot_sequence.cpp; applications/benchmarks/mot_engine/mot_sequence.hpp; tests/captured_log.hpp; applications/benchmarks/mot_engine/mot_engine_log_category.hpp
Medium_MOTSequenceTest.FrameRateFallbackIsLogged | rewrite | 7 | Sign "assertions about different results of different calls": two calls with different inputs (a missing seqinfo.ini, a file without frameRate) and two different messages in one body. Rewrite as rows: a missing file or a missing key gives the fallback and one WARN each | tests/applications/benchmarks/mot_engine/test_mot_sequence.cpp; applications/benchmarks/mot_engine/mot_sequence.hpp; tests/captured_log.hpp; applications/benchmarks/mot_engine/mot_engine_log_category.hpp
```

### `tests/applications/tracker_bridge/test_byte_track_adapter.cpp`

```
Small_ByteTrackAdapterTest.IsNamedBytetrack | rewrite | 1 | Forbidden kind: a getter returning a constant. Its only real promise is that the name is the predictions folder the runner writes. No test checks that folder for bytetrack (the runner test checks only sushitrack). Rewrite as the runner's name row for each adapter | tests/applications/tracker_bridge/test_byte_track_adapter.cpp; applications/tracker_bridge/byte_track_adapter.hpp; applications/tracker_bridge/tracker_adapter.hpp; tests/applications/benchmarks/mot_engine/test_mot_runner.cpp
Small_ByteTrackAdapterTest.MissingSettingsKeepTheDefaults | rewrite | 1 | Forbidden kind: a small test reaches a file. The constructor reads "this/path/does/not/exist.json" through read_json_settings. The check (an empty frame gives no track) holds for any defaults, so the promise that defaults hold is not observed. Rewrite: the defaults must show in the tracks; the file reach needs a string seam (owner's decision) or the test moves to medium | tests/applications/tracker_bridge/test_byte_track_adapter.cpp; applications/tracker_bridge/byte_track_adapter.hpp; applications/tracker_bridge/json_settings.hpp
```

### `tests/applications/tracker_bridge/test_json_settings.cpp`

```
Medium_JsonSettingsTest.ReadsANumberFromAFileWithComments | resize | 8 | Promise row: read_json_settings: a file with comments parses to its document, and json_number returns the stored number. Comment stripping is a decision on values, but read_json_settings(const std::string&) in applications/tracker_bridge/json_settings.hpp offers only a path. Seam: parse from a string or stream (owner's decision); then the test moves to small | tests/applications/tracker_bridge/test_json_settings.cpp; applications/tracker_bridge/json_settings.hpp; applications/tracker_bridge/tracker_bridge_log_category.hpp
Small_JsonSettingsTest.MissingFileReadsAsAnEmptyObject | rewrite | 1 | Forbidden kind: a small test reads a file that does not exist. Only test of read_json_settings: missing file reads as an empty object. Rewrite on a string seam (see the ReadsANumber record) or move to medium | tests/applications/tracker_bridge/test_json_settings.cpp; applications/tracker_bridge/json_settings.hpp; applications/tracker_bridge/tracker_bridge_log_category.hpp
Small_JsonSettingsTest.AbsentOrNonNumericKeyGivesTheFallback | rewrite | 7 | Sign "assertions about different results of different calls": two inputs and two fallbacks in one body. The non-numeric case also logs a WARN this test does not capture. Rewrite as rows: an absent key gives the fallback; a non-numeric value gives it after one WARN (the WARN row is in json_number) | tests/applications/tracker_bridge/test_json_settings.cpp; applications/tracker_bridge/json_settings.hpp
Small_JsonSettingsTest.UnusableSettingsAreLogged | rewrite | 1 | Forbidden kind: a small test reads a missing file and installs the logger sink through CapturedLog, which is process-wide. Three WARN promises in one body, to split on rewrite. The CapturedLog question is open for the owner (see the report line on it) | tests/applications/tracker_bridge/test_json_settings.cpp; applications/tracker_bridge/json_settings.hpp; applications/tracker_bridge/tracker_bridge_log_category.hpp; tests/captured_log.hpp
```

### `tests/applications/tracker_bridge/test_mot_detection.cpp`

```
Small_MOTDetectionTest.DefaultsToAnEmptyClassOneBox | delete | 1 | Forbidden kind: a default value of a plain struct, with no behaviour. No promise row. The label default that the reader relies on is set by read_mot_detections and checked where it is parsed (ReadsDetectionsByFrame) | tests/applications/tracker_bridge/test_mot_detection.cpp; applications/tracker_bridge/mot_detection.hpp; tests/applications/benchmarks/mot_engine/test_mot_sequence.cpp
```

### `tests/applications/tracker_bridge/test_oc_sort_adapter.cpp`

```
Small_OCSORTAdapterTest.IsNamedOcsort | rewrite | 1 | Forbidden kind: a getter returning a constant. Only real promise is the folder name; no test checks the ocsort folder. Rewrite as the runner's name row for each adapter | tests/applications/tracker_bridge/test_oc_sort_adapter.cpp; applications/tracker_bridge/oc_sort_adapter.hpp; applications/tracker_bridge/tracker_adapter.hpp; tests/applications/benchmarks/mot_engine/test_mot_runner.cpp
Small_OCSORTAdapterTest.EmptyFrameGivesNoTrack | merge | 4 | Same promise as the SushiTrack copy, with another adapter. Merge with tests/applications/tracker_bridge/test_tracker_adapter.cpp::Small_TrackerAdapterTest (the parametrised contract, row "an empty frame reports no track"). The SushiTrack copy, in tests/applications/tracker_bridge/test_sushitrack_adapter.cpp, is a rewrite, so the contract becomes the one place this is checked | tests/applications/tracker_bridge/test_oc_sort_adapter.cpp; applications/tracker_bridge/oc_sort_adapter.hpp; tests/applications/tracker_bridge/test_sushitrack_adapter.cpp; tests/applications/tracker_bridge/test_tracker_adapter.cpp
```

### `tests/applications/tracker_bridge/test_sushitrack_adapter.cpp`

```
Small_SushiTrackAdapterTest.IsNamedSushitrack | delete | 1 | Forbidden kind: a getter returning a constant. Its promise (the folder is named sushitrack) is tested elsewhere: test_mot_runner.cpp ProcessesEveryFrameAndWritesPredictions reads pred/sushitrack/seq.txt. The constructor also reads the file named by SUSHITRACK_CONFIG, which a small test may not touch | tests/applications/tracker_bridge/test_sushitrack_adapter.cpp; applications/tracker_bridge/sushitrack_adapter.hpp; tests/applications/benchmarks/mot_engine/test_mot_runner.cpp; source/configuration/config_loader.cpp
Small_SushiTrackAdapterTest.EmptyFrameGivesNoTrack | rewrite | 1 | Forbidden kind: a small test reaches a file and the environment, because SushiTrackAdapter() resolves its parameters from SUSHITRACK_CONFIG (config_loader.cpp line 63 is the path-then-environment rule). The promise is not checked for this adapter anywhere else. Rewrite as a contract row, with the parameters given to the adapter (owner's decision on the constructor) | tests/applications/tracker_bridge/test_sushitrack_adapter.cpp; applications/tracker_bridge/sushitrack_adapter.hpp; source/configuration/config_loader.cpp; tests/applications/tracker_bridge/test_tracker_adapter.cpp
Small_SushiTrackAdapterTest.ReportedTrackFollowsItsDetection | rewrite | 1 | Forbidden kind: a bare tolerance literal, EXPECT_NEAR(..., 1.f) twice; the tolerance must be derived. Also a small test that reads the environment-named settings file and runs a 10-frame loop in the body. Only test of the promise. The repository has no tolerance helper under tests/ (checked by listing tests/ and by find for "tolerance"). Rewrite with a derived tolerance and a builder for the frames | tests/applications/tracker_bridge/test_sushitrack_adapter.cpp; applications/tracker_bridge/sushitrack_adapter.hpp; applications/tracker_bridge/mot_detection.hpp; source/configuration/config_loader.cpp
```

### `tests/applications/tracker_bridge/test_track_result.cpp`

```
Small_TrackResultTest.BraceInitialisationFollowsTheFieldOrder | delete | 1 | Forbidden kind: trivial code, the field order of a plain struct, with no behaviour and no caller that brace-initialises TrackResult. The comment's claim "as the bridge does" is false: tracker_bridge.cpp brace-initialises CTrackResult (line 154) and MOTDetection (line 141), not TrackResult | tests/applications/tracker_bridge/test_track_result.cpp; applications/tracker_bridge/track_result.hpp; applications/tracker_bridge/tracker_bridge.cpp
```

### `tests/applications/tracker_bridge/test_tracker_adapter.cpp`

```
Small_TrackerAdapterTest.SteadyObjectKeepsOneIdentity | rewrite | 1 | Forbidden kind: the SushiTrack factory reads the settings file named by SUSHITRACK_CONFIG in a small test. Also Q7: a 20-frame loop in the body, which the straight-line rule forbids unless a named builder holds it, and an extra check that the name is non-empty, which is not this promise. Rewrite: the loop moves into a named helper, the factory gets explicit parameters, the name check goes | tests/applications/tracker_bridge/test_tracker_adapter.cpp; applications/tracker_bridge/tracker_adapter.hpp; applications/tracker_bridge/sushitrack_adapter.hpp; applications/tracker_bridge/byte_track_adapter.hpp; applications/tracker_bridge/oc_sort_adapter.hpp; source/configuration/config_loader.cpp
```

### `tests/applications/tracker_bridge/test_tracker_bridge.cpp`

```
Small_TrackerBridgeTest.BuildsEveryRegisteredTracker | rewrite | 1 | Forbidden kind: a small test reaches the process-wide TrackerRegistry through create_tracker, and each tracker reads the environment-named settings file. A loop over names in the body. Two promises in one body (registered names build; unknown or null names give NULL). Only test of the promise. Rewrite as rows over names, with an owned registry (owner's decision) or in medium | tests/applications/tracker_bridge/test_tracker_bridge.cpp; applications/tracker_bridge/tracker_bridge.h; applications/tracker_bridge/tracker_bridge.cpp; applications/tracker_bridge/tracker_registry.hpp; applications/tracker_bridge/sushitrack_adapter.hpp; source/configuration/config_loader.cpp
Small_TrackerBridgeTest.ProcessWritesTheTracksWithinTheCapacity | rewrite | 1 | Forbidden kind: process-wide registry and environment-named settings file, reached through create_tracker("sushitrack"). A 10-frame loop in the body. Rewrite: tracker_process_c takes a void* tracker, so the test can hand it an OCSORTAdapter() (no file, no environment, no registry) and a 10-frame builder | tests/applications/tracker_bridge/test_tracker_bridge.cpp; applications/tracker_bridge/tracker_bridge.h; applications/tracker_bridge/tracker_bridge.cpp; applications/tracker_bridge/oc_sort_adapter.hpp
Small_TrackerBridgeTest.NullResultsAreRefused | rewrite | 1 | Forbidden kind: process-wide registry reached through create_tracker, which is setup the promise does not need. The refusal of null results needs only a tracker pointer, so the rewrite passes an OCSORTAdapter() as in ProcessWrites. Log check stays with the logger question open | tests/applications/tracker_bridge/test_tracker_bridge.cpp; applications/tracker_bridge/tracker_bridge.h; applications/tracker_bridge/tracker_bridge.cpp; applications/tracker_bridge/oc_sort_adapter.hpp; tests/captured_log.hpp
Small_TrackerBridgeTest.RefusedNamesAreLogged | rewrite | 1 | Forbidden kind: the process-wide registry, read by create_tracker and by registered_list, plus the logger sink. Only test of the promise that an unknown or null name logs one ERROR listing the names. Rewrite on an owned registry, with the log check in medium | tests/applications/tracker_bridge/test_tracker_bridge.cpp; applications/tracker_bridge/tracker_bridge.cpp; applications/tracker_bridge/tracker_registry.hpp; tests/captured_log.hpp
Small_TrackerBridgeTest.TruncationIsReportedByStatusAndLog | rewrite | 1 | Forbidden kind: process-wide registry and environment-named settings file through create_tracker. Also the test relies on SushiTrack confirming two tracks within three frames, a tracker-internal timing, and checks the WARN with EXPECT_GE, which is weaker than the message it names. Rewrite with a tracker that yields two tracks, asserted exactly (EXPECT_EQ on the WARN count) | tests/applications/tracker_bridge/test_tracker_bridge.cpp; applications/tracker_bridge/tracker_bridge.cpp; applications/tracker_bridge/tracker_registry.hpp; applications/tracker_bridge/sushitrack_adapter.hpp; source/configuration/config_loader.cpp
Small_TrackerBridgeTest.EmptyFrameRunsAndReturnsOk | rewrite | 1 | Forbidden kind: process-wide registry and environment-named settings file through create_tracker("sushitrack"). Only test of the promise that an empty frame returns OK with zero tracks. Rewrite with OCSORTAdapter() as the tracker (see ProcessWrites) | tests/applications/tracker_bridge/test_tracker_bridge.cpp; applications/tracker_bridge/tracker_bridge.cpp; applications/tracker_bridge/oc_sort_adapter.hpp; applications/tracker_bridge/sushitrack_adapter.hpp; source/configuration/config_loader.cpp
Small_TrackerBridgeTest.NegativeCountsAreRefused | rewrite | 1 | Forbidden kind: process-wide registry and environment-named settings file through create_tracker("sushitrack"). The refusal needs only a tracker pointer. Rewrite with OCSORTAdapter() as the tracker; the status, zero count and ERROR are the promise | tests/applications/tracker_bridge/test_tracker_bridge.cpp; applications/tracker_bridge/tracker_bridge.cpp; applications/tracker_bridge/oc_sort_adapter.hpp; applications/tracker_bridge/sushitrack_adapter.hpp; source/configuration/config_loader.cpp
```

### `tests/applications/tracker_bridge/test_tracker_bridge_log_category.cpp`

```
Small_TrackerBridgeLogCategoryTest.NamesTheProjectAndTheModule | delete | 1 | Forbidden kind: a log category constant, with no behaviour of its own. No promise row | tests/applications/tracker_bridge/test_tracker_bridge_log_category.cpp; applications/tracker_bridge/tracker_bridge_log_category.hpp
```

### `tests/applications/tracker_bridge/test_tracker_registry.cpp`

```
Small_TrackerRegistryTest.FindsANameInAnyCase | rewrite | 1 | Forbidden kinds: the process-wide TrackerRegistry::get_instance, whose constructor is private so the test cannot own an instance (owner's decision on a public constructor); and SilentAdapter, a stub of TrackerAdapter, which the repository owns. Only test of the promise (lookup folds case). Rewrite on an owned instance with a real adapter (OCSORTAdapter) | tests/applications/tracker_bridge/test_tracker_registry.cpp; applications/tracker_bridge/tracker_registry.hpp; applications/tracker_bridge/tracker_adapter.hpp; applications/tracker_bridge/oc_sort_adapter.hpp
Small_TrackerRegistryTest.UnknownNameBuildsNothing | rewrite | 1 | Forbidden kind: the process-wide registry. The C API tests that also check an unknown name are rewrites too, so no kept test covers this promise: rewrite, not delete. Rewrite on an owned instance | tests/applications/tracker_bridge/test_tracker_registry.cpp; applications/tracker_bridge/tracker_registry.hpp
Small_TrackerRegistryTest.RegisteringANameTwiceWarnsAndNamesAreSorted | rewrite | 1 | Forbidden kinds: the process-wide registry, whose registrations persist across tests (state left behind); the SilentAdapter stub; the logger sink. Two promises in one body (a WARN; sorted names). Rewrite on an owned instance with real adapters, split into a warn test and a sort test | tests/applications/tracker_bridge/test_tracker_registry.cpp; applications/tracker_bridge/tracker_registry.hpp; applications/tracker_bridge/tracker_adapter.hpp; tests/captured_log.hpp
```

### `tests/association/test_assignment_solver.cpp`

```
Small_AssignmentSolverTest.SolvesThroughTheInterface | keep | 9 | AssignmentSolver: a cheap off-diagonal pairing is matched off the diagonal | tests/association/test_assignment_solver.cpp
Small_AssignmentSolverTest.LeavesPairsAboveTheThresholdUnmatched | delete | 4 | sibling tests/association/test_lapjv_solver.cpp::Small_LAPJVSolverTest.OneByOneMatchAboveThreshold checks the same promise at the same size, on both sides | tests/association/test_assignment_solver.cpp; tests/association/test_lapjv_solver.cpp
```

### `tests/association/test_association_log_category.cpp`

```
Small_AssociationLogCategoryTest.NamesTheProjectAndTheModule | delete | 1 | forbidden kind: a log category, a constant with no logic behind it | tests/association/test_association_log_category.cpp; tests/captured_log.hpp
```

### `tests/association/test_cosine_cost_calculator.cpp`

```
Small_CosineCostCalculatorTest.IdenticalFeaturesCostIsZero | keep | 9 | CosineCostCalculator: identical features cost 0 | tests/association/test_cosine_cost_calculator.cpp; tests/tracklet_builders.hpp
Small_CosineCostCalculatorTest.OrthogonalFeaturesCostIsOne | keep | 9 | CosineCostCalculator: orthogonal features cost 1 | tests/association/test_cosine_cost_calculator.cpp; tests/tracklet_builders.hpp
Small_CosineCostCalculatorTest.MissingFeatureOnAProducesCostOne | merge | 4 | merge with Small_CosineCostCalculatorTest.MissingFeatureOnBProducesCostOne: same promise, other side, same size, one row table | tests/association/test_cosine_cost_calculator.cpp; tests/tracklet_builders.hpp
Small_CosineCostCalculatorTest.SingleMatchesBatch | rewrite | 6 | oracle is calculate_single, another path of the unit under test; rewrite with the hand value 0 for identical features | tests/association/test_cosine_cost_calculator.cpp; tests/tracklet_builders.hpp
Small_CosineCostCalculatorTest.MissingFeatureOnBProducesCostOne | merge | 4 | merge with Small_CosineCostCalculatorTest.MissingFeatureOnAProducesCostOne | tests/association/test_cosine_cost_calculator.cpp; tests/tracklet_builders.hpp
```

### `tests/association/test_cost_calculator.cpp`

```
Small_CostCalculatorTest.GivesOneRowPerTrackAndOneColumnPerDetection | keep | 9 | CostCalculator contract: 2 tracks by 3 detections gives a 2 x 3 matrix, for all ten calculators (one parametrised line) | tests/association/test_cost_calculator.cpp; tests/tracklet_builders.hpp
```

### `tests/association/test_cost_matrix.cpp`

```
Small_CostMatrixTest.DefaultConstructionIsEmpty | delete | 1 | forbidden kind: a default value; the empty state is tested by ResizeToZeroProducesEmptyMatrix | tests/association/test_cost_matrix.cpp
Small_CostMatrixTest.ResizeDefaultFillsSentinel | keep | 9 | CostMatrix: resize fills 1e4 by default; the sentinel is the solver's contract | tests/association/test_cost_matrix.cpp
Small_CostMatrixTest.ResizeExplicitFillValue | delete | 4 | sibling tests/association/test_cost_matrix.cpp::Small_CostMatrixTest.GrowResizeReinitialises checks the explicit fill at a larger size (5 x 5) | tests/association/test_cost_matrix.cpp
Small_CostMatrixTest.ElementAccessReadWrite | keep | 9 | CostMatrix: a written cell reads back | tests/association/test_cost_matrix.cpp
Small_CostMatrixTest.ConstAccessorReadsCorrectly | keep | 9 | CostMatrix: the const accessor reads the same storage | tests/association/test_cost_matrix.cpp
Small_CostMatrixTest.EmptyFalseAfterResize | delete | 4 | sibling tests/association/test_cost_matrix.cpp::Small_CostMatrixTest.ResizeDefaultFillsSentinel asserts the same non-empty state at 3 x 4 | tests/association/test_cost_matrix.cpp
Small_CostMatrixTest.ShrinkResizeReinitialises | keep | 9 | CostMatrix: shrinking reinitialises every active cell | tests/association/test_cost_matrix.cpp
Small_CostMatrixTest.GrowResizeReinitialises | keep | 9 | CostMatrix: growing reinitialises every cell to the requested value | tests/association/test_cost_matrix.cpp
Small_CostMatrixTest.DataPointerIsValid | keep | 9 | CostMatrix: data() is non-null after resize and indexes row-major | tests/association/test_cost_matrix.cpp
Small_CostMatrixTest.ConstDataPointerMatchesAccessor | keep | 9 | CostMatrix: the const data() overload reads the same buffer | tests/association/test_cost_matrix.cpp
Small_CostMatrixTest.ResizeToZeroProducesEmptyMatrix | keep | 9 | CostMatrix: resize to 0 x 0 is empty | tests/association/test_cost_matrix.cpp
Small_CostMatrixTest.SingleRowMatrixAccessAllCells | merge | 4 | merge with SingleColumnMatrixAccessAllCells into one row table of degenerate shapes | tests/association/test_cost_matrix.cpp
Small_CostMatrixTest.SingleColumnMatrixAccessAllCells | merge | 4 | merge with SingleRowMatrixAccessAllCells into one row table of degenerate shapes | tests/association/test_cost_matrix.cpp
Small_CostMatrixTest.DataPointerRowMajorNonSquare | keep | 9 | CostMatrix: row-major index for a 3 x 5 matrix | tests/association/test_cost_matrix.cpp
```

### `tests/association/test_diou_cost_calculator.cpp`

```
Small_DIoUCostCalculatorTest.IdenticalBoxesCostIsZero | keep | 9 | DIoU: identical boxes cost 0 | tests/association/test_diou_cost_calculator.cpp; tests/tracklet_builders.hpp
Small_DIoUCostCalculatorTest.SeparatedBoxesCostPositive | keep | 9 | DIoU: separated boxes cost more than 0 (weak bound, kept) | tests/association/test_diou_cost_calculator.cpp; tests/tracklet_builders.hpp
Small_DIoUCostCalculatorTest.SingleMatchesBatch | rewrite | 6 | oracle is calculate_single, another path of the unit; rewrite with a hand-computed DIoU value for the 60-pixel-shift pair | tests/association/test_diou_cost_calculator.cpp; tests/tracklet_builders.hpp
Small_DIoUCostCalculatorTest.ContainedMisalignedBoxCostGreaterThanIoUCost | keep | 9 | metamorphic relation: DIoU cost above IoU cost for a misaligned contained box | tests/association/test_diou_cost_calculator.cpp; tests/tracklet_builders.hpp
```

### `tests/association/test_fuse_score_decorator.cpp`

```
Small_FuseScoreDecoratorTest.HighScoreLowersCost | keep | 9 | FuseScore: cost = 1 - (1 - inner) * score; hand value 0.2 | tests/association/test_fuse_score_decorator.cpp; tests/tracklet_builders.hpp
Small_FuseScoreDecoratorTest.ScoreOnePreservesCost | keep | 9 | metamorphic relation: score 1 leaves the inner cost unchanged | tests/association/test_fuse_score_decorator.cpp; tests/tracklet_builders.hpp
Small_FuseScoreDecoratorTest.SentinelCellsPreserved | keep | 9 | FuseScore: sentinel cells stay at or above 1e3 | tests/association/test_fuse_score_decorator.cpp; tests/tracklet_builders.hpp
Small_FuseScoreDecoratorTest.SingleMatchesBatch | rewrite | 6 | oracle is calculate_single, another path of the unit; rewrite with the hand value 1 - (1 - c) * 0.7, c being the hand-computed IoU of the 30-pixel shift | tests/association/test_fuse_score_decorator.cpp; tests/tracklet_builders.hpp
Small_FuseScoreDecoratorTest.ScoreZeroProducesCostOne | keep | 9 | FuseScore: score 0 gives cost 1 | tests/association/test_fuse_score_decorator.cpp; tests/tracklet_builders.hpp
```

### `tests/association/test_fused_cost_calculator.cpp`

```
Small_FusedCostCalculatorTest.NullGateIdenticalFeaturesDirectAssignment | keep | 9 | Fused: identical features under a null gate take the direct path, cost 0 | tests/association/test_fused_cost_calculator.cpp; tests/tracklet_builders.hpp
Small_FusedCostCalculatorTest.NullGateSentinelPreservedWhenGateFails | rewrite | 7 | the name says NullGate but the gate is a KalmanFilter; rename to state the gated-pair sentinel; the body is otherwise sound | tests/association/test_fused_cost_calculator.cpp; tests/tracklet_builders.hpp; tests/gating_log_fixture.hpp
Small_FusedCostCalculatorTest.EmptyCollectionProducesEmptyMatrix | keep | 9 | Fused: empty collection gives empty matrix | tests/association/test_fused_cost_calculator.cpp; tests/tracklet_builders.hpp
Small_FusedCostCalculatorTest.SingleMatchesBatch | rewrite | 6 | oracle is calculate_single, another path of the unit; rewrite with a hand value for the 30-pixel pair | tests/association/test_fused_cost_calculator.cpp; tests/tracklet_builders.hpp
Small_FusedCostCalculatorLogTest.LogsEveryGatedPairOfACallInOneLine | rewrite | 2 | the one-line promise has no other test; EXPECT_LE(lines, 1) accepts zero lines, and TRACE is compiled out in release, so the test passes without the promise; assert one line in TRACE builds and GTEST_SKIP otherwise | tests/association/test_fused_cost_calculator.cpp; tests/gating_log_fixture.hpp; tests/captured_log.hpp; source/association/fused_cost_calculator.cpp
```

### `tests/association/test_gated_iou_cost_calculator.cpp`

```
Small_GatedIoUCostCalculatorTest.NullGatePassesAllCellsFillsIoUCost | keep | 9 | GatedIoU: null gate passes, identical boxes cost 0 | tests/association/test_gated_iou_cost_calculator.cpp; tests/tracklet_builders.hpp
Small_GatedIoUCostCalculatorTest.NullGateNonOverlappingProducesCostOne | keep | 9 | GatedIoU: null gate, disjoint boxes cost 1 | tests/association/test_gated_iou_cost_calculator.cpp; tests/tracklet_builders.hpp
Small_GatedIoUCostCalculatorTest.FailedGateProducesSentinel | keep | 9 | GatedIoU: a failed gate gives a sentinel | tests/association/test_gated_iou_cost_calculator.cpp; tests/tracklet_builders.hpp
Small_GatedIoUCostCalculatorTest.EmptyCollectionProducesEmptyMatrix | keep | 9 | GatedIoU: empty collection gives empty matrix | tests/association/test_gated_iou_cost_calculator.cpp; tests/tracklet_builders.hpp
Small_GatedIoUCostCalculatorTest.SingleMatchesBatch | rewrite | 6 | oracle is calculate_single, another path of the unit; rewrite with a hand value for the 30-pixel pair | tests/association/test_gated_iou_cost_calculator.cpp; tests/tracklet_builders.hpp
Small_GatedIoUCostCalculatorLogTest.LogsEveryGatedPairOfACallInOneLine | rewrite | 2 | the one-line promise has no other test; EXPECT_LE(lines, 1) accepts zero lines, so the test passes with the logging removed; assert one line in TRACE builds and GTEST_SKIP otherwise | tests/association/test_gated_iou_cost_calculator.cpp; tests/gating_log_fixture.hpp; tests/captured_log.hpp; source/association/fused_cost_calculator.cpp
```

### `tests/association/test_giou_cost_calculator.cpp`

```
Small_GIoUCostCalculatorTest.IdenticalBoxesCostIsZero | keep | 9 | GIoU: identical boxes cost 0 | tests/association/test_giou_cost_calculator.cpp; tests/tracklet_builders.hpp
Small_GIoUCostCalculatorTest.SeparatedBoxesCostPositive | keep | 9 | GIoU: separated boxes cost more than 0 (weak bound, kept) | tests/association/test_giou_cost_calculator.cpp; tests/tracklet_builders.hpp
Small_GIoUCostCalculatorTest.SingleMatchesBatch | rewrite | 6 | oracle is calculate_single, another path of the unit; rewrite with a hand-computed GIoU value | tests/association/test_giou_cost_calculator.cpp; tests/tracklet_builders.hpp
```

### `tests/association/test_iou_cost_calculator.cpp`

```
Small_IoUCostCalculatorTest.IdenticalBoxesCostIsZero | keep | 9 | IoU: identical boxes cost 0 | tests/association/test_iou_cost_calculator.cpp; tests/tracklet_builders.hpp
Small_IoUCostCalculatorTest.NonOverlappingBoxesCostIsOne | keep | 9 | IoU: disjoint boxes cost 1 | tests/association/test_iou_cost_calculator.cpp; tests/tracklet_builders.hpp
Small_IoUCostCalculatorTest.PartialOverlapCostInRange | keep | 9 | IoU: partial overlap strictly between 0 and 1; the range is the promise as named, the exact 2/3 is not pinned; kept | tests/association/test_iou_cost_calculator.cpp; tests/tracklet_builders.hpp
Small_IoUCostCalculatorTest.AreaRatioViolationIsSentinel | keep | 9 | IoU: area ratio above 4 gives the sentinel | tests/association/test_iou_cost_calculator.cpp; tests/tracklet_builders.hpp
Small_IoUCostCalculatorTest.EmptyBCollectionProducesEmptyMatrix | keep | 9 | IoU: empty B collection gives empty matrix | tests/association/test_iou_cost_calculator.cpp; tests/tracklet_builders.hpp
Small_IoUCostCalculatorTest.SingleMatchesBatch | rewrite | 6 | oracle is calculate_single, another path of the unit; rewrite with the hand value of the 50-pixel overlap pair | tests/association/test_iou_cost_calculator.cpp; tests/tracklet_builders.hpp
```

### `tests/association/test_lapjv.cpp`

```
Small_LAPJVTest.FindsTheMinimumCostPermutation | keep | 9 | LAPJV: the 3 x 3 minimum permutation {1, 2, 0} with inverse solutions | tests/association/test_lapjv.cpp; include/SushiTrack/association/lapjv.hpp
Small_LAPJVTest.WorkspaceResizeOnlyGrows | rewrite | 5 | the test reads the five member vectors' sizes of a public struct; rewrite against the behaviour: after resize(5) then resize(2), lapjv_internal solves a 5 x 5 problem correctly | tests/association/test_lapjv.cpp; include/SushiTrack/association/lapjv.hpp
```

### `tests/association/test_lapjv_solver.cpp`

```
Small_LAPJVSolverTest.EmptyMatrixAllUnmatched | keep | 9 | LAPJVSolver: a 0 x 0 matrix gives no matches and no unmatched indices | tests/association/test_lapjv_solver.cpp
Small_LAPJVSolverTest.OneByOneMatchBelow | keep | 9 | LAPJVSolver: a 1 x 1 cost below threshold matches | tests/association/test_lapjv_solver.cpp
Small_LAPJVSolverTest.OneByOneMatchAboveThreshold | keep | 9 | LAPJVSolver: a 1 x 1 cost above threshold is unmatched on both sides | tests/association/test_lapjv_solver.cpp; tests/association/test_assignment_solver.cpp
Small_LAPJVSolverTest.IdentityMatrixPerfectDiagonalMatch | keep | 9 | LAPJVSolver: identity matrix gives the diagonal, each column once | tests/association/test_lapjv_solver.cpp
Small_LAPJVSolverTest.MoreRowsThanColsSurplusRowsUnmatched | merge | 4 | merge with MoreColsThanRowsSurplusColsUnmatched: surplus on the larger side is unmatched, one row table | tests/association/test_lapjv_solver.cpp
Small_LAPJVSolverTest.MoreColsThanRowsSurplusColsUnmatched | merge | 4 | merge with MoreRowsThanColsSurplusRowsUnmatched | tests/association/test_lapjv_solver.cpp
Small_LAPJVSolverTest.ThresholdControlsAcceptedAssignments | keep | 9 | LAPJVSolver: the threshold decides which matches are accepted | tests/association/test_lapjv_solver.cpp
Small_LAPJVSolverTest.NoDoubleAssignments | delete | 4 | sibling Small_LAPJVSolverTest.IdentityMatrixPerfectDiagonalMatch checks the same column uniqueness at n = 4 with the size asserted; this one asserts no size, so an empty result passes | tests/association/test_lapjv_solver.cpp
Small_LAPJVSolverTest.RepeatedCallsProduceConsistentResults | rewrite | 6 | the oracle for the second call is the first call, which is the code under test; the test compares sizes only; rewrite to compare each pair against the diagonal oracle on both calls | tests/association/test_lapjv_solver.cpp
Small_LAPJVSolverTest.AllSentinelMatrixProducesNoMatches | keep | 9 | LAPJVSolver: all-sentinel matrix gives no matches | tests/association/test_lapjv_solver.cpp
Small_LAPJVSolverTest.OneRowAllAboveThresholdNoMatch | keep | 9 | LAPJVSolver: a row above threshold everywhere is unmatched | tests/association/test_lapjv_solver.cpp
Small_LAPJVSolverTest.PermutationMatrixMatchesOptimally | keep | 9 | LAPJVSolver: the hand-derived permutation of a 3 x 3 cost | tests/association/test_lapjv_solver.cpp
Small_LAPJVSolverTest.EmptyMatrixWithCountsLogsAnError | resize | 8 | the log is captured from the process-wide SushiLog sink (tests/captured_log.hpp); small forbids a process-wide instance; the seam is a logger passed into the solver; medium until it exists | tests/association/test_lapjv_solver.cpp; tests/captured_log.hpp
Small_LAPJVSolverTest.NonFiniteCostLogsOneWarnAndStaysUnmatched | rewrite | 2 | the stays-unmatched part is never asserted: EXPECT_LE(matches.size(), 2) cannot fail on a 2 x 2 matrix, and pair (0, 0) is never checked absent; the name joins two promises, so split the once-logged WARN from the match absence | tests/association/test_lapjv_solver.cpp; tests/captured_log.hpp
```

### `tests/association/test_observation_momentum_decorator.cpp`

```
Small_ObservationMomentumDecoratorTest.AlignedDetectionGetsLowerCost | keep | 9 | Momentum: motion ahead lowers 0.5 to 0.3 | tests/association/test_observation_momentum_decorator.cpp; tests/tracklet_builders.hpp
Small_ObservationMomentumDecoratorTest.OpposingDetectionGetsHigherCost | keep | 9 | Momentum: motion behind raises 0.5 to 0.7 | tests/association/test_observation_momentum_decorator.cpp; tests/tracklet_builders.hpp
Small_ObservationMomentumDecoratorTest.PerpendicularDetectionUnchanged | keep | 9 | Momentum: perpendicular motion leaves 0.5 | tests/association/test_observation_momentum_decorator.cpp; tests/tracklet_builders.hpp
Small_ObservationMomentumDecoratorTest.SentinelPairsArePreserved | keep | 9 | Momentum: a sentinel passes through | tests/association/test_observation_momentum_decorator.cpp; tests/tracklet_builders.hpp
Small_ObservationMomentumDecoratorTest.SlowTrackIsGatedOff | keep | 9 | Momentum: a track below min velocity adds nothing | tests/association/test_observation_momentum_decorator.cpp; tests/tracklet_builders.hpp
Small_ObservationMomentumDecoratorTest.StaleTrackIsGatedOff | keep | 9 | Momentum: a track older than max_age adds nothing | tests/association/test_observation_momentum_decorator.cpp; tests/tracklet_builders.hpp
Small_ObservationMomentumDecoratorTest.TrackWithoutVelocityIsGatedOff | keep | 9 | Momentum: a track with one observation adds nothing | tests/association/test_observation_momentum_decorator.cpp; tests/tracklet_builders.hpp
Small_ObservationMomentumDecoratorTest.SingleMatchesBatch | rewrite | 6 | oracle is calculate_single, another path of the unit; the batch value 0.3 is already hand-derived in AlignedDetectionGetsLowerCost, so rewrite against 0.3 | tests/association/test_observation_momentum_decorator.cpp; tests/tracklet_builders.hpp
Small_ObservationMomentumDecoratorTest.ManeuverScaleDoesNotAffectCruisingTrack | keep | 9 | Momentum: maneuver scale leaves a Cruising track at 0.3 | tests/association/test_observation_momentum_decorator.cpp; tests/tracklet_builders.hpp
```

### `tests/association/test_pure_reid_cost_calculator.cpp`

```
Small_PureReIDCostCalculatorTest.NullGateIdenticalFeaturesPassesThreshold | keep | 9 | PureReID: identical features under a null gate cost 0 | tests/association/test_pure_reid_cost_calculator.cpp; tests/tracklet_builders.hpp
Small_PureReIDCostCalculatorTest.NullGateOrthogonalFeaturesExceedsThreshold | keep | 9 | PureReID: orthogonal features exceed the threshold to the sentinel | tests/association/test_pure_reid_cost_calculator.cpp; tests/tracklet_builders.hpp
Small_PureReIDCostCalculatorTest.FailedGateProducesSentinel | keep | 9 | PureReID: a failed gate gives a sentinel | tests/association/test_pure_reid_cost_calculator.cpp; tests/tracklet_builders.hpp
Small_PureReIDCostCalculatorTest.EmptyCollectionProducesEmptyMatrix | keep | 9 | PureReID: empty collection gives empty matrix | tests/association/test_pure_reid_cost_calculator.cpp; tests/tracklet_builders.hpp
Small_PureReIDCostCalculatorTest.SingleMatchesBatch | rewrite | 6 | oracle is calculate_single, another path of the unit; rewrite with the hand value of the identical-feature pair | tests/association/test_pure_reid_cost_calculator.cpp; tests/tracklet_builders.hpp
Small_PureReIDCostCalculatorLogTest.LogsEveryGatedPairOfACallInOneLine | rewrite | 2 | the one-line promise has no other test; EXPECT_LE(lines, 1) accepts zero lines, and TRACE is compiled out in release; assert one line in TRACE builds and GTEST_SKIP otherwise | tests/association/test_pure_reid_cost_calculator.cpp; tests/gating_log_fixture.hpp; tests/captured_log.hpp; source/association/fused_cost_calculator.cpp
```

### `tests/association/test_reid_first_cost_calculator.cpp`

```
Small_ReIDFirstCostCalculatorTest.DirectAssignmentWhenBothHaveFeaturesAndCloseReID | keep | 9 | ReIDFirst: identical features with close ReID take the direct path, cost 0 | tests/association/test_reid_first_cost_calculator.cpp; tests/tracklet_builders.hpp; source/association/reid_first_cost_calculator.cpp
Small_ReIDFirstCostCalculatorTest.BlendedPathWhenNoFeatures | rewrite | 2 | the assertion 0 <= c <= 1 accepts any value in range; with no features the code returns the IoU cost (source line 140) unless require_reid is set (line 133), so the blended formula is never reached; the name claims a path the test does not reach | tests/association/test_reid_first_cost_calculator.cpp; source/association/reid_first_cost_calculator.cpp
Small_ReIDFirstCostCalculatorTest.MissingFeatureOnBProducesFallback | rewrite | 2 | the assertion c >= 0 accepts any non-negative value; the fallback is the IoU cost (source line 140), 0 for identical boxes unless require_reid is set; pin that value | tests/association/test_reid_first_cost_calculator.cpp; source/association/reid_first_cost_calculator.cpp
Small_ReIDFirstCostCalculatorTest.EmptyCollectionProducesEmptyMatrix | keep | 9 | ReIDFirst: empty collection gives empty matrix | tests/association/test_reid_first_cost_calculator.cpp; tests/tracklet_builders.hpp
Small_ReIDFirstCostCalculatorTest.SingleMatchesBatch | rewrite | 6 | oracle is calculate_single, another path of the unit; rewrite with the hand value of the identical-feature pair | tests/association/test_reid_first_cost_calculator.cpp; tests/tracklet_builders.hpp
```

### `tests/association/test_siou_cost_calculator.cpp`

```
Small_SIoUCostCalculatorTest.IdenticalBoxesCostIsZero | keep | 9 | SIoU: identical boxes cost 0 | tests/association/test_siou_cost_calculator.cpp; tests/tracklet_builders.hpp
Small_SIoUCostCalculatorTest.SeparatedBoxesCostPositive | keep | 9 | SIoU: separated boxes cost more than 0 (weak bound, kept) | tests/association/test_siou_cost_calculator.cpp; tests/tracklet_builders.hpp
Small_SIoUCostCalculatorTest.SingleMatchesBatch | rewrite | 6 | oracle is calculate_single, another path of the unit; rewrite with a hand-computed SIoU value | tests/association/test_siou_cost_calculator.cpp; tests/tracklet_builders.hpp
```

### `tests/c_api/test_c_api_log_category.cpp`

```
Small_CAPILogCategoryTest.NamesTheProjectAndTheModule | delete | 1 | forbidden kind: a log category, a constant with no logic behind it | tests/c_api/test_c_api_log_category.cpp; tests/captured_log.hpp
```

### `tests/c_api/test_sushitrack_c/test_create_validation.cpp`

```
Small_SushiTrackCCreateValidationTest.UnregisteredKalmanTypeIsRefusedAtCreate | split | 3 | two promises: the refusal (INVALID_ARGUMENT, null handle) and the one ERROR naming the type; the log half reaches the process-wide sink and is sized by question 8 after the split | tests/c_api/test_sushitrack_c/test_create_validation.cpp; tests/captured_log.hpp; include/SushiTrack/c_api/sushitrack_c.h
Small_SushiTrackCCreateValidationTest.UnregisteredKalmanTypeIsRefusedWithGatingOn | merge | 4 | merge with UnregisteredKalmanTypeIsRefusedAtCreate (its refusal half): same promise with gating on | tests/c_api/test_sushitrack_c/test_create_validation.cpp; include/SushiTrack/c_api/sushitrack_c.h
Small_SushiTrackCCreateValidationTest.RefusedArgumentsAreLoggedByName | split | 3 | several promises: one ERROR per refusing entry, each naming the entry and the argument; the return-code assertions repeat tests/c_api/test_sushitrack_c/test_error_handling.cpp (question 4) and are dropped in the split; one test per entry | tests/c_api/test_sushitrack_c/test_create_validation.cpp; tests/c_api/test_sushitrack_c/test_error_handling.cpp; tests/captured_log.hpp
Small_SushiTrackCCreateValidationTest.AdjustedValuesWarnAndStillBuild | split | 3 | three adjustments (iou_type, frame_rate, track_buffer), each one WARN and a built tracker; one test per adjustment | tests/c_api/test_sushitrack_c/test_create_validation.cpp; tests/captured_log.hpp; source/c_api/sushitrack_c.cpp
Small_SushiTrackCCreateValidationTest.FeaturelessDetectionsUnderReidWarnOnce | rewrite | 7 | straight-line rule: the five-frame loop in the body builds the input; move it into a named builder | tests/c_api/test_sushitrack_c/test_create_validation.cpp; tests/captured_log.hpp
Small_SushiTrackCCreateValidationTest.RepeatedOverflowWarnsOnce | rewrite | 7 | straight-line rule: the five-frame loop in the body builds the input; move it into a named builder | tests/c_api/test_sushitrack_c/test_create_validation.cpp; tests/captured_log.hpp
```

### `tests/c_api/test_sushitrack_c/test_entry_points.cpp`

```
Small_SushiTrackCParamsTest.DefaultFrameRateIsPositive | delete | 1 | forbidden kind: a default value; every test that builds a tracker from the defaults exercises it | tests/c_api/test_sushitrack_c/test_entry_points.cpp; tests/c_api/test_sushitrack_c/test_error_handling.cpp
Small_SushiTrackCParamsTest.DefaultTrackBufferIsPositive | delete | 1 | forbidden kind: a default value; covered by the builds from defaults in test_error_handling.cpp | tests/c_api/test_sushitrack_c/test_entry_points.cpp; tests/c_api/test_sushitrack_c/test_error_handling.cpp
Small_SushiTrackCParamsTest.DefaultMatchThreshIsFiniteAndPositive | delete | 1 | forbidden kind: a default value; covered by the builds from defaults in test_error_handling.cpp | tests/c_api/test_sushitrack_c/test_entry_points.cpp; tests/c_api/test_sushitrack_c/test_error_handling.cpp
Small_SushiTrackCCreateExTest.NullParamsReturnsInvalidArgument | delete | 4 | sibling tests/c_api/test_sushitrack_c/test_error_handling.cpp::Small_SushiTrackCInvalidArgumentStandaloneTest.CreateExNullParamsSetsTheOutHandleToNull checks the same status and also the out handle | tests/c_api/test_sushitrack_c/test_entry_points.cpp; tests/c_api/test_sushitrack_c/test_error_handling.cpp
Small_SushiTrackCCreateExTest.NullOutHdlReturnsInvalidArgument | keep | 9 | create_ex: a null out handle is refused; the canonical return check for this argument | tests/c_api/test_sushitrack_c/test_entry_points.cpp; include/SushiTrack/c_api/sushitrack_c.h
Small_SushiTrackCCreateExTest.ValidParamsReturnsSuccessAndNonNullHandle | keep | 9 | create_ex: valid params give SUCCESS and a handle | tests/c_api/test_sushitrack_c/test_entry_points.cpp
Small_SushiTrackCUpdateTest.ZeroDetectionsOnEmptyTrackerReturnsZero | keep | 9 | update: zero detections on an empty tracker return 0 | tests/c_api/test_sushitrack_c/test_entry_points.cpp
Small_SushiTrackCUpdateTest.ReturnValueDoesNotExceedMaxTracks | delete | 4 | sibling tests/c_api/test_sushitrack_c/test_error_handling.cpp::Small_SushiTrackCBufferCapacityTest.TracksPastTheCapacityReturnBufferOverflow checks the count equals the capacity (3) at 12 detections | tests/c_api/test_sushitrack_c/test_entry_points.cpp; tests/c_api/test_sushitrack_c/test_error_handling.cpp
Small_SushiTrackCUpdateTest.NewDetectionProducesNonZeroTrackId | keep | 9 | update: a new detection gets a non-zero track id | tests/c_api/test_sushitrack_c/test_entry_points.cpp
Small_SushiTrackCUpdateTest.ThreeConsecutiveHitsPromoteToTracked | keep | 9 | update: three hits promote to Tracked | tests/c_api/test_sushitrack_c/test_entry_points.cpp
Small_SushiTrackCUpdateTest.RemovedTrackAbsentFromNextUpdate | keep | 9 | update: a removed track is absent from the next output; the sequence is the precondition of the one checked promise | tests/c_api/test_sushitrack_c/test_entry_points.cpp
Small_SushiTrackCLogTest.CreateAndDestroyLogTheirLifecycleEvents | resize | 8 | the promise is a log, captured from the process-wide SushiLog sink; small forbids a process-wide instance; the seam is a log sink passed to the tracker; medium until it exists | tests/c_api/test_sushitrack_c/test_entry_points.cpp; tests/captured_log.hpp
Small_SushiTrackCLogTest.DefaultLevelKeepsLifecycleEventsQuiet | resize | 8 | same seam as the test above: a process-wide log sink; medium until the seam exists | tests/c_api/test_sushitrack_c/test_entry_points.cpp; tests/captured_log.hpp
```

### `tests/c_api/test_sushitrack_c/test_error_handling.cpp`

```
Small_SushiTrackCInvalidArgumentStandaloneTest.GetDefaultParamsNullPointerDoesNotCrash | rewrite | 1 | forbidden kind: SUCCEED() as the only statement; rewrite so a checked observable shows the null call changed nothing (for example, a later create from defaults still succeeds) | tests/c_api/test_sushitrack_c/test_error_handling.cpp; include/SushiTrack/c_api/sushitrack_c.h
Small_SushiTrackCInvalidArgumentStandaloneTest.CreateWithNullParamsAutoLoadsConfig | rewrite | 2 | the promise (null params auto-loads sushitrack.json as an overlay) is not observed; the test asserts only a non-null handle, which a builder without the overlay also gives; the doc comment is cut off mid-parenthesis | tests/c_api/test_sushitrack_c/test_error_handling.cpp; source/configuration/config_loader.cpp
Small_SushiTrackCInvalidArgumentStandaloneTest.CreateExNullParamsSetsTheOutHandleToNull | keep | 9 | create_ex: null params give INVALID_ARGUMENT and a null out handle | tests/c_api/test_sushitrack_c/test_error_handling.cpp; include/SushiTrack/c_api/sushitrack_c.h
Small_SushiTrackCInvalidArgumentTest.UpdateWithNullHandleReturnsInvalidArgument | keep | 9 | update: null handle is refused, count 0 | tests/c_api/test_sushitrack_c/test_error_handling.cpp
Small_SushiTrackCInvalidArgumentTest.UpdateWithNullDetectionsAndPositiveCountReturnsInvalidArgument | keep | 9 | update: null detections with a positive count are refused; a separate argument from the other refusals | tests/c_api/test_sushitrack_c/test_error_handling.cpp
Small_SushiTrackCInvalidArgumentTest.UpdateWithNullOutputBufferAndPositiveMaxReturnsInvalidArgument | keep | 9 | update: null output buffer is refused; a separate argument | tests/c_api/test_sushitrack_c/test_error_handling.cpp
Small_SushiTrackCInvalidArgumentTest.UpdateWithNegativeObjectCountReturnsInvalidArgument | keep | 9 | update: negative n_objects is refused; a separate argument | tests/c_api/test_sushitrack_c/test_error_handling.cpp
Small_SushiTrackCInvalidArgumentTest.UpdateWithNegativeMaxTracksReturnsInvalidArgument | keep | 9 | update: negative max_tracks is refused; a separate argument | tests/c_api/test_sushitrack_c/test_error_handling.cpp
Small_SushiTrackCInvalidArgumentTest.UpdateWithNullOutCountReturnsInvalidArgument | keep | 9 | update: null out_count is refused; a separate argument | tests/c_api/test_sushitrack_c/test_error_handling.cpp
Small_SushiTrackCInvalidArgumentTest.GetTrackletsWithNullHandleReturnsInvalidArgument | keep | 9 | get_tracklets: null handle is refused, count 0 | tests/c_api/test_sushitrack_c/test_error_handling.cpp
Small_SushiTrackCInvalidArgumentTest.GetTrackletsWithNullOutCountReturnsInvalidArgument | keep | 9 | get_tracklets: null out_count is refused | tests/c_api/test_sushitrack_c/test_error_handling.cpp
Small_SushiTrackCParameterRangeTest.ZeroFrameRateIsRejectedOrInert | rewrite | 7 | straight-line rule: an if and a return in the body; the disjunctive oracle accepts either outcome; source/c_api/sushitrack_c.cpp:185-189 clamps a frame_rate below 1 to 1 with a WARN and builds the tracker, so pin SUCCESS and the clamp | tests/c_api/test_sushitrack_c/test_error_handling.cpp; source/c_api/sushitrack_c.cpp
Small_SushiTrackCParameterRangeTest.NegativeFrameRateIsRejectedOrInert | rewrite | 7 | straight-line rule and disjunctive oracle; the doc comment says "must be rejected", but source/c_api/sushitrack_c.cpp:185-189 clamps and the body accepts SUCCESS; the doc and the code disagree, and the owner rules which one holds | tests/c_api/test_sushitrack_c/test_error_handling.cpp; source/c_api/sushitrack_c.cpp
Small_SushiTrackCParameterRangeTest.NegativeTrackBufferIsRejectedOrNormalised | rewrite | 7 | straight-line rule and disjunctive oracle; source/c_api/sushitrack_c.cpp:191-195 clamps a track_buffer below 1; pin SUCCESS and the clamp | tests/c_api/test_sushitrack_c/test_error_handling.cpp; source/c_api/sushitrack_c.cpp
Small_SushiTrackCParameterRangeTest.MatchThreshGreaterThanOneIsTolerated | rewrite | 2 | the doc says the match "must not match anything"; the body asserts only that creation succeeds, so the promise is not observed; assert that no track is confirmed from a detection | tests/c_api/test_sushitrack_c/test_error_handling.cpp
Small_SushiTrackCParameterRangeTest.TrackThreshGreaterThanHighThreshDoesNotCrash | rewrite | 2 | the doc says no track may be fabricated from a low-score detection; the body accepts zero or one track, so the promise is not observed; assert the count the documented split gives | tests/c_api/test_sushitrack_c/test_error_handling.cpp
Small_SushiTrackCBufferCapacityTest.ZeroMaxTracksAppliesTheFrameAndReportsOverflow | keep | 9 | buffer: a zero-capacity call applies the frame and reports overflow; the get_tracklets check is the frame's effect | tests/c_api/test_sushitrack_c/test_error_handling.cpp
Small_SushiTrackCBufferCapacityTest.TracksPastTheCapacityReturnBufferOverflow | keep | 9 | buffer: overflow returns BUFFER_OVERFLOW with count equal to capacity | tests/c_api/test_sushitrack_c/test_error_handling.cpp
Small_SushiTrackCBufferCapacityTest.TracksWithinTheCapacityReturnSuccess | keep | 9 | buffer: tracks that fit return SUCCESS | tests/c_api/test_sushitrack_c/test_error_handling.cpp
Small_SushiTrackCLifecycleSafetyTest.DestroyNullHandleIsNoOp | rewrite | 1 | forbidden kind: SUCCEED() as the only statement; rewrite so a live tracker still updates after a null destroy | tests/c_api/test_sushitrack_c/test_error_handling.cpp
Small_SushiTrackCLifecycleSafetyTest.RepeatedCreateDestroyCycleSucceeds | rewrite | 7 | straight-line rule: the 50-cycle loop in the body; leaks are the sanitizers' promise, not this test's; rewrite to one create, destroy and create again | tests/c_api/test_sushitrack_c/test_error_handling.cpp
Small_SushiTrackCStatusCodeCoverageTest.InitFailureReturnsInitializationFailureCode | resize | 8 | the creator is registered in the process-wide KalmanFilterRegistry singleton and never removed; small forbids it; the seam is a registry passed in; medium until it exists | tests/c_api/test_sushitrack_c/test_error_handling.cpp; include/SushiTrack/c_api/sushitrack_c.h
Small_SushiTrackCStatusCodeCoverageTest.RuntimeErrorReturnsRuntimeErrorCode | resize | 8 | the process-wide registry and the global flag g_fail_later_throws; small forbids both; the seam is the same registry passed in | tests/c_api/test_sushitrack_c/test_error_handling.cpp
Small_SushiTrackCStatusCodeCoverageTest.OutOfMemoryReturnsOutOfMemoryCode | resize | 8 | the process-wide registry; small forbids it; the same seam | tests/c_api/test_sushitrack_c/test_error_handling.cpp
Small_SushiTrackCBoundaryExceptionTest.UpdateSurvivesAThrowingSink | resize | 8 | the promise is the C boundary, which a medium test sees; the fixture sets the process-wide SushiLog sink, which small forbids | tests/c_api/test_sushitrack_c/test_error_handling.cpp
Small_SushiTrackCBoundaryExceptionTest.GetTrackletsSurvivesAThrowingSink | resize | 8 | the C boundary, medium; the fixture sets the process-wide sink | tests/c_api/test_sushitrack_c/test_error_handling.cpp
Small_SushiTrackCBoundaryExceptionTest.CreateExSurvivesAThrowingSink | resize | 8 | the C boundary, medium; the fixture sets the process-wide sink | tests/c_api/test_sushitrack_c/test_error_handling.cpp
Small_SushiTrackCBoundaryExceptionTest.DestroySurvivesAThrowingSink | resize | 8 | the C boundary, medium; the fixture sets the process-wide sink | tests/c_api/test_sushitrack_c/test_error_handling.cpp
```

### `tests/c_api/test_sushitrack_export.cpp`

```
Small_SushiTrackExportTest.ApiMacroImportsNothingInAStaticBuild | keep | 9 | export: SUSHITRACK_API expands to no dllimport in a static build; the expansion "[]" is the specified value | tests/c_api/test_sushitrack_export.cpp
```

### `tests/configuration/test_config_loader.cpp`

```
Medium_ConfigLoaderBaselineTest.ConfigFileParsesAndContainsRootObject | keep | 9 | the shipped sushitrack.json parses to an object with a sushitrack root | tests/configuration/test_config_loader.cpp
Medium_ConfigLoaderBaselineTest.BasicSectionContainsAllDocumentedSwitches | keep | 9 | the shipped file holds every basic switch | tests/configuration/test_config_loader.cpp
Medium_ConfigLoaderBasicFieldsTest.BasicNumericFieldsRoundTripIntoParams | rewrite | 2 | the params come from apply_config_to_params, a copy of the loader in the test file; sushitrack_load_params_from_json is not called, so a broken loader passes; rewrite against the library loader | tests/configuration/test_config_loader.cpp; source/configuration/config_loader.cpp
Medium_ConfigLoaderBasicFieldsTest.EnableFlagsRoundTripIntoParams | rewrite | 2 | same: the in-test helper is exercised, not the library loader; rewrite against sushitrack_load_params_from_json | tests/configuration/test_config_loader.cpp; source/configuration/config_loader.cpp
Medium_ConfigLoaderAdvancedFieldsTest.AssociationFieldsAreApplied | rewrite | 2 | SetUp fills params_ with the in-test helper; the production loader is not called; rewrite against the library loader | tests/configuration/test_config_loader.cpp; source/configuration/config_loader.cpp
Medium_ConfigLoaderAdvancedFieldsTest.CoordinateSystemFieldsAreApplied | rewrite | 2 | same fixture, same cause | tests/configuration/test_config_loader.cpp; source/configuration/config_loader.cpp
Medium_ConfigLoaderAdvancedFieldsTest.ReIDFieldsIncludingDirectAssignmentWeightAreApplied | rewrite | 2 | same fixture, same cause; the comment cites an omission that this test does not close | tests/configuration/test_config_loader.cpp; source/configuration/config_loader.cpp
Medium_ConfigLoaderAdvancedFieldsTest.TentativeLogicTiersAreApplied | rewrite | 2 | same fixture, same cause | tests/configuration/test_config_loader.cpp; source/configuration/config_loader.cpp
Medium_ConfigLoaderAdvancedFieldsTest.MahalanobisGatingFieldsAreApplied | rewrite | 2 | same fixture, same cause | tests/configuration/test_config_loader.cpp; source/configuration/config_loader.cpp
Medium_ConfigLoaderDefaultsTest.MissingBasicSectionLeavesDefaults | rewrite | 2 | the in-test helper is exercised, not the library loader; rewrite against sushitrack_load_params_from_json with a temporary file | tests/configuration/test_config_loader.cpp; source/configuration/config_loader.cpp
Medium_ConfigLoaderDefaultsTest.EmptyDocumentLeavesAllDefaults | rewrite | 2 | the in-test helper is exercised; compares four fields only; rewrite against the library loader | tests/configuration/test_config_loader.cpp; source/configuration/config_loader.cpp
Medium_ConfigLoaderTrackerSmokeTest.LoadedParamsBuildTrackerSuccessfully | rewrite | 2 | the params come from the in-test helper; the library loader is not on the path; rewrite to load through sushitrack_load_params_from_json | tests/configuration/test_config_loader.cpp; source/configuration/config_loader.cpp
Medium_ConfigLoaderTrackerSmokeTest.LoadedParamsSurviveSingleDetectionUpdate | rewrite | 2 | same helper; EXPECT_GE(n, 0) accepts any count and the status is not checked; rewrite through the library loader with the status asserted and the count pinned | tests/configuration/test_config_loader.cpp; source/configuration/config_loader.cpp
Medium_ConfigLoaderLibraryApiTest.LibraryLoaderMatchesReferenceParser | keep | 9 | library loader against an independent reference parser on five fields; the parity covers five of the fields | tests/configuration/test_config_loader.cpp; source/configuration/config_loader.cpp
Medium_ConfigLoaderLibraryApiTest.NullParamsAreRejected | keep | 9 | a null params pointer is refused | tests/configuration/test_config_loader.cpp
Medium_ConfigLoaderLibraryApiTest.MissingNamedFileIsRefusedAndLogged | keep | 9 | a named missing file is refused, params untouched, one ERROR names the file; the file boundary is medium | tests/configuration/test_config_loader.cpp; tests/captured_log.hpp; source/configuration/config_loader.cpp
Medium_ConfigLoaderLibraryApiTest.MissingFileNamedByTheEnvironmentIsRefusedAndLogged | rewrite | 8 | the ScopedEnvironment helper leaves SUSHITRACK_CONFIG defined: on Windows _putenv_s with an empty value sets the variable to empty, and the destructor cannot unset it | tests/configuration/test_config_loader.cpp; source/configuration/config_loader.cpp
Medium_ConfigLoaderLibraryApiTest.NothingNamedAndNothingFoundSucceedsAtInfo | rewrite | 8 | depends on the machine: resolve_config_path walks up to K_MAX_SEARCH_DEPTH parents of the temporary folder (source/configuration/config_loader.cpp:105-125), so a sushitrack.json above it fails the test; it also changes the process directory and leaves the empty folder behind | tests/configuration/test_config_loader.cpp; source/configuration/config_loader.cpp
Medium_ConfigLoaderStrictReaderTest.WrongTypedValueIsRefusedWithItsPath | rewrite | 8 | writes into the shared folder temp/sushitrack_config_loader_tests and never removes the file; the medium rule requires a per-test folder cleaned up | tests/configuration/test_config_loader.cpp; tests/captured_log.hpp
Medium_ConfigLoaderStrictReaderTest.FractionInAnIntegerFieldIsRefused | rewrite | 8 | same shared folder left behind | tests/configuration/test_config_loader.cpp; tests/captured_log.hpp
Medium_ConfigLoaderStrictReaderTest.UnknownKeyIsRefusedWhereverItSits | rewrite | 8 | three positions of one promise, so rows; same shared folder left behind; the two JSON shapes (with and without the sushitrack root) should be stated as the documented shapes | tests/configuration/test_config_loader.cpp; tests/captured_log.hpp
Medium_ConfigLoaderStrictReaderTest.FlagsAndWholeNumbersApply | rewrite | 8 | same shared folder left behind | tests/configuration/test_config_loader.cpp
Medium_ConfigLoaderStrictReaderTest.MalformedJsonReturnsRuntimeError | rewrite | 8 | same shared folder left behind | tests/configuration/test_config_loader.cpp; tests/captured_log.hpp
Medium_ConfigLoaderCreateTest.RefusedFileGivesNoTracker | rewrite | 8 | same shared folder left behind; the second half (a missing file gives no tracker) is a second promise, so split it out | tests/configuration/test_config_loader.cpp; tests/captured_log.hpp
```

### `tests/configuration/test_configuration_log_category.cpp`

```
Small_ConfigurationLogCategoryTest.NamesTheProjectAndTheModule | delete | 1 | forbidden kind: a log category, a constant with no logic behind it | tests/configuration/test_configuration_log_category.cpp; tests/captured_log.hpp
```

### `tests/detection/test_object.cpp`

```
Small_ObjectTest.KeepsWhatItIsGiven | keep | 9 | object: the constructor keeps the four values | tests/detection/test_object.cpp, include/SushiTrack/detection/object.hpp
Small_ObjectTest.FeatureDefaultsToEmpty | delete | 1 | forbidden kind: default value; it states no behaviour, and the tracker passes obj.feature on without a branch on it (source/tracking/tracker.cpp:140) | tests/detection/test_object.cpp, include/SushiTrack/detection/object.hpp, source/tracking/tracker.cpp
Small_ObjectTest.ObjectVectorKeepsSixteenByteAlignment | rewrite | 1 | forbidden kind: a for loop builds the input in the body; the alignment promise is real and only this test holds it; rewrite with a builder of five objects | tests/detection/test_object.cpp, include/SushiTrack/detection/object.hpp
```

### `tests/detection/test_rect.cpp`

```
Small_RectTest.ConstructorStoresComponents | delete | 1 | forbidden kind: trivial code, accessor readback after construction; TLWH order is checked by GetTlbrReturnsBoundingCorners and GenerateByTlbrProducesCorrectDimensions | tests/detection/test_rect.cpp, include/SushiTrack/detection/rect.hpp
Small_RectTest.GetXywhReturnsCentroid | keep | 9 | rect (get_xywh) | tests/detection/test_rect.cpp, include/SushiTrack/detection/rect.hpp
Small_RectTest.GetTlbrReturnsBoundingCorners | keep | 9 | rect (get_tlbr) | tests/detection/test_rect.cpp, include/SushiTrack/detection/rect.hpp
Small_RectTest.GenerateByTlbrRoundTrip | keep | 9 | rect (generate_rect_by_tlbr) round trip | tests/detection/test_rect.cpp, include/SushiTrack/detection/rect.hpp
Small_RectTest.GenerateByTlbrProducesCorrectDimensions | keep | 9 | rect (generate_rect_by_tlbr) width and height; row owner that the merges of SquareBoxFromTlbrHasEqualDimensions and GenerateByTlbrReversesCorners fold into | tests/detection/test_rect.cpp, include/SushiTrack/detection/rect.hpp
Small_RectTest.GenerateByXywhRoundTrip | rewrite | 1 | forbidden kind: bare tolerance 1e-4f in EXPECT_NEAR; the values are exact in float, so EXPECT_FLOAT_EQ suffices; the only round-trip test of xywh | tests/detection/test_rect.cpp, include/SushiTrack/detection/rect.hpp
Small_RectTest.GenerateByXywhProducesCorrectTopLeft | rewrite | 1 | forbidden kind: bare tolerance 1e-4f; the only test of the top-left formula, since a consistent error in both directions still passes the round trip | tests/detection/test_rect.cpp, include/SushiTrack/detection/rect.hpp
Small_RectTest.MutableAccessorsModifyComponents | delete | 1 | forbidden kind: trivial code, setter and getter pair with no derived promise | tests/detection/test_rect.cpp, include/SushiTrack/detection/rect.hpp
Small_RectTest.IntSpecialisationCompiles | delete | 1 | forbidden kind: trivial code; the name states compiling; the library explicitly instantiates Rect<int>, so the compile is checked at build time | tests/detection/test_rect.cpp, include/SushiTrack/detection/rect.hpp, source/detection/rect.cpp
Small_RectTest.ZeroSizeRectHasCoincidentCorners | keep | 9 | rect: zero-size box has coincident corners | tests/detection/test_rect.cpp, include/SushiTrack/detection/rect.hpp
Small_RectTest.NegativeOriginCentroidIsCorrect | keep | 9 | rect: negative origin, centroid correct | tests/detection/test_rect.cpp, include/SushiTrack/detection/rect.hpp
Small_RectTest.SquareBoxFromTlbrHasEqualDimensions | merge | 4 | merge with Small_RectTest.GenerateByTlbrProducesCorrectDimensions as a square row; it asserts only that width equals height, not their values; the same formula is checked there | tests/detection/test_rect.cpp, include/SushiTrack/detection/rect.hpp
Small_RectTest.LargeBoxRoundTripIsExact | delete | 1 | forbidden kind: bare tolerance 1e-3f although named exact; the same promise at a larger size is GenerateByXywhRoundTrip, so question 4 also holds | tests/detection/test_rect.cpp
Small_RectTest.HandlesZeroSize | delete | 4 | same promise and same inputs as ZeroSizeRectHasCoincidentCorners in the same file, same assertions | tests/detection/test_rect.cpp
Small_RectTest.AllowsNegativeCoordinates | merge | 4 | merge with Small_RectTest.NegativeOriginCentroidIsCorrect; the same negative origin, checked on the stored values | tests/detection/test_rect.cpp, include/SushiTrack/detection/rect.hpp
Small_RectTest.GenerateByTlbrReversesCorners | merge | 4 | merge with Small_RectTest.GenerateByTlbrProducesCorrectDimensions as a reversed-corner row; the header states no reversed-corner contract, so the owner confirms the row before it stays | tests/detection/test_rect.cpp, include/SushiTrack/detection/rect.hpp
Small_RectTest.XywhAspectRatioCalculation | delete | 4 | the only assertion is the test's own division; the width and height it reads are checked by Small_RectTest.GetXywhReturnsCentroid | tests/detection/test_rect.cpp
```

### `tests/filtering/test_filtering_log_category.cpp`

```
Small_FilteringLogCategoryTest.NamesTheProjectAndTheModule | delete | 1 | forbidden kind: a log category constant | tests/filtering/test_filtering_log_category.cpp
```

### `tests/filtering/test_gate_validator.cpp`

```
Small_GateValidatorTest.DefaultSquaredDistancesAreZeroPerMeasurement | rewrite | 1 | forbidden kind: a stand-in (AcceptingValidator) the test writes for a repository-owned interface; the rewrite needs an in-memory GateValidator adapter in the filtering module, which does not exist; NullGateValidator cannot stand in because it overrides the default | tests/filtering/test_gate_validator.cpp, include/SushiTrack/filtering/gate_validator.hpp, include/SushiTrack/filtering/null_gate_validator.hpp
Small_GateValidatorTest.KalmanGateKeepsTheNearAndRejectsTheFar | delete | 4 | same promise in tests/filtering/test_gated_state_predictor.cpp::Small_GatedStatePredictorTest.GatesItsStartInAndAFarBoxOut and tests/filtering/test_kalman_filter.cpp::Small_KalmanFilterTest.IsInGate_FarObservationFails | tests/filtering/test_gate_validator.cpp, tests/filtering/test_gated_state_predictor.cpp, tests/filtering/test_kalman_filter.cpp
```

### `tests/filtering/test_gated_state_predictor.cpp`

```
Small_GatedStatePredictorTest.GatesItsStartInAndAFarBoxOut | resize | 8 | reaches KalmanFilterRegistry::get_instance, a process-wide singleton, through the parameter; seam: a registry the test builds, which needs a public or injected constructor (private today, owner decision); small once built | tests/filtering/test_gated_state_predictor.cpp, include/SushiTrack/filtering/kalman_filter_registry.hpp
Small_GatedStatePredictorTest.CovarianceStaysSymmetric | rewrite | 1 | forbidden kind: bare tolerance 1e-4f in isApprox; the NSA and IMM cases have no other symmetry test; also reaches the singleton, same seam as GatesItsStartInAndAFarBoxOut | tests/filtering/test_gated_state_predictor.cpp, include/SushiTrack/filtering/kalman_filter_registry.hpp
```

### `tests/filtering/test_generic_kalman_factory.cpp`

```
Small_GenericKalmanFactoryTest.BuildsTheRegisteredTypes | resize | 8 | GenericKalmanFactory::create reads KalmanFilterRegistry::get_instance (source/filtering/generic_kalman_factory.cpp:52); seam: the factory takes a registry; small once built | tests/filtering/test_generic_kalman_factory.cpp, source/filtering/generic_kalman_factory.cpp, include/SushiTrack/filtering/kalman_filter_registry.hpp
Small_GenericKalmanFactoryTest.UnregisteredTypeBuildsNothing | rewrite | 7 | sign: CapturedLog is declared and never asserted; it only sets every category to Off, silencing the ERROR the registry logs; drop it and assert through the seam | tests/filtering/test_generic_kalman_factory.cpp, tests/captured_log.hpp, source/filtering/generic_kalman_factory.cpp
```

### `tests/filtering/test_imm_filter.cpp`

```
Small_IMMFilterTest.StartsOnTheConstantVelocityPrior | rewrite | 1 | forbidden kind: bare tolerance 1e-5f in two EXPECT_NEAR; the only test of the IMM prior values; use EXPECT_FLOAT_EQ against 0.2f and 0.1f | tests/filtering/test_imm_filter.cpp
Small_IMMFilterTest.ModeProbabilitiesStayWithinZeroAndOne | rewrite | 1 | forbidden kind: a for loop over ten frames in the body and bare tolerance 1e-5f; the isfinite check reads one component only | tests/filtering/test_imm_filter.cpp
Small_IMMFilterTest.RestoreCheckpointBringsBackTheModeProbabilities | rewrite | 1 | forbidden kind: a for loop over five frames in the body and bare tolerance 1e-6f | tests/filtering/test_imm_filter.cpp
```

### `tests/filtering/test_kalman_factory.cpp`

```
Small_KalmanFactoryTest.EveryCreateGivesANewPredictor | resize | 8 | creates through GenericKalmanFactory(0), which reads the registry singleton (source/filtering/generic_kalman_factory.cpp:52); seam as GenericKalmanFactory above; the KalmanFactory contract has no in-memory adapter | tests/filtering/test_kalman_factory.cpp, include/SushiTrack/filtering/kalman_factory.hpp, source/filtering/generic_kalman_factory.cpp
```

### `tests/filtering/test_kalman_filter.cpp`

```
Small_KalmanFilterTest.InitiateMeanMatchesMeasurement | rewrite | 1 | forbidden kind: a for loop in the body checks the velocity block; unroll into four checks or a helper | tests/filtering/test_kalman_filter.cpp, tests/measurement_builders.hpp
Small_KalmanFilterTest.InitiateCovarianceIsDiagonalAndPositive | rewrite | 1 | forbidden kind: nested loops and an if in the body; the zero off-diagonals assert the initial shape, an internal choice (question 5), so only the positive diagonal stays | tests/filtering/test_kalman_filter.cpp, tests/measurement_builders.hpp
Small_KalmanFilterTest.PredictPositionShiftsByVelocityTimesDt | delete | 1 | forbidden kind: bare tolerance 1e-4f; the same promise with exact values in PredictWithDtGreaterThanOne in the same file | tests/filtering/test_kalman_filter.cpp
Small_KalmanFilterTest.PredictIncreasesCovariance | rewrite | 1 | forbidden kind: a for loop in the body | tests/filtering/test_kalman_filter.cpp
Small_KalmanFilterTest.UpdateMeanMovesTowardMeasurement | rewrite | 1 | forbidden kind: bare tolerances 5.f and 10.f in EXPECT_NEAR; the name promises movement, but the body checks nearness to the pre-update position | tests/filtering/test_kalman_filter.cpp
Small_KalmanFilterTest.UpdateDecreasesCovariance | rewrite | 1 | forbidden kind: a for loop in the body | tests/filtering/test_kalman_filter.cpp
Small_KalmanFilterTest.IsInGate_ObservationAtMeanPasses | rewrite | 7 | sign: setup longer than the check; the obs block is dead code, and its comments contradict the code; the check uses box | tests/filtering/test_kalman_filter.cpp
Small_KalmanFilterTest.IsInGate_FarObservationFails | delete | 4 | same promise in tests/filtering/test_gated_state_predictor.cpp::Small_GatedStatePredictorTest.GatesItsStartInAndAFarBoxOut; the comment says 1000 px but the box is 5000 px | tests/filtering/test_kalman_filter.cpp, tests/filtering/test_gated_state_predictor.cpp
Small_KalmanFilterTest.GetGateFlagsMatchesScalarIsInGate | rewrite | 1 | forbidden kind: a for loop in the body | tests/filtering/test_kalman_filter.cpp
Small_KalmanFilterTest.ElevatedMultipliersIncreaseProcessNoise | rewrite | 1 | forbidden kind: a for loop in the body | tests/filtering/test_kalman_filter.cpp
Small_KalmanFilterTest.XYAHModeInitiateMeanMatchesMeasurement | rewrite | 1 | forbidden kind: a for loop in the body over the velocity block | tests/filtering/test_kalman_filter.cpp
Small_KalmanFilterTest.PredictWithDtGreaterThanOne | keep | 9 | the surviving exact test of predict with dt above one; the name states an input rather than a defect, and the owner may rename it | tests/filtering/test_kalman_filter.cpp
Small_KalmanFilterTest.XyahPredictUpdateStability | rewrite | 1 | forbidden kind: bare tolerance 1e-4 (a double literal) in EXPECT_NEAR; checks one component after one cycle | tests/filtering/test_kalman_filter.cpp
Small_KalmanFilterTest.CovarianceRemainsSymmetricAfterUpdate | delete | 4 | forbidden kind: bare tolerance 1e-5f; the same promise (predict then update, symmetry) for type 0 in tests/filtering/test_gated_state_predictor.cpp::Small_GatedStatePredictorTest.CovarianceStaysSymmetric | tests/filtering/test_kalman_filter.cpp, tests/filtering/test_gated_state_predictor.cpp
```

### `tests/filtering/test_kalman_filter_registry.cpp`

```
Small_KalmanFilterRegistryTest.UnregisteredTypeLogsTheTypeAndReturnsNull | split | 3 | two promises joined by "and": returns nullptr, and logs one ERROR naming type 987654; split into two tests, each of which reaches the singleton (question 8 seam applies to each) | tests/filtering/test_kalman_filter_registry.cpp, include/SushiTrack/filtering/kalman_filter_registry.hpp, tests/captured_log.hpp
Small_KalmanFilterRegistryTest.RegisteringATypeTwiceWarnsAndTheLaterCreatorWins | split | 3 | two promises: a WARN on the second registration (checked), and the later creator wins (not checked, see the add row); a check sits between the two calls; the registration of 987001 stays in the singleton after the test | tests/filtering/test_kalman_filter_registry.cpp, include/SushiTrack/filtering/kalman_filter_registry.hpp
```

### `tests/filtering/test_kalman_types.cpp`

```
Small_KalmanTypesTest.MeasurementIsRowMajor | delete | 1 | forbidden kind: an assertion on a compile-time constant (StateCov::IsRowMajor); the [x, y, w, h] order is checked by tests/filtering/test_kalman_filter.cpp::Small_KalmanFilterTest.InitiateMeanMatchesMeasurement | tests/filtering/test_kalman_types.cpp, include/SushiTrack/filtering/kalman_types.hpp
Small_KalmanTypesTest.DetectBoxVectorKeepsAlignedMeasurements | rewrite | 1 | forbidden kind: a push loop and a check loop in the body; the alignment promise is real (aligned_allocator, kalman_types.hpp:55); move the build to a builder | tests/filtering/test_kalman_types.cpp, include/SushiTrack/filtering/kalman_types.hpp
```

### `tests/filtering/test_nsa_kalman_filter.cpp`

```
Small_NSAKalmanFilterTest.InitiateProducesValidCovariance | rewrite | 1 | forbidden kind: a for loop in the body; the only NSA initiate covariance check | tests/filtering/test_nsa_kalman_filter.cpp
Small_NSAKalmanFilterTest.PredictUpdateCycleIsNumericallyStable | rewrite | 1 | forbidden kind: a twenty-cycle loop and an eight-entry check loop in the body; the implicit oracle stays, the cycles move into a builder with a bounded count | tests/filtering/test_nsa_kalman_filter.cpp
Small_NSAKalmanFilterTest.InitiateCovarianceScalesWithScore | rewrite | 1 | forbidden kind: two for loops in the body | tests/filtering/test_nsa_kalman_filter.cpp
Small_NSAKalmanFilterTest.UpdateWeightsMeasurementByScore | keep | 9 | nsa: a higher score pulls the mean nearer the measurement; metamorphic oracle | tests/filtering/test_nsa_kalman_filter.cpp
Small_NSAKalmanFilterTest.MahalanobisDistanceScalesWithScore | keep | 9 | nsa: a lower score gives a smaller distance; metamorphic oracle | tests/filtering/test_nsa_kalman_filter.cpp
Small_NSAKalmanFilterTest.BatchGatingWithScores | split | 3 | one test with two calls, and a threshold taken from the first call's output; split into distances with scores and flags with scores | tests/filtering/test_nsa_kalman_filter.cpp
Small_NSAKalmanFilterTest.NegativeScoreHandledGracefully | rewrite | 1 | forbidden kind: a for loop in the body; the name states a quality rather than the promise | tests/filtering/test_nsa_kalman_filter.cpp
Small_NSAKalmanFilterXYAHTest.XYAHModeSupport | rewrite | 1 | forbidden kind: a for loop in the body; the name states a feature (sign) | tests/filtering/test_nsa_kalman_filter.cpp
Small_NSAKalmanFilterTest.ScoreZeroDoesNotProduceNegativeCovariance | rewrite | 1 | forbidden kind: a for loop in the body; the TEST_F members are shadowed by locals and the fixture's kf_ is unused | tests/filtering/test_nsa_kalman_filter.cpp
```

### `tests/filtering/test_null_gate_validator.cpp`

```
Small_NullGateValidatorTest.AcceptsEveryMeasurement | split | 3 | three methods checked in one test: is_in_gate, get_gate_flags and get_gate_sq_distances | tests/filtering/test_null_gate_validator.cpp, include/SushiTrack/filtering/null_gate_validator.hpp
Small_NullGateValidatorTest.InstanceIsShared | delete | 1 | forbidden kind: trivial getter; identity of a function-local static is a language property; no source caller compares identity (source/c_api/sushitrack_c.cpp:393 assigns the value) | tests/filtering/test_null_gate_validator.cpp, include/SushiTrack/filtering/null_gate_validator.hpp, source/c_api/sushitrack_c.cpp
```

### `tests/filtering/test_state_predictor.cpp`

```
Small_StatePredictorTest.SingleModelReportsNoModeProbabilities | keep | 9 | the -1 sentinel is documented in include/SushiTrack/filtering/state_predictor.hpp and read by source/c_api/sushitrack_c.cpp:137 and source/tracking/tracklet.cpp:358; a consumer contract, so it is kept despite looking like a default value | tests/filtering/test_state_predictor.cpp, include/SushiTrack/filtering/state_predictor.hpp, source/tracking/tracklet.cpp
Small_StatePredictorTest.DefaultCheckpointLeavesTheStateAlone | keep | 9 | default checkpoint pair leaves a single-model mean unchanged; tracklet calls save and restore on any predictor (source/tracking/tracklet.cpp:545, 563); note: covariance is not checked | tests/filtering/test_state_predictor.cpp, include/SushiTrack/filtering/state_predictor.hpp, source/tracking/tracklet.cpp
Small_StatePredictorTest.MultiModelReportsProbabilities | delete | 4 | same promise checked with exact values in tests/filtering/test_imm_filter.cpp::Small_IMMFilterTest.StartsOnTheConstantVelocityPrior | tests/filtering/test_state_predictor.cpp, tests/filtering/test_imm_filter.cpp
```

### `tests/logging/test_log_throttle.cpp`

```
Small_LogThrottleTest.AdmitsTheFirstEventAndOneInEvery | rewrite | 1 | forbidden kind: a for loop and an if in the body; also reaches reset_log_throttles and the global generation (question 8 seam: the generation is passed in) | tests/logging/test_log_throttle.cpp, include/SushiTrack/logging/log_throttle.hpp
Small_LogThrottleTest.ZeroEveryAdmitsEveryEvent | resize | 8 | admit reads log_throttle_generation() and the test calls reset_log_throttles(), both process-wide; seam: the generation passed in; small once built | tests/logging/test_log_throttle.cpp, include/SushiTrack/logging/log_throttle.hpp
Small_LogThrottleTest.ResetMakesTheNextEventTheFirst | resize | 8 | reset_log_throttles is process-wide; same seam as ZeroEveryAdmitsEveryEvent | tests/logging/test_log_throttle.cpp, include/SushiTrack/logging/log_throttle.hpp
Small_LogThrottleTest.MacroLogsOnceAThousandTimesAndNamesTheSuppressedCount | rewrite | 1 | forbidden kind: a 2001-event loop in the body; a function-local static throttle keeps state across runs; the sink is the process-wide CapturedLog | tests/logging/test_log_throttle.cpp, include/SushiTrack/logging/logger.hpp, tests/captured_log.hpp
```

### `tests/logging/test_logger.cpp`

```
Small_LoggerTest.WarnCarriesTheProjectModuleAndFormattedMessage | resize | 8 | CapturedLog installs the SushiLog sink and sets levels process-wide; seam: a sink passed to the logging front (SushiLog's design, owner decision); medium until built | tests/logging/test_logger.cpp, include/SushiTrack/logging/logger.hpp, tests/captured_log.hpp
Small_LoggerTest.RecordBelowTheCategoryLevelIsDropped | resize | 8 | same process-wide sink as WarnCarries...; the check is SushiLog's level filter as much as SushiTrack's logic; medium until the seam exists | tests/logging/test_logger.cpp, tests/captured_log.hpp
Small_LoggerTest.ThrowCarriesTheFormattedMessage | rewrite | 1 | forbidden kind: a try and catch in the body; move the assertion into an expect_* helper | tests/logging/test_logger.cpp, include/SushiTrack/logging/logger.hpp
Small_LoggerTest.ThrowLogsNothing | resize | 8 | CapturedLog reaches the process-wide sink; medium until the seam exists | tests/logging/test_logger.cpp, tests/captured_log.hpp, include/SushiTrack/logging/logger.hpp
Medium_LoggerConcurrencyTest.CallsFromTwoThreadsReachTheSink | rewrite | 1 | forbidden kind: for loops inside the thread lambdas, which are part of the body; the name says every record intact, but only the counts are checked | tests/logging/test_logger.cpp, tests/captured_log.hpp
```

### `tests/logging/test_sushitrack_log_project.cpp`

```
Small_SushiTrackLogProjectTest.NamesTheProjectAndItsVariable | delete | 1 | forbidden kind: constant check; the project name is also checked by Small_LoggerTest.WarnCarriesTheProjectModuleAndFormattedMessage; the variable name SUSHITRACK_LOG_LEVEL is checked nowhere else, owner to confirm | tests/logging/test_sushitrack_log_project.cpp, tests/logging/test_logger.cpp, include/SushiTrack/logging/sushitrack_log_project.hpp
```

### `tests/profiling/test_profiler_keys.cpp`

```
Small_ProfilerKeysTest.NamesAreSetAndUnique | rewrite | 1 | forbidden kind: constant check (non-empty); uniqueness is the real promise, but the list omits the group and unit names (K_GROUP_*, K_UNIT_*), so recorded names go untested | tests/profiling/test_profiler_keys.cpp, include/SushiTrack/profiling/profiler_keys.hpp
```

### `tests/profiling/test_sushitrack_profiler_parts/test_compiled_in.cpp`

```
Medium_ProfilingCompiledInTest.ZoneAppearsInTheRecordUnderItsGroup | rewrite | 1 | forbidden kind: a for loop and an if in the body (the site lookup), and an include of the private header source/private_include/SushiTrack/profiling/profiling.hpp; the lookup moves to a helper beside zone_names in tests/profile_session.hpp | tests/profiling/test_sushitrack_profiler_parts/test_compiled_in.cpp, tests/profile_session.hpp, source/private_include/SushiTrack/profiling/profiling.hpp
Medium_ProfilingCompiledInTest.CounterHoldsTheValueAndAddSums | split | 3 | the name joins two promises: a Set counter holds the value (unchecked) and an Add counter sums (checked, 6.0); split, keep the Add test, and add the Set row | tests/profiling/test_sushitrack_profiler_parts/test_compiled_in.cpp, tests/profile_session.hpp
Medium_ProfilingCompiledInTest.RunsWithoutASession | rewrite | 1 | forbidden kind: the only check is EXPECT_NO_THROW, which asserts nothing about recording; the name promises that nothing is recorded, which is unchecked; the file also includes the private profiling.hpp | tests/profiling/test_sushitrack_profiler_parts/test_compiled_in.cpp, tests/profile_session.hpp, source/private_include/SushiTrack/profiling/profiling.hpp
```

### `tests/profiling/test_sushitrack_profiler_parts/test_compiled_out.cpp`

```
Small_ProfilingCompiledOutTest.MacrosExpandToNothingAndEvaluateNoArgument | rewrite | 1 | forbidden kind: private header; profiling.hpp is under source/private_include and its own comment says no installed header includes it; the promise is sound and the only test of it; the rewrite needs a public profiler macro header, which the repository does not have (owner decision) | tests/profiling/test_sushitrack_profiler_parts/test_compiled_out.cpp, source/private_include/SushiTrack/profiling/profiling.hpp
```

### `tests/profiling/test_sushitrack_profiler_parts/test_environment.cpp`

```
Medium_SushiTrackProfilerPartsTest.RecordHoldsTheLibraryFacts | rewrite | 1 | forbidden kind: constant check on the library name; version and commit checked for presence only; the date format and profile level are real checks; the file includes the private profiling.hpp | tests/profiling/test_sushitrack_profiler_parts/test_environment.cpp, tests/profile_session.hpp, source/private_include/SushiTrack/profiling/profiling.hpp
Medium_SushiTrackProfilerPartsTest.TwoRecordsFromOneProcessCompare | rewrite | 1 | forbidden kind: private header (profiling.hpp), the only defect in an otherwise sound test; the promise (a second session after the first stopped, records compare) is real and not tested elsewhere; the rewrite needs the public header (owner decision) | tests/profiling/test_sushitrack_profiler_parts/test_environment.cpp, source/private_include/SushiTrack/profiling/profiling.hpp
```

### `tests/tracking/test_frame_timing_policy.cpp`

```
Small_FrameTimingPolicyTest.ToKfDtAlwaysReturnsOne | rewrite | 7 | four hand-written calls over one promise are rows, not copies; the promise (the frame policy ignores raw_dt) is kept | tests/tracking/test_frame_timing_policy.cpp; include/SushiTrack/tracking/frame_timing_policy.hpp
Small_FrameTimingPolicyTest.IsExpiredFalseAtExactThreshold | merge | 4 | merge with IsExpiredFalseBelow, IsExpiredTrueAbove and ZeroFrameCountAndZeroBufferNotExpired into one row table of is_expired (strictly above the limit) | tests/tracking/test_frame_timing_policy.cpp; include/SushiTrack/tracking/frame_timing_policy.hpp
Small_FrameTimingPolicyTest.IsExpiredFalseBelow | merge | 4 | merge with IsExpiredFalseAtExactThreshold, the same row table | tests/tracking/test_frame_timing_policy.cpp; include/SushiTrack/tracking/frame_timing_policy.hpp
Small_FrameTimingPolicyTest.IsExpiredTrueAbove | merge | 4 | merge with IsExpiredFalseAtExactThreshold, the same row table | tests/tracking/test_frame_timing_policy.cpp; include/SushiTrack/tracking/frame_timing_policy.hpp
Small_FrameTimingPolicyTest.ZeroFrameCountAndZeroBufferNotExpired | merge | 4 | merge with IsExpiredFalseAtExactThreshold; the zero limit is one more row of the strict boundary | tests/tracking/test_frame_timing_policy.cpp; include/SushiTrack/tracking/frame_timing_policy.hpp
```

### `tests/tracking/test_kinematic_state.cpp`

```
Small_KinematicStateTest.KeepsTheNumbersTheCApiDocuments | keep | 9 | kinematic_state: the regime codes keep the numbers the C API reports; an ABI promise, not a trivial constant. The C API header names the field at line 90 and lists no numbers, so the docstring claim is unverified | tests/tracking/test_kinematic_state.cpp; include/SushiTrack/tracking/kinematic_state.hpp; include/SushiTrack/c_api/sushitrack_c.h
```

### `tests/tracking/test_timing_policy.cpp`

```
Small_TimingPolicyTest.OneFrameIsOneStep | delete | 1 | forbidden: bare tolerance 1e-4f in EXPECT_NEAR; the one-step promise is tested in tests/tracking/test_frame_timing_policy.cpp (ToKfDtAlwaysReturnsOne) and tests/tracking/test_wall_clock_timing_policy.cpp (OneFpsIntervalConvertsToOne) | tests/tracking/test_timing_policy.cpp; tests/tracking/test_frame_timing_policy.cpp; tests/tracking/test_wall_clock_timing_policy.cpp; include/SushiTrack/tracking/timing_policy.hpp
Small_TimingPolicyTest.ExpiresOnlyPastTheLimit | delete | 4 | the same boundary is tested in tests/tracking/test_frame_timing_policy.cpp (IsExpiredFalseAtExactThreshold, IsExpiredTrueAbove) and tests/tracking/test_wall_clock_timing_policy.cpp (IsExpiredFalseWhenUnderThreshold, IsExpiredTrueWhenOverThreshold) | tests/tracking/test_timing_policy.cpp; tests/tracking/test_frame_timing_policy.cpp; tests/tracking/test_wall_clock_timing_policy.cpp; include/SushiTrack/tracking/timing_policy.hpp
```

### `tests/tracking/test_track_state.cpp`

```
Small_TrackStateTest.KeepsTheNumbersTheCApiCarries | keep | 9 | track_state: the lifecycle codes keep the numbers the C API carries | tests/tracking/test_track_state.cpp; include/SushiTrack/tracking/track_state.hpp
```

### `tests/tracking/test_tracker/test_advanced_path.cpp`

```
Small_TrackerAdvancedPathTest.ReIDAndMahalanobisEndToEnd | rewrite | 1 | forbidden: a loop over frames and an if on the frame index in the body; the promise (ReID with the gate keeps the target id against a decoy) has no other test; the file's copies of make_det_with_feat and find_by_id duplicate tests/c_detection_builders.hpp | tests/tracking/test_tracker/test_advanced_path.cpp; tests/c_detection_builders.hpp; include/SushiTrack/c_api/sushitrack_c.h
```

### `tests/tracking/test_tracker/test_association_stages.cpp`

```
Small_TrackerHighConfidenceTest.LAPJVModeAssociatesClosedDetectionToTrackedTrack | merge | 4 | merge with AgeCascadeModeAssociatesCloseDetectionToTrackedTrack (the same promise over primary_assoc_mode) | tests/tracking/test_tracker/test_association_stages.cpp; tests/c_detection_builders.hpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerHighConfidenceAgeCascadeTest.AgeCascadeModeAssociatesCloseDetectionToTrackedTrack | merge | 4 | merge with LAPJVModeAssociatesClosedDetectionToTrackedTrack (mode rows) | tests/tracking/test_tracker/test_association_stages.cpp; tests/c_detection_builders.hpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerLowConfidenceTest.UnmatchedTrackedTrackTransitionsToLost | delete | 4 | the same promise is tested in tests/tracking/test_tracker/test_buffers.cpp (TrackedTransitionsToLostOnFirstMiss), which also checks lost time; the docstring's low-confidence detection never appears in the body | tests/tracking/test_tracker/test_association_stages.cpp; tests/tracking/test_tracker/test_buffers.cpp
Small_TrackerLostRecoveryTest.LostTrackRecoveredByReassociation | delete | 4 | the same promise is tested in tests/tracking/test_tracker/test_buffers.cpp (LostTrackRecoveredWhenReassociated) | tests/tracking/test_tracker/test_association_stages.cpp; tests/tracking/test_tracker/test_buffers.cpp
Small_TrackerDuplicateTest.OverlappingTracksDeduplicatedToOne | delete | 1 | forbidden: a loop and an if-count in the body; EXPECT_LE(tracked, 1) also passes with zero tracks; the same promise is tested in tests/tracking/test_tracker/test_duplicate_suppression.cpp (TransientDuplicateCollapsesToSingleIdentity, rewritten) | tests/tracking/test_tracker/test_association_stages.cpp; tests/tracking/test_tracker/test_duplicate_suppression.cpp
Small_TrackerTentativeTiersTest.HighScoreDetectionPromotesAfterOneConfirmingHit | keep | 9 | tracker: a tentative track is promoted after its confirm hits, at once for a fast score | tests/tracking/test_tracker/test_association_stages.cpp; tests/c_detection_builders.hpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerTentativeTiersTest.StandardScoreDetectionRequiresConfirmHitsBeforePromotion | split | 3 | two promises: a standard-score track is not promoted before its third hit, and it is promoted at the third hit; the checks between frames are the two promises | tests/tracking/test_tracker/test_association_stages.cpp; tests/c_detection_builders.hpp
Small_TrackerZeroDetectionTest.ZeroDetectionsOnEmptyTrackerReturnsZero | keep | 9 | tracker: an empty frame on an empty tracker returns zero tracks | tests/tracking/test_tracker/test_association_stages.cpp; tests/c_detection_builders.hpp
Small_TrackerZeroDetectionTest.NDetectionsProduceNTracks | delete | 4 | the same count promise at 25 objects is tested in tests/tracking/test_tracker/test_multi_object.cpp (CrowdedSceneKeepsDistinctStableIdentities, seed count) | tests/tracking/test_tracker/test_association_stages.cpp; tests/tracking/test_tracker/test_multi_object.cpp
```

### `tests/tracking/test_tracker/test_boundary_conditions.cpp`

```
Small_TrackerMalformedDetectionTest.ZeroWidthDetectionDoesNotCrash | merge | 4 | merge into one row table of degenerate and non-finite inputs (zero width, zero height, negative score, Inf, NaN feature) with the finite-geometry oracle, with ZeroHeightDetectionDoesNotCrash, NegativeScoreDoesNotProduceNaNGeometry, InfCoordinatesDoNotCrash and NaNFeatureVectorDoesNotPoisonTracker | tests/tracking/test_tracker/test_boundary_conditions.cpp; tests/c_detection_builders.hpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerMalformedDetectionTest.ZeroHeightDetectionDoesNotCrash | merge | 4 | merge into the degenerate-input row table with ZeroWidthDetectionDoesNotCrash | tests/tracking/test_tracker/test_boundary_conditions.cpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerMalformedDetectionTest.NegativeScoreDoesNotProduceNaNGeometry | merge | 4 | merge into the degenerate-input row table with ZeroWidthDetectionDoesNotCrash | tests/tracking/test_tracker/test_boundary_conditions.cpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerMalformedDetectionTest.NaNCoordinatesDoNotCorruptOtherTracks | rewrite | 1 | forbidden: a loop with an if in the body; the good track's position is never checked, so the promise (a NaN detection leaves the other track intact) is not tested; no other test covers it | tests/tracking/test_tracker/test_boundary_conditions.cpp; tests/c_detection_builders.hpp
Small_TrackerMalformedDetectionTest.InfCoordinatesDoNotCrash | merge | 4 | merge into the degenerate-input row table with ZeroWidthDetectionDoesNotCrash | tests/tracking/test_tracker/test_boundary_conditions.cpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerMalformedDetectionTest.NaNFeatureVectorDoesNotPoisonTracker | merge | 4 | merge into the degenerate-input row table with ZeroWidthDetectionDoesNotCrash; the test also destroys and rebuilds the handle in its body, which belongs in the row's setup | tests/tracking/test_tracker/test_boundary_conditions.cpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerLargeCoordinateTest.MillionPixelCoordinatesRemainFinite | merge | 4 | merge with HugeFrameToFrameJumpDoesNotProduceNonFinite (magnitude rows) | tests/tracking/test_tracker/test_boundary_conditions.cpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerLargeCoordinateTest.HugeFrameToFrameJumpDoesNotProduceNonFinite | merge | 4 | merge with MillionPixelCoordinatesRemainFinite (magnitude rows) | tests/tracking/test_tracker/test_boundary_conditions.cpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerHighDimensionalReIDTest.HighDimUnitFeaturePreservesIdentity | keep | 9 | tracker: a unit feature of 128, 512 or 2048 dimensions keeps its object's id across two frames | tests/tracking/test_tracker/test_boundary_conditions.cpp; tests/c_detection_builders.hpp
Small_TrackerHighDimensionalReIDZeroNormTest.ZeroNormFeatureIsToleratedWithoutNaN | rewrite | 1 | forbidden: a loop in the body repeats expect_finite_geometry, which the file already defines; the promise has no other test; the rewrite calls the helper | tests/tracking/test_tracker/test_boundary_conditions.cpp; tests/c_detection_builders.hpp
Small_TrackerLongRunStabilityTest.OneThousandFramesStationaryNoNumericalDrift | rewrite | 1 | forbidden: a loop with an if on the frame index in the body; the name claims no drift but the oracle checks finiteness only; the rewrite asserts a derived bound on position over the run | tests/tracking/test_tracker/test_boundary_conditions.cpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerSimilarityBoundaryTest.NearUnitySimilarityIsAccepted | rewrite | 2 | the assertions (at least one track per frame) would pass if identity were lost; no other test checks this promise, so rewrite to assert the same id across the two frames | tests/tracking/test_tracker/test_boundary_conditions.cpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerSimilarityBoundaryTest.NearOrthogonalSimilarityIsAccepted | rewrite | 2 | the same weak assertions; no other test checks this promise, so rewrite to assert a fresh id for the far detection | tests/tracking/test_tracker/test_boundary_conditions.cpp; include/SushiTrack/c_api/sushitrack_c.h
```

### `tests/tracking/test_tracker/test_buffers.cpp`

```
Small_TrackerBuffersTest.TrackedTransitionsToLostOnFirstMiss | merge | 4 | merge with LostTimeAccumulatesAcrossMissedFrames (lost-time rows over miss count) | tests/tracking/test_tracker/test_buffers.cpp; tests/c_detection_builders.hpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerBuffersTest.LostTimeAccumulatesAcrossMissedFrames | merge | 4 | merge with TrackedTransitionsToLostOnFirstMiss (lost-time rows over miss count) | tests/tracking/test_tracker/test_buffers.cpp; tests/c_detection_builders.hpp
Small_TrackerBuffersTest.LostTrackRemovedAfterBufferExpiry | keep | 9 | tracker: a lost track past its buffer is removed and leaves the output | tests/tracking/test_tracker/test_buffers.cpp; tests/c_detection_builders.hpp
Small_TrackerBuffersTest.LostTrackRecoveredWhenReassociated | keep | 9 | tracker: a lost track re-associated in its window returns to tracked with lost time zero | tests/tracking/test_tracker/test_buffers.cpp; tests/c_detection_builders.hpp
Small_TrackerBuffersTest.RemovedTrackAbsentFromNextFrameOutput | keep | 9 | tracker: a removed track is absent from the next output | tests/tracking/test_tracker/test_buffers.cpp; tests/c_detection_builders.hpp
Small_TrackerBuffersTest.MultipleTracksRetainIndependentIds | delete | 4 | the distinct-id promise at 25 objects is tested in tests/tracking/test_tracker/test_multi_object.cpp (CrowdedSceneKeepsDistinctStableIdentities) | tests/tracking/test_tracker/test_buffers.cpp; tests/tracking/test_tracker/test_multi_object.cpp
Small_TrackerBuffersTest.MultipleTrackIndependentLostState | rewrite | 1 | forbidden: loops with EXPECT in the body; the only test of independent Lost states | tests/tracking/test_tracker/test_buffers.cpp; tests/c_detection_builders.hpp
Small_TrackerBuffersLargeBufferTest.DifferentBufferWindowAffectsExpiry | rewrite | 1 | forbidden: a loop in the body; EXPECT_LT 0.4 accepts any lost time; the rewrite compares the two-miss and five-miss outcomes as rows over track_buffer | tests/tracking/test_tracker/test_buffers.cpp; tests/c_detection_builders.hpp
```

### `tests/tracking/test_tracker/test_byte_association.cpp`

```
Small_TrackerByteAssociationTest.ConfirmedTrackSustainedByLowConfidenceDetections | rewrite | 1 | forbidden: loops with ASSERT and EXPECT in the body; three promises (survives low frames, no second identity, recovers on a high score) split when rewritten | tests/tracking/test_tracker/test_byte_association.cpp; tests/c_detection_builders.hpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerByteAssociationTest.LowConfidenceDetectionDoesNotSeedNewTrack | rewrite | 1 | forbidden: a loop with EXPECT in the body; same promise as MidConfidenceDetectionDoesNotSeedNewTrack, merge as rows over score | tests/tracking/test_tracker/test_byte_association.cpp; tests/c_detection_builders.hpp
Small_TrackerByteAssociationTest.MidConfidenceDetectionDoesNotSeedNewTrack | rewrite | 1 | forbidden: a loop with EXPECT in the body; same promise as LowConfidenceDetectionDoesNotSeedNewTrack, merge as rows over score | tests/tracking/test_tracker/test_byte_association.cpp; tests/c_detection_builders.hpp
```

### `tests/tracking/test_tracker/test_config_variations.cpp`

```
Small_TrackerIoUVariantTest.EachIoUVariantTracksSingleObjectStably | rewrite | 1 | forbidden: a loop with ASSERT in the helper the body calls; the helper asserts but carries no expect_ or assert_ name; the four iou_type rows are already a parametrised test | tests/tracking/test_tracker/test_config_variations.cpp; tests/c_detection_builders.hpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerCoordinateSystemTest.XyahModeHandlesAspectChange | rewrite | 1 | forbidden: a loop with ASSERT in the body; the identity-under-aspect-change promise has no other test; the name states a feature ("handles") | tests/tracking/test_tracker/test_config_variations.cpp; tests/c_detection_builders.hpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerKalmanVariantTest.NsaFilterHandlesFluctuatingConfidence | rewrite | 1 | forbidden: a loop with ASSERT over the score array in the body; the name states a feature ("handles") | tests/tracking/test_tracker/test_config_variations.cpp; tests/c_detection_builders.hpp; include/SushiTrack/c_api/sushitrack_c.h
```

### `tests/tracking/test_tracker/test_duplicate_suppression.cpp`

```
Small_TrackerDuplicateSuppressionTest.TransientDuplicateCollapsesToSingleIdentity | rewrite | 1 | forbidden: loops with ASSERT in the body; the same collapse promise as CoincidentDetectionsDoNotEstablishTwinIdentities, merge as one row table when rewritten | tests/tracking/test_tracker/test_duplicate_suppression.cpp; tests/c_detection_builders.hpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerDuplicateSuppressionTest.CoincidentDetectionsDoNotEstablishTwinIdentities | rewrite | 1 | forbidden: a loop with ASSERT in the body; the same promise as TransientDuplicateCollapsesToSingleIdentity, merge as one row table when rewritten | tests/tracking/test_tracker/test_duplicate_suppression.cpp; tests/c_detection_builders.hpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerDuplicateSuppressionTest.DuplicateDropLogsBothTrackIds | rewrite | 1 | forbidden: a loop in the body; the name says both track ids are logged, but the body checks a count, the level and the word "Frame" only; the log is process-wide, so the rewrite uses an in-memory sink (question 8) | tests/tracking/test_tracker/test_duplicate_suppression.cpp; tests/captured_log.hpp; tests/c_detection_builders.hpp; include/SushiTrack/c_api/sushitrack_c.h
```

### `tests/tracking/test_tracker/test_end_to_end.cpp`

```
Small_TrackerLongSequenceTrackingTest.ThreeHundredFrameSingleObjectKeepsIdentity | rewrite | 1 | forbidden: a loop with EXPECT in the body; four promises (identity, monotonic age, finite geometry, zero update gap) split when rewritten | tests/tracking/test_tracker/test_end_to_end.cpp; tests/c_detection_builders.hpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerReIDEndToEndTest.DistinctFeaturesPreventIdSwapOnCrossover | rewrite | 1 | forbidden: a loop in the body; the checks prove only that both ids exist, so a swap passes; the rewrite asserts each id on its own side of the crossing | tests/tracking/test_tracker/test_end_to_end.cpp; tests/c_detection_builders.hpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerReIDEndToEndTest.LostTrackRecoveredByMatchingFeature | rewrite | 1 | forbidden: a loop of five misses in the body (a builder or a row holds the gap); the only ReID recovery through the C API | tests/tracking/test_tracker/test_end_to_end.cpp; tests/c_detection_builders.hpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerTentativeWithReIDTest.TentativeWithFeaturesConfirmsAfterHitCount | rewrite | 1 | forbidden: a loop with ASSERT in the body; the promise with features on has no other test; the hit-count promise without features is in test_association_stages.cpp | tests/tracking/test_tracker/test_end_to_end.cpp; tests/tracking/test_tracker/test_association_stages.cpp; tests/c_detection_builders.hpp
Small_TrackerTentativeWithReIDTest.TentativeReAcquiresAfterSingleMissedFrame | keep | 9 | tracker: a tentative track keeps its id across one missed frame; the id check is not in test_tentative_lifecycle.cpp, so there is no duplicate | tests/tracking/test_tracker/test_end_to_end.cpp; tests/tracking/test_tracker/test_tentative_lifecycle.cpp; tests/c_detection_builders.hpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerImmGatingEndToEndTest.ManeuveringTrackKeepsIdentityInsideGate | rewrite | 1 | forbidden: a loop with ASSERT in the body; the track moves at constant acceleration and no regime is asserted, so the name claims more than the test checks | tests/tracking/test_tracker/test_end_to_end.cpp; tests/c_detection_builders.hpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerImmGatingEndToEndTest.OutOfGateDetectionDoesNotBindEstablishedTrack | rewrite | 1 | forbidden: an if in the body; the branch where the track is absent asserts nothing, so the test passes if the track is removed; the docstring's Lost outcome is not checked | tests/tracking/test_tracker/test_end_to_end.cpp; tests/c_detection_builders.hpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerVariableDtTest.MixedFrameRateSequenceRetainsIdentity | delete | 1 | forbidden: a loop with ASSERT in the body; the same promise is tested in tests/tracking/test_tracker/test_time_aware.cpp (IrregularFrameIntervalsKeepIdentityStable, kept as rewrite) | tests/tracking/test_tracker/test_end_to_end.cpp; tests/tracking/test_tracker/test_time_aware.cpp
Small_TrackerVariableDtTest.OneSecondGapDoesNotCrash | rewrite | 6 | the oracle is no crash, which the testing skill allows only for a fuzz or stress target; the one assertion accepts any count from zero up; the rewrite needs the owner's statement of the state after a one-second gap | tests/tracking/test_tracker/test_end_to_end.cpp; tests/c_detection_builders.hpp
Small_TrackerHighToLowConfidenceTest.TrackSurvivesScoreTransitionFromHighToLow | delete | 4 | the same promise (a low-confidence detection keeps a confirmed track) is tested at a larger size in tests/tracking/test_tracker/test_byte_association.cpp (ConfirmedTrackSustainedByLowConfidenceDetections, three frames) | tests/tracking/test_tracker/test_end_to_end.cpp; tests/tracking/test_tracker/test_byte_association.cpp
```

### `tests/tracking/test_tracker/test_full_stack.cpp`

```
Small_TrackerFullStackTest.AdvancedStackTracksConfirmsAndRecovers | rewrite | 1 | forbidden: loops with ASSERT and EXPECT in the body; four promises (confirm, sustained identity, occlusion keeps the id, ReID recovery) split when rewritten; imm_filter.hpp is included and unused | tests/tracking/test_tracker/test_full_stack.cpp; tests/c_detection_builders.hpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerFullStackTest.LongRunUnderFullStackStaysStable | rewrite | 1 | forbidden: a loop and an if in the body; the id re-anchors to whatever track is tracked and confirmed, so an identity swap passes; the name "stays stable" describes a smoke run, not a promise | tests/tracking/test_tracker/test_full_stack.cpp; tests/c_detection_builders.hpp; include/SushiTrack/c_api/sushitrack_c.h
```

### `tests/tracking/test_tracker/test_kinematic_regimes.cpp`

```
Small_TrackerKinematicStateTest.StationaryObjectReportsStationary | rewrite | 1 | forbidden: a loop with ASSERT in the body; the only test of the stationary regime | tests/tracking/test_tracker/test_kinematic_regimes.cpp; tests/c_detection_builders.hpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerKinematicStateTest.SteadyMotionReportsCruising | rewrite | 1 | forbidden: a loop with ASSERT in the body; the velocity ramp belongs in a builder; the only test of the cruising regime | tests/tracking/test_tracker/test_kinematic_regimes.cpp; tests/c_detection_builders.hpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerKinematicStateTest.AbruptAccelerationReportsManeuvering | rewrite | 1 | forbidden: two loops and an if in the body; the check is an OR over frames, so one frame of the regime passes | tests/tracking/test_tracker/test_kinematic_regimes.cpp; tests/c_detection_builders.hpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerKinematicStateTest.OccludedTrackReportsCoasting | rewrite | 1 | forbidden: loops with ASSERT in the body; the regime is read from the Lost output, the only test of Coasting through the C API | tests/tracking/test_tracker/test_kinematic_regimes.cpp; tests/c_detection_builders.hpp; include/SushiTrack/c_api/sushitrack_c.h
```

### `tests/tracking/test_tracker/test_multi_object.cpp`

```
Small_TrackerMultiObjectTest.CrowdedSceneKeepsDistinctStableIdentities | rewrite | 1 | forbidden: a loop over frames with ASSERT and EXPECT in the body; several promises (seed count, distinct seed ids, identity set over frames) split when rewritten; the seed-count rows also cover NDetectionsProduceNTracks | tests/tracking/test_tracker/test_multi_object.cpp; tests/c_detection_builders.hpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerMultiObjectTest.CounterMovingSeparableObjectsDoNotSwapIds | rewrite | 1 | forbidden: a loop with ASSERT and EXPECT in the body; the no-swap promise has no other test | tests/tracking/test_tracker/test_multi_object.cpp; tests/c_detection_builders.hpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerMultiObjectTest.OutputRespectsMaxTracksCapacityAndPriority | rewrite | 1 | forbidden: loops in the body; the name joins capacity and priority, which split when rewritten; priority is not exercised because all six tracks are tracked | tests/tracking/test_tracker/test_multi_object.cpp; tests/c_detection_builders.hpp; include/SushiTrack/c_api/sushitrack_c.h
```

### `tests/tracking/test_tracker/test_observation_momentum.cpp`

```
Small_TrackerObservationMomentumTest.CruisingObjectKeepsStableId | rewrite | 1 | forbidden: a loop with EXPECT and an if in the body; the test passes with OCM off as well, so it does not test the momentum term | tests/tracking/test_tracker/test_observation_momentum.cpp; tests/c_detection_builders.hpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerObservationMomentumTest.StationaryObjectUnaffectedByMomentum | rewrite | 1 | forbidden: a loop in the body compares the boxes; the metamorphic OCM on and off oracle is sound and is kept in the rewrite | tests/tracking/test_tracker/test_observation_momentum.cpp; tests/c_detection_builders.hpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerObservationMomentumTest.ParallelCruisersKeepDistinctIds | rewrite | 1 | forbidden: loops with continue and an if in the body; the same blind spot as CruisingObjectKeepsStableId for the momentum term | tests/tracking/test_tracker/test_observation_momentum.cpp; tests/c_detection_builders.hpp; include/SushiTrack/c_api/sushitrack_c.h
```

### `tests/tracking/test_tracker/test_occlusion_recovery.cpp`

```
Small_TrackerOcclusionRecoveryTest.ShortOcclusionRecoversOriginalId | rewrite | 1 | forbidden: a gap loop in the body and a confirm helper with a loop and EXPECT; when rewritten, merge rows over gap length with LostTrackRecoveredWhenReassociated | tests/tracking/test_tracker/test_occlusion_recovery.cpp; tests/tracking/test_tracker/test_buffers.cpp; tests/c_detection_builders.hpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerOcclusionRecoveryTest.LongOcclusionRetiresTrackAndReassignsNewId | rewrite | 1 | forbidden: loops and an if in the body; the frame-counted retire promise also holds in test_buffers.cpp (LostTrackRemovedAfterBufferExpiry), so the rewrite keeps only the new-id check | tests/tracking/test_tracker/test_occlusion_recovery.cpp; tests/tracking/test_tracker/test_buffers.cpp; tests/c_detection_builders.hpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerOcclusionRecoveryTest.OneOfTwoObjectsRecoversWithoutIdTransfer | rewrite | 1 | forbidden: loops with ASSERT and an if in the body; the no-id-transfer promise has no other test | tests/tracking/test_tracker/test_occlusion_recovery.cpp; tests/c_detection_builders.hpp; include/SushiTrack/c_api/sushitrack_c.h
```

### `tests/tracking/test_tracker/test_profiling.cpp`

```
Medium_TrackerProfilingTest.EveryLevelOneZoneAppearsInEachFrame | rewrite | 1 | forbidden: a loop over periods with EXPECT in the body; zone presence is the sink's contract, checked in the rewrite through an in-memory sink (question 8) | tests/tracking/test_tracker/test_profiling.cpp; tests/profile_session.hpp
Medium_TrackerProfilingTest.TentativeStageNamesItsOwnZone | resize | 8 | the zone name is chosen on the tentative setting, a decision on values; the real session stamps samples from the clock, so the seam is an in-memory profiler sink port and the test runs small | tests/tracking/test_tracker/test_profiling.cpp; tests/profile_session.hpp
Medium_TrackerProfilingTest.OutputIsEqualWithTheGateOpenAndClosed | rewrite | 1 | forbidden: a loop with EXPECT in the body; the metamorphic oracle (gate open against closed) is sound and kept | tests/tracking/test_tracker/test_profiling.cpp; tests/profile_session.hpp
Medium_TrackerProfilingTest.LevelOneCountersMatchTheScenario | split | 3 | several promises in one test: the detections counter, the lifecycle counters, the match counters and the output counters are each a promise | tests/tracking/test_tracker/test_profiling.cpp; tests/profile_session.hpp
Medium_TrackerProfilingTest.OverflowIsCountedAsDroppedTracks | resize | 8 | the overflow count is a decision on values read back through the real session's clock-stamped record; the seam is an in-memory profiler sink port and the test runs small | tests/tracking/test_tracker/test_profiling.cpp; tests/profile_session.hpp
Medium_TrackerProfilingTest.LevelTwoRecordsAppearOnlyAtLevelTwo | rewrite | 1 | forbidden: an if with GTEST_SKIP in the body makes assertions conditional on the build; two promises (absent at level 1, present at level 2) split when rewritten | tests/tracking/test_tracker/test_profiling.cpp; tests/profile_session.hpp
```

### `tests/tracking/test_tracker/test_tentative_lifecycle.cpp`

```
Small_TrackerTentativeTest.AgesOutViaSecondsBudgetWhenTimeAware | merge | 4 | merge with SingleMissDoesNotExpire (lost-time rows around the 0.08 s budget) | tests/tracking/test_tracker/test_tentative_lifecycle.cpp; tests/c_detection_builders.hpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerTentativeFrameBudgetTest.AgesOutViaFrameBudgetWhenFrameAware | rewrite | 7 | a check sits between each miss (sign of a check between actions); rows over the miss count and over dt, which the frame mode ignores | tests/tracking/test_tracker/test_tentative_lifecycle.cpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerTentativeTest.PromotedToTrackedAtHitThreshold | delete | 4 | the same three-hit promotion is tested in tests/tracking/test_tracker/test_association_stages.cpp (StandardScoreDetectionRequiresConfirmHitsBeforePromotion, split); the hits equal three check moves there | tests/tracking/test_tracker/test_tentative_lifecycle.cpp; tests/tracking/test_tracker/test_association_stages.cpp
Small_TrackerTentativeTest.PartialHitSequenceResetsLostTimeOnMatch | split | 3 | two promises: a miss accumulates lost time, and a match resets it and adds a hit; the checks between the three frames separate them | tests/tracking/test_tracker/test_tentative_lifecycle.cpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerTentativeTest.SingleMissDoesNotExpire | merge | 4 | merge with AgesOutViaSecondsBudgetWhenTimeAware (boundary rows: exactly the budget keeps the track); its EXPECT_NE on state accepts any non-tracked state, so the row names state 4 | tests/tracking/test_tracker/test_tentative_lifecycle.cpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerTentativeTest.TwoSimultaneousTentativeTracksIndependent | rewrite | 1 | forbidden: a loop with an if and else counting in the body; the rewrite matches each track by its position | tests/tracking/test_tracker/test_tentative_lifecycle.cpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerTentativeTest.TentativePromotedTrackPersistsAfterPromotion | keep | 9 | tracker: a promoted track keeps its id and adds a hit on each later match | tests/tracking/test_tracker/test_tentative_lifecycle.cpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerTentativeInitSuppressTest.OverlappingDetectionDoesNotInitNewTentative | merge | 4 | merge with OverlappingDetectionInitsTentativeWhenGateDisabled (suppression on and off rows) | tests/tracking/test_tracker/test_tentative_lifecycle.cpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerTentativeInitSuppressTest.OverlappingDetectionInitsTentativeWhenGateDisabled | merge | 4 | merge with OverlappingDetectionDoesNotInitNewTentative (suppression on and off rows) | tests/tracking/test_tracker/test_tentative_lifecycle.cpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerTentativeDisabledTest.WithTentativeDisabledNewTrackIsImmediatelyTracked | keep | 9 | tracker: with tentative off a high detection makes a tracked track at once | tests/tracking/test_tracker/test_tentative_lifecycle.cpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerTentativeRecordsTest.LowPoolConfirmationLogsTheConfirmation | resize | 8 | installs CapturedLog on the process-wide logger; the log line depends on values, so the seam is an in-memory log sink passed to the tracker; the test then runs small | tests/tracking/test_tracker/test_tentative_lifecycle.cpp; tests/captured_log.hpp
Small_TrackerTentativeRecordsTest.EveryLifecycleRecordCarriesItsFrame | rewrite | 1 | forbidden: a loop with an if and a continue in the body over captured records; the process-wide log (question 8) is replaced by the in-memory sink in the rewrite | tests/tracking/test_tracker/test_tentative_lifecycle.cpp; tests/captured_log.hpp
```

### `tests/tracking/test_tracker/test_time_aware.cpp`

```
Small_TrackerTimeAwareTest.SubBudgetRealTimeGapRecoversId | rewrite | 1 | forbidden: a gap loop in the body and a confirm helper with a loop and EXPECT; merge rows with OverBudgetRealTimeGapRetiresTrack when rewritten (gap against budget) | tests/tracking/test_tracker/test_time_aware.cpp; tests/c_detection_builders.hpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerTimeAwareTest.OverBudgetRealTimeGapRetiresTrack | rewrite | 1 | forbidden: loops and an if in the body; merge rows with SubBudgetRealTimeGapRecoversId when rewritten | tests/tracking/test_tracker/test_time_aware.cpp; tests/c_detection_builders.hpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerTimeAwareTest.IrregularFrameIntervalsKeepIdentityStable | rewrite | 1 | forbidden: a loop with ASSERT and an if in the body; the id re-anchors to out[0] whenever it is tracked, which can hide an identity swap; survivor for the irregular-interval promise once MixedFrameRateSequenceRetainsIdentity is deleted | tests/tracking/test_tracker/test_time_aware.cpp; tests/tracking/test_tracker/test_end_to_end.cpp; tests/c_detection_builders.hpp; include/SushiTrack/c_api/sushitrack_c.h
Small_TrackerTimeAwareTest.ExcessiveDtIsCappedAndGeometryStaysFinite | rewrite | 1 | forbidden: a loop with ASSERT in the body; the name claims a cap, but the checks test finiteness and continuity only; the cap is checked at policy level in tests/tracking/test_wall_clock_timing_policy.cpp (LargeDtIsCapped) | tests/tracking/test_tracker/test_time_aware.cpp; tests/tracking/test_wall_clock_timing_policy.cpp; tests/c_detection_builders.hpp
```

### `tests/tracking/test_tracker_config.cpp`

```
Small_TrackerConfigTest.DefaultThresholdsAreOrderedAndInRange | split | 3 | two promises joined by "and": the ordering of track_thresh below high_thresh and the range of each fraction; the fraction checks loop over a braced list in the body | tests/tracking/test_tracker_config.cpp; include/SushiTrack/tracking/tracker_config.hpp
Small_TrackerConfigTest.DefaultGateIsTheFourDegreeChiSquareBound | rewrite | 1 | forbidden: a default value tested as a constant, and bare tolerance 1e-4f in EXPECT_NEAR; the chi-square bound is the promise, so the rewrite checks a distance just inside and just outside the bound, or derives the tolerance | tests/tracking/test_tracker_config.cpp; include/SushiTrack/tracking/tracker_config.hpp
```

### `tests/tracking/test_tracking_log_category.cpp`

```
Small_TrackingLogCategoryTest.NamesTheProjectAndTheModule | delete | 1 | forbidden: a log category asserted as a string; the logging tests use the constant itself, not its strings | tests/tracking/test_tracking_log_category.cpp; include/SushiTrack/tracking/tracking_log_category.hpp
```

### `tests/tracking/test_tracklet/test_appearance_features.cpp`

```
Small_TrackletAppearanceTest.NoFeatureInitially | merge | 4 | merge with HasFeatureAfterUpdate (has_feature rows) | tests/tracking/test_tracklet/test_appearance_features.cpp; include/SushiTrack/tracking/tracklet.hpp
Small_TrackletAppearanceTest.HasFeatureAfterUpdate | merge | 4 | merge with NoFeatureInitially (has_feature rows) | tests/tracking/test_tracklet/test_appearance_features.cpp; include/SushiTrack/tracking/tracklet.hpp
Small_TrackletAppearanceTest.SmoothFeatIsUnitNormalised | rewrite | 1 | forbidden: bare tolerance 1e-5f in EXPECT_NEAR; the surviving normalisation test once UpdateFeaturesNormalisesVector is deleted; the tolerance is derived from the helper | tests/tracking/test_tracklet/test_appearance_features.cpp; tests/tracking/test_tracklet/test_lifecycle.cpp; include/SushiTrack/tracking/tracklet.hpp
Small_TrackletAppearanceTest.CurrFeatIsUnitNormalised | rewrite | 1 | forbidden: bare tolerance 1e-5f in EXPECT_NEAR; the only test of curr_feat normalisation; merge rows with SmoothFeatIsUnitNormalised when rewritten | tests/tracking/test_tracklet/test_appearance_features.cpp; include/SushiTrack/tracking/tracklet.hpp
Small_TrackletAppearanceTest.EMABlendsConsecutiveFeatures | keep | 9 | tracklet_appearance: the EMA keeps a component of each consecutive feature; the docstring says alpha 0.9 but the test passes 0.5, so the comment is wrong, not the verdict | tests/tracking/test_tracklet/test_appearance_features.cpp; include/SushiTrack/tracking/tracklet.hpp
Small_TrackletAppearanceTest.SimilarityIdenticalFeatureIsOne | rewrite | 1 | forbidden: bare tolerance 1e-4f in EXPECT_NEAR; the surviving identity test once the lifecycle copy is deleted; the dot product of a unit basis with itself is exact, so EXPECT_FLOAT_EQ applies | tests/tracking/test_tracklet/test_appearance_features.cpp; tests/tracking/test_tracklet/test_lifecycle.cpp; include/SushiTrack/tracking/tracklet.hpp
Small_TrackletAppearanceTest.SimilarityOrthogonalFeatureIsZero | rewrite | 1 | forbidden: bare bound 0.05f standing in for a tolerance; the orthogonal unit basis gives exactly zero, so EXPECT_FLOAT_EQ applies | tests/tracking/test_tracklet/test_appearance_features.cpp; include/SushiTrack/tracking/tracklet.hpp
Small_TrackletAppearanceTest.SimilarityChecksHistory | rewrite | 1 | forbidden: a loop in the body builds 30 updates; the bound 0.5 is loose, and the original vector is exactly in the history (capacity 50), so the similarity is one | tests/tracking/test_tracklet/test_appearance_features.cpp; include/SushiTrack/tracking/tracklet.hpp
Small_TrackletAppearanceTest.HistoryDoesNotExceedCapacity | delete | 1 | forbidden: a loop in the body; the same capacity promise is in tests/tracking/test_tracklet/test_lifecycle.cpp (FeatureHistoryCapIsRespected, kept as rewrite) | tests/tracking/test_tracklet/test_appearance_features.cpp; tests/tracking/test_tracklet/test_lifecycle.cpp
Small_TrackletAppearanceTest.EmptyFeatureVectorIsNoOp | keep | 9 | tracklet_appearance: an empty feature update leaves has_feature false; the comment inside the body repeats implementation detail and needs rewriting, outside this verdict | tests/tracking/test_tracklet/test_appearance_features.cpp; include/SushiTrack/tracking/tracklet.hpp
Small_TrackletAppearanceFeaturesTest.ChangingFeatureLengthWarnsOnce | resize | 8 | the test installs CapturedLog on the process-wide logger; the warning depends on values (the lengths), so the seam is an in-memory log sink passed to the tracklet; the test then runs small; the suite prefix differs from the file's other suite | tests/tracking/test_tracklet/test_appearance_features.cpp; tests/captured_log.hpp; tests/tracklet_builders.hpp; include/SushiTrack/tracking/tracklet.hpp
```

### `tests/tracking/test_tracklet/test_lifecycle.cpp`

```
Small_TrackletTest.ConstructionStateIsNew | keep | 9 | tracklet: a new tracklet is New, not activated and not confirmed | tests/tracking/test_tracklet/test_lifecycle.cpp; include/SushiTrack/tracking/tracklet.hpp
Small_TrackletTest.ActivateOnFrameOne | merge | 4 | merge with ActivateOnLaterFrameIsNotActivated (frame-number rows of activation) | tests/tracking/test_tracklet/test_lifecycle.cpp; include/SushiTrack/tracking/tracklet.hpp
Small_TrackletTest.ActivateOnLaterFrameIsNotActivated | merge | 4 | merge with ActivateOnFrameOne (frame-number rows of activation) | tests/tracking/test_tracklet/test_lifecycle.cpp; include/SushiTrack/tracking/tracklet.hpp
Small_TrackletTest.ActivateTentativeState | keep | 9 | tracklet: activate_tentative makes an unconfirmed, unactivated tentative track | tests/tracking/test_tracklet/test_lifecycle.cpp; include/SushiTrack/tracking/tracklet.hpp
Small_TrackletTest.ConfirmPromotesTentative | keep | 9 | tracklet: confirm promotes a tentative track and takes its final id | tests/tracking/test_tracklet/test_lifecycle.cpp; include/SushiTrack/tracking/tracklet.hpp
Small_TrackletTest.MarkAsLostSetsLostState | keep | 9 | tracklet: mark_as_lost sets Lost and clears activation | tests/tracking/test_tracklet/test_lifecycle.cpp; include/SushiTrack/tracking/tracklet.hpp
Small_TrackletTest.MarkAsLostInitialisesAccumulatedTime | merge | 4 | merge with MarkAsLostWithZeroDtInitialisesToZeroOverride (lost-time rows over dt) | tests/tracking/test_tracklet/test_lifecycle.cpp; include/SushiTrack/tracking/tracklet.hpp
Small_TrackletTest.MarkAsRemovedSetsRemovedState | keep | 9 | tracklet: mark_as_removed sets Removed | tests/tracking/test_tracklet/test_lifecycle.cpp; include/SushiTrack/tracking/tracklet.hpp
Small_TrackletTest.ReActivateRestoresTrackedState | keep | 9 | tracklet: re_activate restores Tracked and resets lost time | tests/tracking/test_tracklet/test_lifecycle.cpp; include/SushiTrack/tracking/tracklet.hpp
Small_TrackletTest.PredictIncrementsLostTimeForLostTrack | rewrite | 1 | forbidden: bare tolerance 1e-5f in EXPECT_NEAR; the only test of the lost-track increment; the sum 0 plus 0.04f is exact, so EXPECT_FLOAT_EQ applies | tests/tracking/test_tracklet/test_lifecycle.cpp; include/SushiTrack/tracking/tracklet.hpp
Small_TrackletTest.PredictDoesNotIncrementLostTimeForTrackedTrack | merge | 4 | merge with PredictIncrementsLostTimeForLostTrack (state rows of predict) | tests/tracking/test_tracklet/test_lifecycle.cpp; include/SushiTrack/tracking/tracklet.hpp
Small_TrackletTest.UpdateKeepStatePreservesTrackState | merge | 4 | merge with UpdateNoKeepStatePromotesToTracked (keep_state rows) | tests/tracking/test_tracklet/test_lifecycle.cpp; include/SushiTrack/tracking/tracklet.hpp
Small_TrackletTest.UpdateNoKeepStatePromotesToTracked | merge | 4 | merge with UpdateKeepStatePreservesTrackState (keep_state rows) | tests/tracking/test_tracklet/test_lifecycle.cpp; include/SushiTrack/tracking/tracklet.hpp
Small_TrackletTest.UpdateFeaturesNormalisesVector | delete | 1 | forbidden: bare tolerance 1e-5f in EXPECT_NEAR; the same normalisation promise is tested in tests/tracking/test_tracklet/test_appearance_features.cpp (SmoothFeatIsUnitNormalised, rewritten) | tests/tracking/test_tracklet/test_lifecycle.cpp; tests/tracking/test_tracklet/test_appearance_features.cpp
Small_TrackletTest.FeatureSimilarityIdenticalVsOrthogonal | delete | 1 | forbidden: a loop in the body builds the unit vector; bare tolerances 1e-4f and 0.1f; both halves are tested in tests/tracking/test_tracklet/test_appearance_features.cpp (SimilarityIdenticalFeatureIsOne, SimilarityOrthogonalFeatureIsZero, both rewritten) | tests/tracking/test_tracklet/test_lifecycle.cpp; tests/tracking/test_tracklet/test_appearance_features.cpp
Small_TrackletTest.FeatureHistoryCapIsRespected | rewrite | 1 | forbidden: a loop in the body builds the feature stream; EXPECT_LE accepts any count under the cap and the eviction named in the docstring is not asserted; this is the surviving capacity test once HistoryDoesNotExceedCapacity is deleted | tests/tracking/test_tracklet/test_lifecycle.cpp; tests/tracking/test_tracklet/test_appearance_features.cpp; include/SushiTrack/tracking/tracklet.hpp
Small_TrackletTest.PredictIncrementsLostTimeForTentativeTrack | delete | 1 | forbidden: bare tolerance 1e-5f in EXPECT_NEAR; the same promise is in this file as PredictIncrementsLostTimeForTentative | tests/tracking/test_tracklet/test_lifecycle.cpp; include/SushiTrack/tracking/tracklet.hpp
Small_TrackletTest.ActivateTentativeInitialisesLostTimeToZero | delete | 4 | duplicate of ActivateTentativeSetsLostTimeZero in this file: the same call and the same promise at the same size; that test also checks the state | tests/tracking/test_tracklet/test_lifecycle.cpp; include/SushiTrack/tracking/tracklet.hpp
Small_TrackletTest.MarkAsLostWithZeroDtInitialisesToZero | delete | 4 | duplicate of MarkAsLostWithZeroDtInitialisesToZeroOverride in this file: zero dt, same promise, same size; the override also checks the state | tests/tracking/test_tracklet/test_lifecycle.cpp; include/SushiTrack/tracking/tracklet.hpp
Small_TrackletTest.ReActivatePreservesTrackId | keep | 9 | tracklet: re_activate keeps the track id | tests/tracking/test_tracklet/test_lifecycle.cpp; include/SushiTrack/tracking/tracklet.hpp
Small_TrackletTest.HitsIncrementOnConsecutiveUpdates | keep | 9 | tracklet: each matched update adds one hit | tests/tracking/test_tracklet/test_lifecycle.cpp; include/SushiTrack/tracking/tracklet.hpp
Small_TrackletTest.PredictIncrementsLostTimeForTentative | rewrite | 1 | forbidden: bare tolerance 1e-5f in EXPECT_NEAR; the surviving copy of the tentative increment once PredictIncrementsLostTimeForTentativeTrack is deleted; 0 plus 1.0f is exact | tests/tracking/test_tracklet/test_lifecycle.cpp; include/SushiTrack/tracking/tracklet.hpp
Small_TrackletTest.ActivateTentativeSetsLostTimeZero | keep | 9 | tracklet: activate_tentative starts the lost-time counter at zero | tests/tracking/test_tracklet/test_lifecycle.cpp; include/SushiTrack/tracking/tracklet.hpp
Small_TrackletTest.MarkAsLostWithZeroDtInitialisesToZeroOverride | merge | 4 | merge with MarkAsLostInitialisesAccumulatedTime (lost-time rows over dt) | tests/tracking/test_tracklet/test_lifecycle.cpp; include/SushiTrack/tracking/tracklet.hpp
Small_TrackletTest.ActivateRecordsObservationWithoutVelocity | keep | 9 | tracklet: a fresh observation is recorded with no velocity | tests/tracking/test_tracklet/test_lifecycle.cpp; include/SushiTrack/tracking/tracklet.hpp
Small_TrackletTest.PredictIncrementsFramesSinceObservation | rewrite | 7 | a check sits between two predict calls (sign of a check between actions); rows over the number of unobserved predicts | tests/tracking/test_tracklet/test_lifecycle.cpp; include/SushiTrack/tracking/tracklet.hpp
Small_TrackletTest.UpdateResetsFramesSinceObservation | keep | 9 | tracklet: frames since observation advances per unobserved predict and resets on a match; the ASSERT before the update is setup only | tests/tracking/test_tracklet/test_lifecycle.cpp; include/SushiTrack/tracking/tracklet.hpp
Small_TrackletTest.ObservationVelocityFollowsHorizontalMotion | rewrite | 1 | forbidden: a loop in the body builds the observation stream; bare tolerance 1e-3f in three EXPECT_NEAR; rows shared with ObservationVelocitySignTracksDirection | tests/tracking/test_tracklet/test_lifecycle.cpp; include/SushiTrack/tracking/tracklet.hpp
Small_TrackletTest.ObservationVelocitySignTracksDirection | rewrite | 1 | forbidden: a loop in the body and bare tolerance 1e-3f; the reversed-motion case is a row of the same direction table as ObservationVelocityFollowsHorizontalMotion | tests/tracking/test_tracklet/test_lifecycle.cpp; include/SushiTrack/tracking/tracklet.hpp
Small_TrackletTest.LastObservationIsRawDetection | keep | 9 | tracklet: the last observation is the raw detection, not the estimate | tests/tracking/test_tracklet/test_lifecycle.cpp; include/SushiTrack/tracking/tracklet.hpp
```

### `tests/tracking/test_wall_clock_timing_policy.cpp`

```
Small_WallClockTimingPolicyTest.OneFpsIntervalConvertsToOne | merge | 4 | merge with NearOneSnapsToPreciselyOne (snap rows); the one-frame case also appears in tests/tracking/test_timing_policy.cpp | tests/tracking/test_wall_clock_timing_policy.cpp; include/SushiTrack/tracking/wall_clock_timing_policy.hpp; tests/tracking/test_timing_policy.cpp
Small_WallClockTimingPolicyTest.LargeDtIsCapped | merge | 4 | merge with ExactCapBoundaryReturnsCap (cap rows) | tests/tracking/test_wall_clock_timing_policy.cpp; include/SushiTrack/tracking/wall_clock_timing_policy.hpp
Small_WallClockTimingPolicyTest.NearOneSnapsToPreciselyOne | merge | 4 | merge with OneFpsIntervalConvertsToOne (snap rows) | tests/tracking/test_wall_clock_timing_policy.cpp; include/SushiTrack/tracking/wall_clock_timing_policy.hpp
Small_WallClockTimingPolicyTest.HalfFrameScalesCorrectly | rewrite | 1 | forbidden: bare tolerance 1e-4f in EXPECT_NEAR; the only test of linear scaling; use fps 16 and dt 1/32, both exact, so the result is exactly 0.5 and EXPECT_FLOAT_EQ applies | tests/tracking/test_wall_clock_timing_policy.cpp; include/SushiTrack/tracking/wall_clock_timing_policy.hpp
Small_WallClockTimingPolicyTest.IsExpiredFalseWhenUnderThreshold | merge | 4 | merge with IsExpiredTrueWhenOverThreshold (elapsed-time rows around the timeout) | tests/tracking/test_wall_clock_timing_policy.cpp; include/SushiTrack/tracking/wall_clock_timing_policy.hpp
Small_WallClockTimingPolicyTest.IsExpiredFalseAtExactBoundary | merge | 4 | merge with IsExpiredTrueWhenOverThreshold; its timeout repeats the implementation's float expression, so the row should state 30 frames at 25 fps as the specified value | tests/tracking/test_wall_clock_timing_policy.cpp; include/SushiTrack/tracking/wall_clock_timing_policy.hpp
Small_WallClockTimingPolicyTest.IsExpiredTrueWhenOverThreshold | merge | 4 | merge with IsExpiredFalseWhenUnderThreshold (same row table) | tests/tracking/test_wall_clock_timing_policy.cpp; include/SushiTrack/tracking/wall_clock_timing_policy.hpp
Small_WallClockTimingPolicyTest.ZeroFpsClampedToOneHz | split | 3 | two promises: the clamp to one fps (to_kf_dt) and the timeout under it (is_expired at buffer 5); the is_expired half joins the elapsed-time rows | tests/tracking/test_wall_clock_timing_policy.cpp; include/SushiTrack/tracking/wall_clock_timing_policy.hpp
Small_WallClockTimingPolicyTest.NegativeDtProducesNonPositiveKfDt | keep | 9 | wall_clock_timing_policy: a negative interval gives a non-positive step | tests/tracking/test_wall_clock_timing_policy.cpp; include/SushiTrack/tracking/wall_clock_timing_policy.hpp
Small_WallClockTimingPolicyTest.ExactCapBoundaryReturnsCap | merge | 4 | merge with LargeDtIsCapped (cap rows) | tests/tracking/test_wall_clock_timing_policy.cpp; include/SushiTrack/tracking/wall_clock_timing_policy.hpp
Small_WallClockTimingPolicyTest.SmallEpsilonAboveBoundaryExpires | merge | 4 | merge with IsExpiredTrueWhenOverThreshold (expiry rows) | tests/tracking/test_wall_clock_timing_policy.cpp; include/SushiTrack/tracking/wall_clock_timing_policy.hpp
```

### `tools/tests/test_check_test_additions.py`

```
CheckTestAdditionsTest.test_a_commit_at_its_limit_passes_and_one_over_it_is_named | delete | 3 | tools/testing/check_test_additions.py was removed from the shared tools upstream (sushiskills tools/testing holds no such file); the copy in this repository and its test are a leftover | tools/tests/test_check_test_additions.py
CheckTestAdditionsTest.test_a_fix_has_the_smaller_limit | delete | 3 | the same removal from the shared tools; leftover copy | tools/tests/test_check_test_additions.py
CheckTestAdditionsTest.test_a_second_program_needs_the_owners_line | delete | 3 | the same removal from the shared tools; leftover copy | tools/tests/test_check_test_additions.py
CheckTestAdditionsTest.test_the_owners_line_approves_a_second_program | delete | 3 | the same removal from the shared tools; leftover copy | tools/tests/test_check_test_additions.py
CheckTestAdditionsTest.test_a_repository_that_sets_no_base_commit_is_not_checked | delete | 3 | the same removal from the shared tools; leftover copy | tools/tests/test_check_test_additions.py
```

### `tools/tests/test_test_budget_settings.py`

```
TestBudgetSettingsTest.test_the_skill_table_matches_the_settings | rewrite | 1 | forbidden kind: control flow in the body (the skipTest guard); it is the only test of the promise that the skill's Budget table equals K_TEST_BUDGET; this repository has no skills/ folder, so the test always skips and checks nothing here; rewrite with the skip moved into the fixture | tools/tests/test_test_budget_settings.py; tools/settings.py
TestBudgetSettingsTest.test_reads_a_number_with_a_thousands_space_and_a_trailing_note | split | 3 | several promises in one body: a thousands-spaced number parses, a trailing note is dropped, a row no key names is ignored, rows outside the Budget heading are ignored; read_budget is defined in this test file, not in a module, so it needs a home before the split tests can import it | tools/tests/test_test_budget_settings.py
TestBudgetSettingsTest.test_names_each_key_that_differs | resize | 8 | differences() names each differing key in sorted order, None for a missing side; plain values, so small; the file has no size label, which fails collection; resize to small, no seam needed | tools/tests/test_test_budget_settings.py; tools/settings.py
TestBudgetSettingsTest.test_every_row_of_the_table_has_a_constant | resize | 8 | the label map and K_TEST_BUDGET name the same keys; a comparison of values, so small; Q4: the skill-table test checks the same key set at a larger size, so this smaller copy stays; no size label, resize to small | tools/tests/test_test_budget_settings.py; tools/settings.py
```

## Adds

Promises a header or a port states that no test checks: `unit: promise | oracle | size`.

```
docker.build: a failed docker build returns the child's exit code and prints the failure | fake runner returning a non-zero code, the code returned | small
deploy.deploy: an unknown kind is refused with exit code 2, names the choices, and builds nothing | specified value | small
label.sequences: with private set, only MOT sequences whose name ends in YOLOX are returned; with public set, the others | specified value | small
label.sequences: a scenario matches an exact name first, then a case-insensitive substring | specified value | small
diag.status: a missing build tree is shown as absent and status returns 0 | specified value | medium
tests.module_tests: run_cli_tests passes the label selection to pytest as the -m expression, run in the cli folder | specified value on the argv, runner stubbed | medium
FrameRecorder: a record that cannot be written sets error and logs ERROR | contract: the error text and the ERROR record | medium
FrameRecorder: events the capture loses are counted in dropped and logged WARN | contract: the dropped count and the WARN text | medium
run_mot_sequence: a predictions path that cannot be written throws runtime_error | contract: the exception type | medium
run_mot_sequence: predictions rows are `frame,id,x,y,w,h,score,-1,-1,-1` | specified format, through a stream seam | small
read_frame_rate: a frameRate that is not positive gives the fallback and one WARN | contract: fallback value and WARN text | small
read_json_settings: a file that is not valid JSON reads as an empty object and logs ERROR | contract: empty object and ERROR text | medium
json_section: the named section is returned when present | contract: the section's value | small
ByteTrackAdapter: values in its settings section replace the defaults | specified value | medium
OCSORTAdapter: values in its settings section replace the defaults | specified value | medium
SushiTrackAdapter: a parameter file that does not load keeps the defaults and logs one WARN | contract: defaults and WARN text | medium
delete_tracker: a NULL tracker is ignored | contract: no crash and no effect | small
tracker_process_c: a null tracker is refused with INVALID_ARGUMENT and leaves num_results alone | contract: status and num_results | small
tracker_process_c: a tracker whose update throws yields ERROR and zero results | contract: status and num_results; needs an input that makes update throw, or a seam | small
TrackerRegistry: a second registration replaces the first creator | contract: the later creator's adapter name | small
ReIDFirstCostCalculator: a pair with features beyond the threshold costs (1 - w) * reid_dist + w * iou_d | formula at source/association/reid_first_cost_calculator.cpp:130 with hand values | small
Configuration loader: SUSHITRACK_CONFIG naming an existing file loads it when the path argument is null | source/configuration/config_loader.cpp:63-103 | medium
Configuration loader: the search for sushitrack.json stops after K_MAX_SEARCH_DEPTH parents of the current directory | source/configuration/config_loader.cpp:105-125; a file placed beyond the depth is not found | medium
KalmanFilterRegistry: a second register_creator for a type replaces the first | create() returns what the later creator returns | small, needs a registry the test builds (see RegisteringATypeTwice...)
reset_log_throttles: restarts every LogThrottle, not only the one that asks | admit() after reset admits the first event of two throttles | small, needs the generation passed in (see ZeroEveryAdmitsEveryEvent)
ProfilerKeys: every group and unit name is set | non-empty check of K_GROUP_* and K_UNIT_* | small
SUSHITRACK_COUNTER: a Set counter holds its last sample | recorded counter_value after two Set samples | medium
rect (iou): disjoint boxes give zero | specified value from include/SushiTrack/detection/rect.hpp | small, no rect-level iou test in this chunk; tests/association/test_cost_calculator.cpp calls iou only indirectly
tracklet: get_kinematic_state reads Cruising when no IMM predictor is present | specified value | small
tracklet: get_last_seen_state returns the pre-loss regime while the track is coasting | specified value | small
tracklet: get_maneuver_probability is zero when no IMM predictor is present | specified value | small
tracklet_appearance: a full feature history evicts its oldest feature | exact contents of the feature history | small
tracker: OCM re-ranking changes the primary association when enabled | metamorphic, OCM on and off assign differently on a crossing scene | small
tracklet: ORU recovery replays the virtual trajectory from the last checkpoint | metamorphic, ORU on and off give different recovered means after a gap | small
: none. The rules of check_test_additions are a leftover and are not in scope; the settings promise is tested by tools/tests/test_test_budget_settings.py, and its skill-side half cannot run in this repository. Headers and modules read for this section: tools/tests/test_check_test_additions.py; tools/tests/test_test_budget_settings.py; tools/settings.py.
```
