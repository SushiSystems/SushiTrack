/**************************************************************************/
/* tracker_config.hpp                                                     */
/**************************************************************************/
/*                          This file is part of:                         */
/*                               SushiTrack                               */
/*              https://github.com/SushiSystems/SushiTrack                */
/*                          https://sushisystems.io                       */
/**************************************************************************/
/* Copyright (c) 2026-present Mustafa Garip & Sushi Systems               */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License");        */
/* you may not use this file except in compliance with the License.       */
/* You may obtain a copy of the License at                                */
/*                                                                        */
/*     http://www.apache.org/licenses/LICENSE-2.0                         */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                          */
/**************************************************************************/

/**
 * @file tracker_config.hpp
 * @brief Configuration structure for SushiTrack algorithm parameters.
 * @author Mustafa Garip
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
        // BASIC PARAMETERS

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

        /** @brief Flag to enable ReID visual feature extraction and association. */
        bool  enable_reid = false;

        /** @brief Flag to enable Mahalanobis distance gating to filter impossible associations. */
        bool  enable_mahalanobis = false;

        // ADVANCED PARAMETERS

        // Association

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

        // Coordinate System

        /** @brief Flag to enable centroid-based (XYAH) coordinate representation instead of XYWH. */
        bool  enable_xyah = false;

        /** @brief Defines the type of IoU metric used for spatial penalization (0: IoU, 1: DIoU, 2: GIoU, 3: SIoU). */
        int   iou_type = 1;

        /** @brief Flag to add a +1 offset in bounding box dimension calculations (inclusive). */
        bool  iou_plus_one = true;

        /** @brief Defines the motion estimator (0: Linear Kalman, 1: Noise Scale Adaptive, 2: IMM multi-model). */
        int   kalman_type = 0;

        // ReID Params

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

        // Observation-Centric Momentum (OCM)

        /** @brief Flag to enable the observation-centric momentum re-ranking term in primary association. */
        bool  enable_ocm = false;

        /**
         * @brief Flag to enable Observation-centric Re-Update (ORU) on lost-track recovery.
         */
        bool  enable_oru = false;

        /**
         * @brief Strength of the momentum re-ranking term.
         */
        float ocm_vdc_weight = 0.1f;

        /** @brief Observation window, in observed frames, over which a track's direction of travel is measured. */
        int   ocm_delta_t = 3;

        /** @brief Minimum observed speed below which momentum is suppressed. */
        float ocm_min_velocity = 1.0f;

        /** @brief Maximum frames since the last observation for which momentum is applied. */
        int   ocm_max_age = 1;

        /**
         * @brief Fraction of ocm_vdc_weight applied to Maneuvering-regime tracks.
         */
        float ocm_maneuver_weight_scale = 0.15f;

        // Tentative Logic

        /** @brief Maximum cost limit allowed for tentative track associations. */
        float tentative_match_thresh = 0.3f;

        /** @brief Number of consecutive successful matches required to confirm a tentative track. */
        int   tentative_confirm_hits = 3;

        /**
         * @brief Wall-clock age budget for a tentative track, in seconds.
         */
        float tentative_max_age_seconds = 0.08f;

        /**
         * @brief Maximum consecutive missed frames a tentative track may survive before removal.
         */
        int   tentative_max_miss_frames = 2;

        /**
         * @brief IoU above which a new-track detection overlapping a confirmed track is suppressed.
         */
        float tentative_init_suppress_iou = 0.7f;

        // Mahalanobis Gating

        /** @brief Chi-squared statistical distance threshold utilized for Mahalanobis gating. */
        float gating_thresh = 9.4877f;

        /** @brief Weighting factor applied to motion similarity within fused association costs. */
        float motion_cost_weight = 0.15f;

        /** @brief Maximum allowed area ratio disparity to filter impossible candidate pairs. */
        float max_area_ratio = 50.0f;

        // EXTENDED PARAMETERS

        /**
         * @brief Score threshold for single-hit tentative confirmation.
         */
        float tentative_fast_thresh       = 0.85f;

        /** @brief Required hit count when the matched score exceeds tentative_fast_thresh. */
        int   tentative_fast_hits         = 1;

        /**
         * @brief Lower score-band boundary for tentative confirmation hits.
         */
        float tentative_slow_thresh       = 0.40f;

        /** @brief Extra hits added to tentative_confirm_hits for scores in [tentative_slow_thresh, high_thresh). */
        int   tentative_slow_hits_offset  = 1;

        /** @brief Extra hits added to tentative_confirm_hits for scores below tentative_slow_thresh. */
        int   tentative_vslow_hits_offset = 2;

        /** @brief Scaling factor for high-confidence direct visual matches. */
        float reid_direct_assignment_weight = 0.2f;
    };

} // namespace SushiTrack