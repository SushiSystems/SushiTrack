# Data layout

Each benchmark keeps its sequences in its own `data/` folder, and the engine reads one
benchmark at a time:

```
applications/benchmarks/
├── mot17/data/
│   ├── MOT17-02-DPM/
│   │   ├── det/det.txt               # MOT-format detections
│   │   ├── gt/gt.txt                 # MOT-format ground truth
│   │   └── seqinfo.ini               # at minimum: seqLength, frameRate
│   └── MOT17-02-FRCNN/  …
├── mot20/data/
│   └── MOT20-01/  MOT20-01-YOLOX/  …
└── dancetrack/data/
    └── dancetrack0001/  …
```

MOT CSV format:

```
<frame>, <id>, <x>, <y>, <w>, <h>, <score>, <class>, …
```

`seqinfo.ini` is parsed for `frameRate` (used for `dt = 1/frameRate` when `time_aware=1`). If absent, 30 fps is assumed.

Prediction files are written to `<sequence>/pred/<tracker>/<sequence>.txt` by each `st eval` run. `st clean --predictions` removes every `pred/` folder.
