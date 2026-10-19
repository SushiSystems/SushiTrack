# mot20

The MOT20 train set: sequences 01, 02, 03 and 05, each with the public detections and with the
YOLOX private ones, 8 folders in `data/`. `st eval --private` keeps the YOLOX folders only and
`--public` leaves them out.

```bash
st eval --benchmark mot20 --compare
```

Git does not track `data/`. The sequences come from https://motchallenge.net/data/MOT20/ and
keep their own licence.
