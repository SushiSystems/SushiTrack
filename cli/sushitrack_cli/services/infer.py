"""The YOLOX video inference pipeline behind `ds infer`.

A front-end for tests/regression/inference/demo.py: this only translates flags and
launches it. The pipeline needs tracker_bridge from a `ds build` plus its own
Python dependencies (torch, opencv) — those belong to the demo, not to this CLI.
"""

import sys

from .. import console, proc
from .label import TRACKERS, resolve_tracker

# Flag name on the parsed namespace -> the switch demo.py expects.
_SWITCHES = (
    ("stress", "--stress"),
    ("no_save", "--no-save"),
    ("legacy", "--legacy"),
    ("mot_label", "--mot-label"),
    ("no_screen", "--no-screen"),
    ("reid", "--reid"),
)


def run(cfg, paths, args):
    demo = paths.inference / "demo.py"
    if not demo.exists():
        console.error(f"demo.py not found at {demo}")
        return 1
    if not args.source:
        console.error("--source <video path or camera index> is required.")
        return 2

    key = resolve_tracker(args.tracker or cfg.test_tracker)
    command = [
        sys.executable, str(demo),
        "--source", args.source,
        "--tracker", TRACKERS[key],
        "--name", args.name,
        "--device", args.device,
        "--conf", str(args.conf),
        "--nms", str(args.nms),
        "--num-classes", str(args.num_classes),
    ]
    if args.ckpt:
        command += ["--ckpt", args.ckpt]
    if args.tsize:
        command += ["--tsize", *[str(t) for t in args.tsize]]
    for flag, switch in _SWITCHES:
        if getattr(args, flag, False):
            command.append(switch)
    if args.save_name:
        command += ["--save_name", args.save_name]
    if args.reid and args.reid_model:
        command += ["--reid-model", args.reid_model]
    command.extend(args.forwarded)

    console.header("Infer")
    console.info(f"Tracker: {TRACKERS[key]}  |  source: {args.source}")
    return proc.run(command, cwd=paths.root)
