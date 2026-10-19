# Data layout

The MOT benchmark reads from `applications/benchmarks/mot_evaluation/data/`:

```
applications/benchmarks/mot_evaluation/data/
├── MOT17-02-DPM/
│   ├── det/det.txt                   # MOT-format detections
│   ├── gt/gt.txt                     # MOT-format ground truth
│   └── seqinfo.ini                   # at minimum: seqLength, frameRate
├── MOT17-02-FRCNN/  …
├── MOT20-01/         …
├── MOT20-01-YOLOX/   …
├── dancetrack0001/img1/00000001.jpg  …
└── …
```

MOT CSV format:

```
<frame>, <id>, <x>, <y>, <w>, <h>, <score>, <class>, …
```

`seqinfo.ini` is parsed for `frameRate` (used for `dt = 1/frameRate` when `time_aware=1`). If absent, 30 fps is assumed.

Prediction files are written to `<sequence>/pred/<tracker>/<sequence>.txt` after each regression run. `cli/misc/clean_preds.sh` wipes every `pred/` subdirectory.
