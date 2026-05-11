import os
import sys
import argparse
import time
import cv2
import torch
import numpy as np
import json
from pathlib import Path

# Add project root to path
_ROOT = Path(__file__).parent.parent
sys.path.append(str(_ROOT / "test" / "python"))

# Add YOLOX to path
_YOLOX_PATH = _ROOT / "3rdparty" / "YOLOX"
sys.path.append(str(_YOLOX_PATH))

try:
    from sushitrack_wrapper import SushiTrackTracker, SushiTrackParams
except ImportError:
    print("Error: sushitrack_wrapper not found. Make sure it's in test/python/")
    sys.exit(1)

try:
    from yolox.exp import get_exp
    from yolox.utils import fuse_model, get_model_info, postprocess, vis
except ImportError:
    print("Warning: YOLOX not found in 3rdparty/YOLOX. Please clone it or install yolox package.")
    # We will try to handle this gracefully in the main loop if needed

def make_parser():
    parser = argparse.ArgumentParser("SushiTrack Video Demo")
    parser.add_argument("--path", default="./assets/test_video.mp4", help="path to video or camera id")
    parser.add_argument("-n", "--name", type=str, default="yolox-s", help="model name")
    parser.add_argument("-c", "--ckpt", default=None, type=str, help="ckpt for eval")
    parser.add_argument("--device", default="gpu", type=str, help="device to run our model, can set to cpu or gpu")
    parser.add_argument("--conf", default=0.1, type=float, help="test conf")
    parser.add_argument("--nms", default=0.45, type=float, help="test nms threshold")
    parser.add_argument("--tsize", default=[640], nargs="+", type=int, help="test img size (height, width) or just one value for square")
    parser.add_argument("--fps", default=30, type=int, help="fps for video")
    parser.add_argument("--num_classes", default=80, type=int, help="number of classes in the model")
    parser.add_argument("--no_save", action="store_true", help="whether to skip saving the inference result")
    parser.add_argument("--save_name", default=None, help="save name for video")
    parser.add_argument("--legacy", action="store_true", help="use legacy normalization (BGR2RGB, 0-1 scale, Mean/Std subtraction)")
    
    # Tracker args
    parser.add_argument("--track_thresh", type=float, default=0.5, help="tracking confidence threshold")
    parser.add_argument("--track_buffer", type=int, default=30, help="the frames for keep lost tracks")
    parser.add_argument("--match_thresh", type=float, default=0.8, help="matching threshold for tracking")
    parser.add_argument("--lib_path", type=str, default=str(_ROOT / "build" / "Release" / "sushitrack.dll"), help="path to sushitrack shared lib")
    parser.add_argument("--config", type=str, default=str(_ROOT / "test" / "config.json"), help="path to sushitrack config json")
    
    return parser

class Predictor(object):
    def __init__(self, model, exp, device="cpu", fp16=False, legacy=False):
        self.model = model
        self.num_classes = exp.num_classes
        self.confthre = exp.test_conf
        self.nmsthre = exp.nmsthre
        self.test_size = exp.test_size
        self.device = device
        self.fp16 = fp16
        self.rgb_means = (0.485, 0.456, 0.406)
        self.std = (0.229, 0.224, 0.225)
        self.legacy = legacy

    def inference(self, img):
        img_info = {"id": 0}
        if isinstance(img, str):
            img_info["file_name"] = os.path.basename(img)
            img = cv2.imread(img)
        else:
            img_info["file_name"] = None

        height, width = img.shape[:2]
        img_info["height"] = height
        img_info["width"] = width
        img_info["raw_img"] = img

        ratio = min(self.test_size[0] / img.shape[0], self.test_size[1] / img.shape[1])
        img_info["ratio"] = ratio

        img, _ = self.preproc(img, self.test_size, self.rgb_means, self.std)
        img = torch.from_numpy(img).unsqueeze(0)
        img = img.float()
        
        if self.legacy:
            # BGR to RGB
            img = img[:, [2, 1, 0], :, :]
            # 0-255 to 0-1
            img /= 255.0
            # Mean/Std subtraction
            img[:, 0, :, :] = (img[:, 0, :, :] - self.rgb_means[0]) / self.std[0]
            img[:, 1, :, :] = (img[:, 1, :, :] - self.rgb_means[1]) / self.std[1]
            img[:, 2, :, :] = (img[:, 2, :, :] - self.rgb_means[2]) / self.std[2]
            
        if self.device == "gpu":
            img = img.cuda()
            if self.fp16:
                img = img.half()

        with torch.no_grad():
            outputs = self.model(img)
            outputs = postprocess(
                outputs, self.num_classes, self.confthre,
                self.nmsthre, class_agnostic=True
            )
        return outputs, img_info

    def preproc(self, img, input_size, mean, std):
        if len(img.shape) == 3:
            padded_img = np.ones((input_size[0], input_size[1], 3), dtype=np.uint8) * 114
        else:
            padded_img = np.ones(input_size, dtype=np.uint8) * 114

        r = min(input_size[0] / img.shape[0], input_size[1] / img.shape[1])
        resized_img = cv2.resize(
            img,
            (int(img.shape[1] * r), int(img.shape[0] * r)),
            interpolation=cv2.INTER_LINEAR,
        ).astype(np.uint8)
        padded_img[: int(img.shape[0] * r), : int(img.shape[1] * r)] = resized_img

        padded_img = padded_img.transpose((2, 0, 1))
        padded_img = np.ascontiguousarray(padded_img, dtype=np.float32)
        return padded_img, r

