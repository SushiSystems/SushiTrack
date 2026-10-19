# benchmarks

Scenario benchmarks: programs that measure and decide nothing. None of them is a test or is
registered with CTest.

| Program | Measures | Run |
| --- | --- | --- |
| `mot_evaluation/` | SushiTrack, ByteTrack and OC-SORT over the MOT17, MOT20 and DanceTrack sequences in `mot_evaluation/data/`: microseconds per frame, and the HOTA, CLEAR and identity metrics its evaluator computes | `st eval --scenario MOT17-02-DPM --compare` |

`mot_evaluation` takes `--data DIR --settings DIR [--tracker NAME]... [--sequence NAME]...`,
writes each tracker's predictions to `<sequence>/pred/<tracker>/<sequence>.txt` and prints one
`[ PERF <tracker> ] <sequence>: <us> us/frame` line per pair. It exits 0 when every run ran, 1
when an input is missing and 2 for a bad command line. `st eval` runs it and then
`mot_evaluation/evaluator/`, which scores the predictions; the
[Evaluation pipeline](../../docs/guides/EVALUATION.md) describes both.
