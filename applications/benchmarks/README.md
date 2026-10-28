# benchmarks

Scenario benchmarks: they measure and decide nothing. None of them is a test or is registered
with CTest. One engine runs them all; each benchmark folder holds the sequences of one dataset.

| Folder | Holds | Run |
| --- | --- | --- |
| `mot_engine` | The engine: the `mot_engine` program, which runs SushiTrack, ByteTrack and OC-SORT over a benchmark's sequences, and `evaluator/`, which scores them | `st eval` |
| `mot17` | The MOT17 train set: 7 sequences under four detectors, 28 folders | `st eval --benchmark mot17` |
| `mot20` | The MOT20 train set: 4 sequences, with and without YOLOX detections, 8 folders | `st eval --benchmark mot20` |
| `dancetrack` | 40 DanceTrack sequences | `st eval --benchmark dancetrack` |

A benchmark is a folder with a `data/` folder of sequences, each a folder that holds
`det/det.txt`, `gt/gt.txt` and `seqinfo.ini`. `st eval` finds every such folder, so a new
dataset is a new folder and nothing else. Git does not track the `data/` folders; the
[Data layout](../../docs/reference/DATA_LAYOUT.md) names where each machine gets them.

`mot_engine` takes `--data DIR --settings DIR [--tracker NAME]... [--sequence NAME]... [--profiles DIR | --no-profile]`,
writes each tracker's predictions to `<sequence>/pred/<tracker>/<sequence>.txt`, writes one
profile record per pair under `--profiles` (`profiles` under the build tree by default) and prints one
`[ PERF <tracker> ] <sequence>: <us> us/frame` line and one `[ PROFILE ]` line per pair. It exits 0 when every run ran, 1
when an input is missing and 2 for a bad command line. The
[Evaluation pipeline](../../docs/guides/EVALUATION.md) describes the engine and the evaluator.
