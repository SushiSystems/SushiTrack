# Evaluation pipeline

`tests/regression/evaluator/`:

- `trackeval_runner.py`: wraps the TrackEval Python module. Builds per-benchmark sequence maps, configures `MotChallenge2DBox` dataset, runs `HOTA`, `CLEAR`, `Identity` metrics.
- `report_generator.py`: `print_trackeval_table(results, compare)` formats the per-tracker × per-sequence table; `export_trackeval_results(results, dir)` writes JSON/CSV.
- `run_eval.py`: standalone CLI for `TrackEvalRunner` (independent of the compiled binary).

`st eval` automatically chains regression_test → TrackEval; pass `--eval-only` to skip the C++ binary and re-evaluate cached predictions, or `--trackeval` to invoke the upstream `third_party/TrackEval/scripts/run_mot_challenge.py` script.
