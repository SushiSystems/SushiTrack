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
        // --- Logic Configuration ---
        bool  enable_reid = false;  ///< Controls the use of visual appearance features in association.

        // --- Temporal Configuration ---
        bool  time_aware = false;   ///< Controls whether time intervals (dt) are treated as frame counts or real-time units.

        // --- Kinematic State Machine Parameters ---
        bool  enable_fsm = false;                      ///< Controls dynamic Kalman noise adjustment based on object kinematics.
        float fsm_stationary_vel_thresh   = 40.0f;     ///< Velocity threshold for stationary state classification.
        float fsm_maneuver_accel_thresh   = 250.0f;    ///< Acceleration threshold for maneuvering state classification.

        // --- Lifecycle Parameters ---
        bool  enable_tentative = false; ///< Enables an intermediate state for track validation.
        int   frame_rate       = 25;    ///< Target processing frequency.
        int   track_buffer     = 30;    ///< Maximum frames to maintain a track in the lost state.

        /**
         * @brief Association threshold parameters.
         *
         * The track_thresh default matches the BYTE association strategy: scores
         * above the threshold form the high-confidence pool used in the first
         * association pass, scores below form the low-confidence pool used in
         * the second pass. Setting the threshold too low collapses the second
         * pass and pollutes the primary matches with weak detections.
         */
        float track_thresh     = 0.5f;  ///< Threshold separating the high-confidence and low-confidence detection pools.
        float high_thresh      = 0.5f;  ///< Minimum score required to initialize a new tracklet.
        float match_thresh     = 0.8f;  ///< Maximum cost limit for the primary association step.

        // --- State-Specific Matching ---
        float unconfirmed_iou_thresh = 0.7f; ///< Maximum cost limit for unconfirmed track association.

        // --- Validation Gating ---
        bool  enable_mahalanobis = false;  ///< Enables the use of Mahalanobis distance for search space gating.
        float gating_thresh      = 9.4877f;  ///< Chi-squared threshold for the Mahalanobis gate.
        float max_area_ratio     = 50.0f;  ///< Area ratio limit to filter candidate pairs.
        float motion_cost_weight = 0.15f;  ///< Weight of motion similarity in fused cost calculations.

        // --- Stage-Specific Parameters ---
        float iou_match_thresh       = 0.5f;  ///< Threshold for secondary association step.
        float duplicate_iou_thresh   = 0.15f; ///< IoU threshold for redundant track identification.

        // --- Tentative Track Logic ---
        float tentative_match_thresh = 0.5f; ///< Cost limit for tentative track matching.
        int   tentative_confirm_hits = 4;    ///< Required consecutive matches for track confirmation.
        int   tentative_max_age      = 12;    ///< Maximum persistence for tentative tracks without matches.


        float tentative_fast_thresh       = 0.85f; ///< Score threshold for instant track confirmation.
        int   tentative_fast_hits         = 1;     ///< Hits required when score exceeds fast threshold.
        float tentative_slow_thresh       = 0.40f; ///< Score threshold below which confirmation is delayed.
        int   tentative_slow_hits_offset  = 1;     ///< Additional hits required for slow-moving tracks.
        int   tentative_vslow_hits_offset = 2;     ///< Additional hits required for very low confidence tracks.

        // --- Kalman Filter Weights ---
        float std_weight_position = 0.05f;    ///< Baseline uncertainty multiplier for position components.
        float std_weight_velocity = 0.00625f; ///< Baseline uncertainty multiplier for velocity components.

        // --- Kalman Sensitivity Multipliers ---
        float kf_pos_weight  = 2.0f;  ///< Multiplier for position state uncertainty.
        float kf_size_weight = 2.0f;  ///< Multiplier for dimension state uncertainty.
        float kf_vel_weight  = 10.0f; ///< Multiplier for velocity state uncertainty.

        /**
         * @brief Coordinate and distance settings.
         *
         * The xyah parameterization (center, aspect ratio, height) decouples the
         * dominant stable dimension (height) from per-frame box noise and is the
         * default to align with the SORT and ByteTrack convention. xywh is left
         * available for use cases where width is the more reliable dimension.
         */
        bool  enable_xyah    = true;  ///< Enables the centroid, aspect ratio, height coordinate parameterization.
        int   kalman_type    = 0;     ///< Type of Kalman Filter (0: Linear, 1: NSA).
        bool  iou_plus_one   = true;  ///< Enables the +1 offset in bounding box dimension calculations (inclusive vs exclusive).
        bool  enable_diou    = true;  ///< Enables Distance-IoU for spatial penalization in the primary stages.

        // --- ReID Tuning (ReIDFirst path: enable_reid=1, enable_mahalanobis=0) ---
        float feature_momentum     = 0.9f;  ///< Exponential moving average weight for visual feature updates.
        float reid_direct_assignment_weight = 0.2f; ///< Scaling factor for high-confidence direct visual matches.
        float reid_high_thresh     = 0.3f;  ///< Distance threshold for direct association in the high-confidence ReIDFirst pass.
        float reid_high_iou_weight = 0.3f;  ///< IoU fallback weight in the high-confidence ReIDFirst pass.
        float reid_low_thresh      = 0.5f;  ///< Distance threshold for direct association in the low-confidence ReIDFirst pass.
        float reid_low_iou_weight  = 0.2f;  ///< IoU fallback weight in the low-confidence ReIDFirst pass.
        float reid_recovery_thresh = 0.4f;  ///< Distance threshold for PureReID lost-track visual recovery.

        /**
         * @brief Gallery management.
         */
        int   max_feature_history = 100; ///< Maximum number of visual appearance vectors stored per tracklet.

        /**
         * @brief Primary association mode: Global LAPJV (single optimal assignment).
         *
         * Uses a single LAPJV over the union of all candidate tracks, producing
         * the globally optimal assignment. This replaces the legacy age-cascade
         * greedy strategy and matches the BYTE reference implementation.
         */
        int   primary_assoc_mode = 1;

    };
    
} // namespace SushiTrack