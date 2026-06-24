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

        /** @brief Defines the type of IoU metric used for spatial penalization (0: IoU, 1: DIoU, 2: GIoU, 3: SIoU). */
        int   iou_type = 1;

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

        /**
         * @brief Maximum time a tentative track may go without a match before it is removed,
         *        expressed in seconds.
         *
         * Interpreted identically in both timing modes:
         *   - time_aware = true:  compared directly against the accumulated lost-time of the
         *     tentative track, which predict() increments by the real dt on each frame.
         *   - time_aware = false: multiplied by frame_rate to obtain the equivalent frame
         *     count, then compared against frames elapsed since the last successful match.
         *
         * Default 0.08 s equals 2 frames at 25 fps, preserving the prior integer default.
         */
        float tentative_max_age_seconds = 0.08f;

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

        /**
         * @brief Score threshold for single-hit tentative confirmation.
         *
         * When the matched detection's score is at or above this value,
         * tentative_fast_hits (default 1) is used as the required hit count.
         * A single high-confidence match is sufficient to confirm the track.
         */
        float tentative_fast_thresh       = 0.85f;

        /** @brief Required hit count when the matched score exceeds tentative_fast_thresh. */
        int   tentative_fast_hits         = 1;

        /**
         * @brief Score threshold below which an extra confirmation hit is required.
         *
         * When the matched score is below this value but above the floor
         * (score < tentative_slow_thresh), tentative_slow_hits_offset additional
         * hits are added to tentative_confirm_hits. Below the floor,
         * tentative_vslow_hits_offset is added instead.
         */
        float tentative_slow_thresh       = 0.40f;

        /** @brief Extra hits added to tentative_confirm_hits for scores in [tentative_slow_thresh, high_thresh). */
        int   tentative_slow_hits_offset  = 1;

        /** @brief Extra hits added to tentative_confirm_hits for scores below tentative_slow_thresh. */
        int   tentative_vslow_hits_offset = 2;

        /**
         * @note Score-history independence of the confirmation threshold.
         *
         * The required hit count is evaluated against the score of the most
         * recently matched detection (t->getScore()), not the score at birth.
         * This means the threshold that applies at confirm time is determined
         * by the last observation, regardless of what earlier matches scored.
         *
         * Consequence: a track that accumulated hits against low-confidence
         * detections and then matches a high-confidence detection on a later
         * frame will be confirmed immediately if hits_ >= tentative_fast_hits,
         * because the new score drives the required-hits lookup. This is
         * intentional: the final high-confidence observation is treated as
         * the authoritative signal that the track is real. If the desired
         * behaviour is to gate confirmation on the score at birth, the
         * threshold lookup should be moved to activateTentative() and the
         * result stored as a per-track field.
         */
        /** @brief Scaling factor for high-confidence direct visual matches. */
        float reid_direct_assignment_weight = 0.2f;
    };
    
} // namespace SushiTrack