def main(args):
    # 1. Setup YOLOX
    exp = get_exp(None, args.name)
    exp.test_conf = args.conf
    exp.nmsthre = args.nms
    # Handle tsize: could be [640] or [800, 1440]
    if len(args.tsize) == 1:
        # If MOT17 model and default 640 was used, use 800x1440
        if args.tsize[0] == 640 and args.ckpt and "mot17" in args.ckpt.lower():
            print("Detected MOT17 model, using default size (800, 1440)")
            exp.test_size = (800, 1440)
        else:
            exp.test_size = (args.tsize[0], args.tsize[0])
    else:
        exp.test_size = (args.tsize[0], args.tsize[1])
    
    exp.num_classes = args.num_classes

    model = exp.get_model()
    if args.device == "gpu":
        model.cuda()
    model.eval()

    if args.ckpt:
        ckpt = torch.load(args.ckpt, map_location="cpu")
        model.load_state_dict(ckpt["model"])
        print("Loaded checkpoint from", args.ckpt)

    predictor = Predictor(model, exp, args.device, False, args.legacy)

    # 2. Video Processing Setup
    cap = cv2.VideoCapture(args.path if args.path.isdigit() else args.path)
    width = cap.get(cv2.CAP_PROP_FRAME_WIDTH)
    height = cap.get(cv2.CAP_PROP_FRAME_HEIGHT)
    fps = cap.get(cv2.CAP_PROP_FPS)

    # 3. Setup SushiTrack
    import ctypes
    lib = ctypes.CDLL(args.lib_path)
    params = SushiTrackParams()
    lib.SushiTrack_GetDefaultParams(ctypes.byref(params))
    
    # Load from JSON if exists
    if os.path.exists(args.config):
        print(f"Loading config from {args.config}")
        with open(args.config, "r") as f:
            cfg = json.load(f)
            if "sushitrack" in cfg:
                ds = cfg["sushitrack"]
                # Basic
                if "basic" in ds:
                    b = ds["basic"]
                    params.time_aware = b.get("time_aware", params.time_aware)
                    params.frame_rate = b.get("frame_rate", params.frame_rate)
                    params.track_buffer = b.get("track_buffer", params.track_buffer)
                    params.track_thresh = b.get("track_thresh", params.track_thresh)
                    params.high_thresh = b.get("high_thresh", params.high_thresh)
                    params.match_thresh = b.get("match_thresh", params.match_thresh)
                    params.enable_tentative = b.get("enable_tentative", params.enable_tentative)
                    params.enable_fsm = b.get("enable_fsm", params.enable_fsm)
                    params.enable_reid = b.get("enable_reid", params.enable_reid)
                    params.enable_mahalanobis = b.get("enable_mahalanobis", params.enable_mahalanobis)
                    params.enable_interaction_context = b.get("enable_interaction_context", params.enable_interaction_context)
                
                # Advanced
                if "advanced" in ds:
                    adv = ds["advanced"]
                    # Association
                    if "association" in adv:
                        a = adv["association"]
                        params.primary_assoc_mode = a.get("primary_assoc_mode", params.primary_assoc_mode)
                        params.fuse_score = a.get("fuse_score", params.fuse_score)
                        params.iou_match_thresh = a.get("iou_match_thresh", params.iou_match_thresh)
                        params.unconfirmed_iou_thresh = a.get("unconfirmed_iou_thresh", params.unconfirmed_iou_thresh)
                        params.duplicate_iou_thresh = a.get("duplicate_iou_thresh", params.duplicate_iou_thresh)
                    
                    # Coordinate System
                    if "coordinate_system" in adv:
                        cs = adv["coordinate_system"]
                        params.enable_xyah = cs.get("enable_xyah", params.enable_xyah)
                        params.enable_diou = cs.get("enable_diou", params.enable_diou)
                        params.iou_plus_one = cs.get("iou_plus_one", params.iou_plus_one)
                        params.kalman_type = cs.get("kalman_type", params.kalman_type)

                    # Reid
                    if "reid_params" in adv:
                        r = adv["reid_params"]
                        params.feature_momentum = r.get("feature_momentum", params.feature_momentum)
                        params.reid_high_thresh = r.get("reid_high_thresh", params.reid_high_thresh)
                        params.reid_high_iou_weight = r.get("reid_high_iou_weight", params.reid_high_iou_weight)
                        params.reid_low_thresh = r.get("reid_low_thresh", params.reid_low_thresh)
                        params.reid_low_iou_weight = r.get("reid_low_iou_weight", params.reid_low_iou_weight)
                        params.reid_recovery_thresh = r.get("reid_recovery_thresh", params.reid_recovery_thresh)
                        params.max_feature_history = r.get("max_feature_history", params.max_feature_history)
                        params.reid_direct_assignment_weight = r.get("reid_direct_assignment_weight", params.reid_direct_assignment_weight)

                    # FSM
                    if "fsm_params" in adv:
                        fsm = adv["fsm_params"]
                        params.fsm_stationary_vel_thresh = fsm.get("fsm_stationary_vel_thresh", params.fsm_stationary_vel_thresh)
                        params.fsm_maneuver_accel_thresh = fsm.get("fsm_maneuver_accel_thresh", params.fsm_maneuver_accel_thresh)

                    # Tentative
                    if "tentative_logic" in adv:
                        tl = adv["tentative_logic"]
                        params.tentative_match_thresh = tl.get("tentative_match_thresh", params.tentative_match_thresh)
                        params.tentative_confirm_hits = tl.get("tentative_confirm_hits", params.tentative_confirm_hits)
                        params.tentative_max_age = tl.get("tentative_max_age", params.tentative_max_age)
                        params.tentative_fast_thresh = tl.get("tentative_fast_thresh", params.tentative_fast_thresh)
                        params.tentative_fast_hits = tl.get("tentative_fast_hits", params.tentative_fast_hits)
                        params.tentative_slow_thresh = tl.get("tentative_slow_thresh", params.tentative_slow_thresh)
                        params.tentative_slow_hits_offset = tl.get("tentative_slow_hits_offset", params.tentative_slow_hits_offset)
                        params.tentative_vslow_hits_offset = tl.get("tentative_vslow_hits_offset", params.tentative_vslow_hits_offset)

                    # Mahalanobis
                    if "mahalanobis_gating" in adv:
                        mg = adv["mahalanobis_gating"]
                        params.proximity_thresh = mg.get("proximity_thresh", params.proximity_thresh)
                        params.gating_thresh = mg.get("gating_thresh", params.gating_thresh)
                        params.motion_cost_weight = mg.get("motion_cost_weight", params.motion_cost_weight)
                        params.max_area_ratio = mg.get("max_area_ratio", params.max_area_ratio)

                    # Interaction Context
                    if "interaction_context" in adv:
                        ic = adv["interaction_context"]
                        params.interaction_iou_thresh = ic.get("interaction_iou_thresh", params.interaction_iou_thresh)
                        params.interaction_occluder_penalty = ic.get("interaction_occluder_penalty", params.interaction_occluder_penalty)
                        params.interaction_occluder_validity_frames = ic.get("interaction_occluder_validity_frames", params.interaction_occluder_validity_frames)
                        params.interaction_occluder_validity_seconds = ic.get("interaction_occluder_validity_seconds", params.interaction_occluder_validity_seconds)

    # Command line overrides (if explicitly provided)
    # Note: argparse defaults might overwrite JSON if we are not careful.
    # To keep JSON as priority for specialized fields, we only apply command line
    # for the most common ones that were already there.
    if "--track_thresh" in sys.argv: params.track_thresh = args.track_thresh
    if "--track_buffer" in sys.argv: params.track_buffer = args.track_buffer
    if "--match_thresh" in sys.argv: params.match_thresh = args.match_thresh
    
    # Always sync frame rate with video unless forced
    params.frame_rate = int(fps) if fps > 0 else args.fps
    
    tracker = SushiTrackTracker(args.lib_path, params)
    
    save_result = not args.no_save
    if save_result:
        if args.save_name is None:
            input_name = Path(args.path).stem
            timestamp = time.strftime("%Y%m%d_%H%M%S")
            save_path = f"result_{input_name}_{timestamp}.mp4"
        else:
            save_path = args.save_name
            
        vid_writer = cv2.VideoWriter(
            save_path, cv2.VideoWriter_fourcc(*"mp4v"), fps, (int(width), int(height))
        )
        print(f"Saving results to {save_path}")

    while True:
        ret_val, frame = cap.read()
        if not ret_val:
            break

        outputs, img_info = predictor.inference(frame)
        
        if outputs[0] is not None:
            output = outputs[0].cpu().numpy()
            if len(output) > 0:
                print(f"Detected {len(output)} objects before filtering")
            
            # YOLOX output: [x1, y1, x2, y2, obj_conf, class_conf, class_id]
            # Convert to [x, y, w, h, score, label]
            bboxes = output[:, 0:4]
            # Scale bboxes
            bboxes /= img_info["ratio"]
            
            scores = output[:, 4] * output[:, 5]
            labels = output[:, 6]
            
            # Filter for person class (ID 0 in COCO/MOT)
            mask = labels == 0
            bboxes = bboxes[mask]
            scores = scores[mask]
            labels = labels[mask]

            # Format for tracker
            detections = np.zeros((len(bboxes), 6))
            if len(bboxes) > 0:
                detections[:, 0:2] = bboxes[:, 0:2] # x1, y1
                detections[:, 2] = bboxes[:, 2] - bboxes[:, 0] # w
                detections[:, 3] = bboxes[:, 3] - bboxes[:, 1] # h
                detections[:, 4] = scores
                detections[:, 5] = labels
            
            actual_fps = fps if fps > 0 else args.fps
            online_targets = tracker.update(detections, 1.0 / actual_fps)
            
            # 4. Visualization (Ultralytics Style)
            for t in online_targets:
                if t["state"] == 1: # Tracked
                    tlwh = t["bbox"]
                    tid = t["track_id"]
                    
                    # Generate more vibrant color based on ID
                    palette = [
                        (255, 56, 56), (255, 157, 151), (255, 112, 166), (255, 155, 250), (255, 118, 255),
                        (151, 157, 255), (127, 255, 255), (167, 255, 151), (185, 219, 121), (255, 225, 120),
                        (255, 178, 120), (255, 150, 120), (240, 134, 80), (185, 124, 111), (255, 112, 112)
                    ]
                    color = palette[tid % len(palette)]
                    # Convert RGB to BGR for OpenCV
                    color = (color[2], color[1], color[0])
                    
                    x1, y1, w, h = map(int, tlwh)
                    x2, y2 = x1 + w, y1 + h
                    
                    # Draw Bounding Box
                    cv2.rectangle(frame, (x1, y1), (x2, y2), color, thickness=2, lineType=cv2.LINE_AA)
                    
                    # Label text
                    label = f"#{tid}"
                    font = cv2.FONT_HERSHEY_SIMPLEX
                    font_scale = 0.5
                    font_thickness = 1
                    (tw, th), baseline = cv2.getTextSize(label, font, font_scale, font_thickness)
                    
                    # Draw Label Background
                    cv2.rectangle(frame, (x1, y1 - th - baseline - 2), (x1 + tw + 2, y1), color, -1)
                    # Draw Text
                    cv2.putText(frame, label, (x1 + 1, y1 - baseline - 1), font, font_scale, (255, 255, 255), font_thickness, cv2.LINE_AA)

        cv2.imshow("SushiTrack Demo", frame)
        if save_result:
            vid_writer.write(frame)
            
        ch = cv2.waitKey(1)
        if ch == 27 or ch == ord("q") or ch == ord("Q") or cv2.getWindowProperty("SushiTrack Demo", cv2.WND_PROP_VISIBLE) < 1:
            break

    cap.release()
    if save_result:
        vid_writer.release()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    args = make_parser().parse_args()
    main(args)
