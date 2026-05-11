/**
 * @file      TrackerConfig.h
 * @brief     Configuration structure for SushiTrack tracking algorithm parameters.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      28.04.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */

#pragma once

namespace SushiTrack
{
    /**
     * @brief Configuration parameters for the tracking algorithm.
     * 
     * Defines thresholds for detection classification, association limits, 
     * lifecycle persistence criteria, and motion model noise parameters.
     */
    struct TrackerConfig
    {
        // ====================================================================
        // BASIC PARAMETERS
        // ====================================================================

        /** @brief Flag to enable real-time interval processing rather than frame-based intervals. */
        bool  time_aware = false;

        /** @brief Target processing frequency (frames per second). */
        int   frame_rate = 25;

        /** @brief Maximum number of frames to persist a track in the lost state before deletion. */
        int   track_buffer = 30;

        /** @brief Score threshold for classifying detections into high and low confidence pools. */
        float track_thresh = 0.2f;

        /** @brief Minimum confidence score required to initialize a new tracklet. */
        float high_thresh = 0.5f;

        /** @brief Maximum cost limit allowed for the primary tracking association step. */
        float match_thresh = 0.7f;

        /** @brief Flag to enable the tentative, unconfirmed track lifecycle state. */
        bool  enable_tentative = false;

        /** @brief Flag to enable the Kinematic Finite State Machine for dynamic noise adjustment. */
        bool  enable_fsm = false;

        /** @brief Flag to enable ReID visual feature extraction and association. */
        bool  enable_reid = false;

        /** @brief Flag to enable Mahalanobis distance gating to filter impossible associations. */
        bool  enable_mahalanobis = false;

        // ====================================================================
        // ADVANCED PARAMETERS
        // ====================================================================

        // --- Association ---

        /** @brief Primary association mode (0: Age-cascade legacy, 1: Global LAPJV). */
        int   primary_assoc_mode = 1;

        /** @brief Flag to enable confidence-based score fusion in the primary association matrix. */
        bool  fuse_score = true;

        /** @brief Maximum cost limit allowed for the secondary (IoU-based) association step. */
        float iou_match_thresh = 0.5f;

        /** @brief Maximum cost limit allowed for associating unconfirmed tracks. */
        float unconfirmed_iou_thresh = 0.7f;

        /** @brief IoU threshold to identify and remove redundant, duplicate tracks. */
        float duplicate_iou_thresh = 0.15f;

        // --- Coordinate System ---

        /** @brief Flag to enable centroid-based (XYAH) coordinate representation instead of XYWH. */
        bool  enable_xyah = false;

        /** @brief Flag to enable Distance-IoU (DIoU) metric for spatial penalization. */
        bool  enable_diou = true;

        /** @brief Flag to add a +1 offset in bounding box dimension calculations (inclusive). */
        bool  iou_plus_one = true;

        /** @brief Defines the type of Kalman Filter utilized (0: Linear, 1: Noise Scale Adaptive). */
        int   kalman_type = 0;

        // --- ReID Params ---

        /** @brief Exponential moving average weight for smoothing visual feature updates. */
        float feature_momentum = 0.9f;

        /** @brief Distance threshold for direct visual association in the high-confidence ReID pass. */
        float reid_high_thresh = 0.3f;

        /** @brief Fallback weight applied to the IoU cost during the high-confidence ReID pass. */
        float reid_high_iou_weight = 0.3f;

        /** @brief Distance threshold for direct visual association in the low-confidence ReID pass. */
        float reid_low_thresh = 0.5f;

        /** @brief Fallback weight applied to the IoU cost during the low-confidence ReID pass. */
        float reid_low_iou_weight = 0.2f;

        /** @brief Distance threshold allowed for recovering lost tracks strictly via visual features. */
        float reid_recovery_thresh = 0.4f;

        /** @brief Maximum capacity of visual appearance vectors stored in the gallery per tracklet. */
        int   max_feature_history = 100;

        // --- FSM Params ---

        /** @brief Velocity threshold to classify an object's kinematic state as stationary. */
        float fsm_stationary_vel_thresh = 40.0f;

        /** @brief Acceleration threshold to classify an object's kinematic state as maneuvering. */
        float fsm_maneuver_accel_thresh = 250.0f;

