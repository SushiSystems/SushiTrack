"""
@file      pipeline.py
@brief     Core video processing pipeline abstracting detection and tracking.
@author    Mustafa Garip (mustafagarip@sushisystems.io)
@date      12.05.2026
@version   v1.0.0
@copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
"""

import os
import sys
import time
import cv2
import numpy as np
from abc import ABC, abstractmethod
from typing import Any, Dict, List, Tuple
from loguru import logger

try:
    import torch
    from yolox.exp import get_exp
    from yolox.utils import postprocess
except ImportError:
    pass  # We will handle missing YOLOX gracefully if not instantiated

class IDetector(ABC):
    """
    @brief Abstract interface for object detection models.
    
    Provides a unified method for running inference on image frames,
    adhering to the Dependency Inversion Principle.
    """
    
    @abstractmethod
    def inference(self, frame: np.ndarray) -> Tuple[np.ndarray, float]:
        """
        @brief Executes object detection on a given frame.
        
        @param frame The input image frame as a NumPy array (BGR format).
        @return A tuple containing:
                - Bounding boxes and scores in format [x, y, w, h, score, label].
                - The scaling ratio applied to the frame for inference.
        """
        pass

class YOLOXDetector(IDetector):
    """
    @brief Concrete implementation of IDetector utilizing the YOLOX architecture.
    
    Wraps the YOLOX predictor logic, handling preprocessing, inference, and postprocessing.
    """
    
    def __init__(self, model_name: str, ckpt_path: str, conf_thresh: float, nms_thresh: float, tsize: Tuple[int, int], device: str, num_classes: int, legacy: bool = False):
        """
        @brief Initializes the YOLOX detection model.
        
        @param model_name  Name of the YOLOX model architecture (e.g., 'yolox-s').
        @param ckpt_path   Path to the pre-trained weights file.
        @param conf_thresh Confidence threshold for filtering detections.
        @param nms_thresh  Non-Maximum Suppression threshold.
        @param tsize       Input resolution dimensions (height, width).
        @param device      Target hardware execution device ('cpu' or 'gpu').
        @param num_classes Total number of predictable object classes.
        @param legacy      Boolean flag to use legacy normalization.
        """
        self.device = device
        self.fp16 = False
        self.legacy = legacy
        self.num_classes = num_classes
        self.conf_thresh = conf_thresh
        self.nms_thresh = nms_thresh
        self.test_size = tsize
        self.rgb_means = (0.485, 0.456, 0.406)
        self.std = (0.229, 0.224, 0.225)
        
        exp = get_exp(None, model_name)
        exp.test_conf = conf_thresh
        exp.nmsthre = nms_thresh
        exp.test_size = tsize
        exp.num_classes = num_classes
        
        self.model = exp.get_model()
        if self.device == "gpu":
            self.model.cuda()
        self.model.eval()

        if ckpt_path:
            ckpt = torch.load(ckpt_path, map_location="cpu")
            self.model.load_state_dict(ckpt["model"])
            logger.info(f"Loaded checkpoint from {ckpt_path}")

    def preproc(self, img: np.ndarray, input_size: Tuple[int, int]) -> Tuple[np.ndarray, float]:
        """
        @brief Preprocesses the image frame for YOLOX inference.
        
        @param img        The original input frame.
        @param input_size The target resolution (height, width).
        @return A tuple containing the padded image tensor and the scale ratio.
        """
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

    def inference(self, frame: np.ndarray) -> Tuple[np.ndarray, float]:
        """
        @brief Executes object detection on a given frame.
        
        @param frame The input image frame as a NumPy array (BGR format).
        @return A tuple containing formatted detections [x, y, w, h, score, label] and the scale ratio.
        """
        img, ratio = self.preproc(frame, self.test_size)
        img_tensor = torch.from_numpy(img).unsqueeze(0).float()
        
        if self.legacy:
            img_tensor = img_tensor[:, [2, 1, 0], :, :]
            img_tensor /= 255.0
            img_tensor[:, 0, :, :] = (img_tensor[:, 0, :, :] - self.rgb_means[0]) / self.std[0]
            img_tensor[:, 1, :, :] = (img_tensor[:, 1, :, :] - self.rgb_means[1]) / self.std[1]
            img_tensor[:, 2, :, :] = (img_tensor[:, 2, :, :] - self.rgb_means[2]) / self.std[2]
            
        if self.device == "gpu":
            img_tensor = img_tensor.cuda()
            if self.fp16:
                img_tensor = img_tensor.half()

        with torch.no_grad():
            outputs = self.model(img_tensor)
            outputs = postprocess(
                outputs, self.num_classes, self.conf_thresh,
                self.nms_thresh, class_agnostic=True
            )
            
        if outputs[0] is None:
            return np.zeros((0, 6)), ratio
            
        output = outputs[0].cpu().numpy()
        bboxes = output[:, 0:4]
        bboxes /= ratio
        scores = output[:, 4] * output[:, 5]
        labels = output[:, 6]
        
        # Filter for person class (ID 0)
        mask = labels == 0
        bboxes = bboxes[mask]
        scores = scores[mask]
        labels = labels[mask]
        
        detections = np.zeros((len(bboxes), 6))
        if len(bboxes) > 0:
            detections[:, 0:2] = bboxes[:, 0:2]
            detections[:, 2] = bboxes[:, 2] - bboxes[:, 0]
            detections[:, 3] = bboxes[:, 3] - bboxes[:, 1]
            detections[:, 4] = scores
            detections[:, 5] = labels
            
        return detections, ratio

