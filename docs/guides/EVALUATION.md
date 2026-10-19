# Evaluation pipeline

`st eval` measures SushiTrack against ByteTrack and OC-SORT on three benchmarks, each a folder of
`applications/benchmarks/` with its sequences in `data/`: `mot17`, `mot20` and `dancetrack`. One
engine, `applications/benchmarks/mot_engine/`, runs and scores every benchmark, in two steps;
`--benchmark` picks one, and without it each benchmark is run and scored on its own.

## The engine program

`mot_engine` runs each tracker over each sequence of the benchmark `--data` names. For each
pair it:

1. Reads `det/det.txt` and the frame rate of `seqinfo.ini` (`mot_sequence.hpp`).
2. Drives the tracker through a `TrackerAdapter` of `applications/tracker_bridge/`:
   `SushiTrackAdapter` through SushiTrack's C API, `ByteTrackAdapter` and `OCSORTAdapter` through
   the reference libraries.
3. Writes the tracks to `<sequence>/pred/<tracker>/<sequence>.txt` (`mot_runner.hpp`).
4. Prints `[ PERF <tracker> ] <sequence>: <us> us/frame`, which `st eval` reads back
   (`cli/sushitrack_cli/services/label.py`).

SushiTrack loads `assets/sushitrack.json` through `SUSHITRACK_CONFIG`; ByteTrack and OC-SORT read
`applications/tracker_bridge/bytetrack.json` and `applications/tracker_bridge/ocsort.json`. The
program decides nothing and is not a test; its own tests sit in
`tests/applications/benchmarks/mot_engine/`.

## The evaluator

`applications/benchmarks/mot_engine/evaluator/` scores the predictions:

- `trackeval_runner.py`: wraps the TrackEval Python module. Builds per-benchmark sequence maps, configures `MotChallenge2DBox` dataset, runs `HOTA`, `CLEAR`, `Identity` metrics.
- `report_generator.py`: `print_trackeval_table(results, compare)` formats the per-tracker, per-sequence table; `export_trackeval_results(results, dir)` writes JSON/CSV.
- `run_eval.py`: a standalone command line for `TrackEvalRunner`, independent of the benchmark program.

`st eval` runs the program, then the evaluator. `--eval-only` skips the program and re-scores the
predictions already on disk; `--trackeval` runs the upstream
`third_party/TrackEval/scripts/run_mot_challenge.py` instead of the evaluator.