        // --- Tentative Logic ---

        /** @brief Maximum cost limit allowed for tentative track associations. */
        float tentative_match_thresh = 0.3f;

        /** @brief Number of consecutive successful matches required to confirm a tentative track. */
        int   tentative_confirm_hits = 3;

        /** @brief Maximum frames to persist a tentative track without any successful matches. */
        int   tentative_max_age = 2;

        // --- Mahalanobis Gating ---

        /** @brief Proximity threshold for initial broad spatial filtering before statistical gating. */
        float proximity_thresh = 0.7f;

        /** @brief Chi-squared statistical distance threshold utilized for Mahalanobis gating. */
        float gating_thresh = 9.4877f;

        /** @brief Weighting factor applied to motion similarity within fused association costs. */
        float motion_cost_weight = 0.15f;

        /** @brief Maximum allowed area ratio disparity to filter impossible candidate pairs. */
        float max_area_ratio = 50.0f;

        // ====================================================================
        // EXTENDED PARAMETERS
        // ====================================================================

        /** @brief Score threshold for instant tentative track confirmation. */
        float tentative_fast_thresh       = 0.85f;
        /** @brief Hits required when score exceeds fast threshold. */
        int   tentative_fast_hits         = 1;
        /** @brief Score threshold below which confirmation is delayed. */
        float tentative_slow_thresh       = 0.40f;
        /** @brief Additional hits required for slow-moving tentative tracks. */
        int   tentative_slow_hits_offset  = 1;
        /** @brief Additional hits required for very low confidence tentative tracks. */
        int   tentative_vslow_hits_offset = 2;
        /** @brief Scaling factor for high-confidence direct visual matches. */
        float reid_direct_assignment_weight = 0.2f;

        // ====================================================================
        // INTERACTION CONTEXT (GLOBAL OCCLUSION MODELLING)
        // ====================================================================

        /**
         * @brief Master switch for the scene-level interaction context module.
         *
         * When enabled, the tracker builds a per-frame interaction graph over
         * the predicted bounding boxes of all confirmed and lost tracklets,
         * snapshots an occluder identifier on a track at the moment it
         * transitions into the Lost state, and biases lost-recovery
         * association against detections that coincide with that occluder
         * while the snapshot is still valid. Defaults to false so that the
         * legacy behaviour is preserved bit-for-bit when the feature is not
         * explicitly opted into.
         */
        bool  enable_interaction_context = false;

        /**
         * @brief Minimum predicted-box IoU required to register an interaction.
         *
         * Pairs of tracklets whose predicted bounding boxes overlap below this
         * threshold are not considered to be interacting. Increasing the
         * value reduces false-positive occluder snapshots in sparse scenes;
         * lowering it captures grazing occlusions at the cost of more noise.
         */
        float interaction_iou_thresh = 0.3f;

        /**
         * @brief Additive cost penalty applied to recovery matches that
         *        coincide with a still-valid occluder snapshot.
         *
         * The penalty is added to the lost-recovery association cost when the
         * candidate detection overlaps with the occluder that was active at
         * the moment the tracklet became Lost. Larger values more aggressively
         * suppress premature re-association during the occlusion window.
         */
        float interaction_occluder_penalty = 0.3f;

        /**
         * @brief Maximum frame-counted lifetime of an occluder snapshot.
         *
         * Used when the tracker is operating in frame-based timing mode
         * (time_aware = false). Once the number of frames elapsed since the
         * snapshot exceeds this value, the snapshot is treated as expired and
         * the occluder-aware penalty is no longer applied. Larger values keep
         * the suppression active longer, which protects against late
         * emergence of the original track at the cost of delaying legitimate
         * re-acquisitions of unrelated detections.
         */
        int   interaction_occluder_validity_frames = 60;

        /**
         * @brief Maximum wall-clock lifetime of an occluder snapshot in seconds.
         *
         * Used when the tracker is operating in real-time mode
         * (time_aware = true). The semantics mirror the frame-counted variant
         * but are evaluated against the accumulated lost-time of the
         * tracklet. Decouples the validity window from the frame rate so that
         * downstream pipelines with variable cadence behave consistently.
         */
        float interaction_occluder_validity_seconds = 2.0f;
    };
    
} // namespace SushiTrack