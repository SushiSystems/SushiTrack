"""
@file      demo.py
@brief     CLI Entry point for the video tracking test pipeline.
@author    Mustafa Garip (mustafagarip@sushisystems.io)
@date      12.05.2026
@version   v1.0.0
@copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
"""

import argparse
import sys
import os
import time
from pathlib import Path
from loguru import logger

# Ensure we can import from the local directory and the project root
_THIS_DIR = Path(__file__).parent
_PROJECT_ROOT = _THIS_DIR.parent.parent.parent
_TOOLS_DIR = _PROJECT_ROOT / ".tools"

if str(_TOOLS_DIR) not in sys.path:
    sys.path.insert(0, str(_TOOLS_DIR))

try:
    from paths_manager import DIR_3RDPARTY, DIR_BUILD
    _YOLOX_DIR = DIR_3RDPARTY / "yolox"
    _BUILD_DIR = DIR_BUILD
except ImportError:
    _YOLOX_DIR = _PROJECT_ROOT / "3rdparty" / "yolox"
    _BUILD_DIR = _PROJECT_ROOT / "build"

sys.path.append(str(_THIS_DIR))
sys.path.append(str(_YOLOX_DIR))

from tracker_bridge import TrackerBridge
from pipeline import VideoPipeline, YOLOXDetector

def make_parser() -> argparse.ArgumentParser:
    """
    @brief Constructs the argument parser for the video demo.
    
    @return Configured ArgumentParser instance.
    """
    parser = argparse.ArgumentParser(description="SushiTrack Moduler Video Test Pipeline")
    
    # Generic Arguments
    parser.add_argument("--tracker", type=str, default="SushiTrack", help="Name of the tracker to use (e.g., SushiTrack, ByteTrack, OCSORT)")
    parser.add_argument("--source", type=str, required=True, help="Path to the input video or camera ID")
    parser.add_argument("--stress", action="store_true", help="Run in stress mode (no UI, maximum throughput)")
    parser.add_argument("--no_save", action="store_true", help="Do not save the output video")
    parser.add_argument("--save_name", default=None, help="Explicit filename for saving output")
    
    # YOLOX Arguments
    parser.add_argument("-n", "--name", type=str, default="yolox-s", help="YOLOX model architecture name")
    parser.add_argument("-c", "--ckpt", default=None, type=str, help="Path to YOLOX checkpoint weights")
    parser.add_argument("--device", default="gpu", type=str, help="Device to run inference (cpu or gpu)")
    parser.add_argument("--conf", default=0.1, type=float, help="Detection confidence threshold")
    parser.add_argument("--nms", default=0.45, type=float, help="NMS threshold")
    parser.add_argument("--tsize", default=[640], nargs="+", type=int, help="Test image size (height, width) or single square value")
    parser.add_argument("--num_classes", default=80, type=int, help="Number of classes in the model")
    parser.add_argument("--legacy", action="store_true", help="Use legacy normalization (BGR2RGB, 0-1 scale, Mean/Std)")
    
    return parser

def main():
    """
    @brief Main execution sequence for orchestrating the demo.
    """
    args = make_parser().parse_args()

    logger.info("--- Starting Moduler Video Pipeline ---")
    logger.info(f"Tracker: {args.tracker}")
    logger.info(f"Source: {args.source}")
    logger.info(f"Stress Mode: {args.stress}")
    logger.info(f"YOLOX Model: {args.name}")

    try:
        # Determine target image size
        if len(args.tsize) == 1:
            if args.tsize[0] == 640 and args.ckpt and "mot17" in args.ckpt.lower():
                logger.info("Detected MOT17 model, using default resolution (800, 1440)")
                tsize = (800, 1440)
            else:
                tsize = (args.tsize[0], args.tsize[0])
        else:
            tsize = (args.tsize[0], args.tsize[1])

        # 1. Initialize YOLOX Detector
        detector = YOLOXDetector(
            model_name=args.name,
            ckpt_path=args.ckpt,
            conf_thresh=args.conf,
            nms_thresh=args.nms,
            tsize=tsize,
            device=args.device,
            num_classes=args.num_classes,
            legacy=args.legacy
        )
        logger.info("[Demo] YOLOX Detector initialized successfully.")

        # 2. Determine Save Path
        save_path = None
        if not args.no_save and not args.stress:
            if args.save_name is None:
                input_name = Path(args.source).stem
                timestamp = time.strftime("%Y%m%d_%H%M%S")
                save_path = f"result_{input_name}_{timestamp}.mp4"
            else:
                save_path = args.save_name
            logger.info(f"[Demo] Output will be saved to: {save_path}")

        # 3. Initialize Tracker Bridge
        tracker = TrackerBridge(tracker_name=args.tracker, build_dir=str(_BUILD_DIR))
        logger.info(f"[Demo] {args.tracker} Tracker loaded successfully.")

        # 4. Initialize Pipeline
        pipeline = VideoPipeline(video_path=args.source, detector=detector, stress_mode=args.stress, save_path=save_path)
        logger.info("[Demo] Pipeline orchestrator initialized.")

        # Run Pipeline
        pipeline.run(tracker)
        logger.success("[Demo] Pipeline execution completed without errors.")
        
    except Exception as e:
        logger.exception(f"[Demo] Critical error occurred: {str(e)}")
        sys.exit(1)

if __name__ == "__main__":
    main()
