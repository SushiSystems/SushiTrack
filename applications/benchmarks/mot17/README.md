# mot17

The MOT17 train set: sequences 02, 04, 05, 09, 10, 11 and 13, each under the DPM, FRCNN and SDP
public detections and the YOLOX private ones, 28 folders in `data/`. `st eval --private` keeps the
YOLOX folders only and `--public` leaves them out.

```bash
st eval --benchmark mot17 --compare
```

The annotations come from https://motchallenge.net and keep their own licence; `NOTICE.md` names
it.