class VideoPipeline:
    """
    @brief Orchestrates the video sequence processing and visualization.
    
    Integrates an IDetector and an ITrackerAdapter (via TrackerBridge) to 
    process video streams frame by frame, rendering the results aesthetically.
    """
    
    def __init__(self, video_path: str, detector: IDetector, stress_mode: bool = False, save_path: str = None):
        """
        @brief Initializes the VideoPipeline orchestrator.
        
        @param video_path  Path to the source video file or camera device ID.
        @param detector    Injected implementation of IDetector.
        @param stress_mode Flag to disable visual rendering for maximum throughput.
        @param save_path   Optional path to save the output video.
        """
        self.video_path = video_path
        self.stress_mode = stress_mode
        self.detector = detector
        
        # Handle string digit for camera index
        src = int(video_path) if video_path.isdigit() else video_path
        self.cap = cv2.VideoCapture(src)
        if not self.cap.isOpened():
            raise FileNotFoundError(f"Cannot open video source: {video_path}")
            
        self.width = int(self.cap.get(cv2.CAP_PROP_FRAME_WIDTH))
        self.height = int(self.cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
        self.fps = self.cap.get(cv2.CAP_PROP_FPS)
        
        self.save_path = save_path
        self.vid_writer = None
        if save_path and not stress_mode:
            self.vid_writer = cv2.VideoWriter(
                save_path, cv2.VideoWriter_fourcc(*"mp4v"), self.fps, (self.width, self.height)
            )

    def run(self, tracker: Any):
        """
        @brief Executes the primary loop for video processing.
        
        @param tracker The tracking bridge object exposing an update() method.
        """
        frame_id = 0
        total_time = 0.0
        
        logger.info(f"Starting pipeline on {self.video_path} at {self.fps} FPS")

        while True:
            ret, frame = self.cap.read()
            if not ret:
                break
                
            frame_id += 1
            start_time = time.time()
            
            # 1. Detection
            detections, ratio = self.detector.inference(frame)
            
            # 2. Tracking
            tracks = tracker.update(detections)
            
            end_time = time.time()
            total_time += (end_time - start_time)
            
            # 3. Visualization
            if not self.stress_mode:
                self._draw_frame(frame, tracks)
                cv2.imshow("SushiTrack Video Test", frame)
                if self.vid_writer:
                    self.vid_writer.write(frame)
                    
                ch = cv2.waitKey(1)
                if ch == 27 or ch == ord('q') or cv2.getWindowProperty("SushiTrack Video Test", cv2.WND_PROP_VISIBLE) < 1:
                    break
                    
        self.cap.release()
        if self.vid_writer:
            self.vid_writer.release()
        if not self.stress_mode:
            cv2.destroyAllWindows()
            
        avg_fps = frame_id / total_time if total_time > 0 else 0
        logger.success(f"Processed {frame_id} frames at {avg_fps:.2f} FPS (avg).")

    def _draw_frame(self, frame: np.ndarray, tracks: List[Dict[str, Any]]):
        """
        @brief Applies aesthetic bounding boxes and labels onto the frame.
        
        @param frame  The source frame array.
        @param tracks Collection of track dictionaries outputted by the tracker.
        """
        palette = [
            (255, 56, 56), (255, 157, 151), (255, 112, 166), (255, 155, 250), (255, 118, 255),
            (151, 157, 255), (127, 255, 255), (167, 255, 151), (185, 219, 121), (255, 225, 120),
            (255, 178, 120), (255, 150, 120), (240, 134, 80), (185, 124, 111), (255, 112, 112)
        ]

        for t in tracks:
            tid = t["track_id"]
            x, y, w, h = t["bbox"]
            
            color = palette[tid % len(palette)]
            # Convert RGB to BGR for OpenCV rendering
            color = (color[2], color[1], color[0])
            
            x1, y1 = int(x), int(y)
            x2, y2 = int(x + w), int(y + h)
            
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
