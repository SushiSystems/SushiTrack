/**************************************************************************/
/* tracker.hpp                                                            */
/**************************************************************************/
/*                          This file is part of:                         */
/*                               SushiTrack                               */
/*               https://github.com/SushiSystems/SushiTrack               */
/*                         https://sushisystems.io                        */
/**************************************************************************/
/* Copyright (c) 2026-present Mustafa Garip & Sushi Systems               */
/*                                                                        */
/* Licensed under the PolyForm Noncommercial License 1.0.0 (the           */
/* "License"); you may not use this file except in compliance with the    */
/* License. You may obtain a copy of the License at                       */
/*                                                                        */
/*     https://polyformproject.org/licenses/noncommercial/1.0.0           */
/*                                                                        */
/* Noncommercial use is free. Commercial use requires a separate licence  */
/* from Sushi Systems; see COMMERCIAL.md. The software is provided        */
/* "as is", without warranty of any kind.                                 */
/*                                                                        */
/* Holds code ported from ByteTrack-cpp (MIT), see NOTICE.md              */
/* Copyright (c) 2022 Vertical Beach (lp6m, medalotte)                    */
/* Copyright (c) 2021 Yifu Zhang                                          */
/**************************************************************************/

/**
 * @file tracker.hpp
 * @brief Multi-object tracking engine managing tracklets and state estimation.
 * @author Mustafa Garip
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include <SushiTrack/association/assignment_solver.hpp>
#include <SushiTrack/association/cost_calculator.hpp>
#include <SushiTrack/association/cost_matrix.hpp>
#include <SushiTrack/c_api/sushitrack_export.hpp>
#include <SushiTrack/detection/object.hpp>
#include <SushiTrack/filtering/kalman_factory.hpp>
#include <SushiTrack/tracking/timing_policy.hpp>
#include <SushiTrack/tracking/tracker_config.hpp>
#include <SushiTrack/tracking/tracklet.hpp>

namespace SushiTrack
{
    /**
     * @brief Manages object tracking logic and component coordination.
     *
     * Integrates motion models, similarity metrics, and assignment solvers to
     * maintain object identities across frames.
     */
    class SUSHITRACK_API Tracker final
    {
        public:
            using TrackletPtr = std::shared_ptr<Tracklet>;

            /**
             * @brief Initializes the tracker with required logic components.
             *
             * Configures pipeline stages, timing policies, and association metrics.
             */
            Tracker(const TrackerConfig& config, std::shared_ptr<KalmanFactory> kf_factory,
                    std::unique_ptr<AssignmentSolver> solver,
                    std::shared_ptr<CostCalculator> high_conf_metric,
                    std::shared_ptr<CostCalculator> low_conf_metric,
                    std::shared_ptr<CostCalculator> tentative_metric,
                    std::shared_ptr<CostCalculator> duplicate_metric,
                    std::shared_ptr<CostCalculator> lost_recovery_metric = nullptr);

            /**
             * @brief Destroys the Tracker instance.
             */
            ~Tracker() = default;

            /**
             * @brief Updates track states using current frame detections.
             *
             * @param objects Collection of detected objects for the current frame.
             * @param dt Time interval elapsed since the last update cycle.
             * @return Collection of tracklets across all lifecycle states.
             */
            [[nodiscard]] std::vector<TrackletPtr> update(const ObjectVector& objects,
                                                          float dt = 1.0f);

            /**
             * @brief Retrieves all tracklets managed by the tracker.
             *
             * @return Collection of managed tracklets (Tracked, Lost, Tentative).
             */
            [[nodiscard]] std::vector<TrackletPtr> get_tracks() const;

            /**
             * @brief Retrieves the current global frame identifier.
             *
             * @return Current frame sequence index.
             */
            [[nodiscard]] size_t get_global_frame_id() const { return frame_id_; }

        private:
            TrackerConfig config_;                        ///< Configuration parameters.
            std::shared_ptr<TimingPolicy> timing_policy_; ///< Timing policy implementation.
            std::shared_ptr<KalmanFactory> kf_factory_;   ///< State predictor factory.
            std::unique_ptr<AssignmentSolver> solver_;    ///< Assignment solver implementation.

            std::shared_ptr<CostCalculator>
                high_conf_metric_; ///< Association cost calculator (Stage 1).
            std::shared_ptr<CostCalculator>
                low_conf_metric_; ///< Association cost calculator (Stage 2).
            std::shared_ptr<CostCalculator>
                tentative_metric_; ///< Association cost calculator (Tentative).
            std::shared_ptr<CostCalculator>
                duplicate_metric_; ///< Association cost calculator (Duplicates).
            std::shared_ptr<CostCalculator>
                lost_recovery_metric_; ///< Association cost calculator (Recovery).

            std::vector<TrackletPtr> tentative_tracklets_; ///< Tracks awaiting confirmation.
            std::vector<TrackletPtr> tracked_tracklets_;   ///< Confirmed active tracks.
            std::vector<TrackletPtr> lost_tracklets_;      ///< Tracks recently lost.

            size_t frame_id_;                   ///< Identifier for frame sequence.
            size_t track_id_count_;             ///< Counter for track identity.
            mutable uint64_t current_visit_id_; ///< Identifier for visit marking in association.

            mutable CostMatrix main_cost_matrix_; ///< Matrix for association costs.
            mutable CostMatrix iou_cost_matrix_;  ///< Matrix for overlap costs.

            /**
             * @brief Propagates all tracklets to the current timestamp.
             *
             * @param tracklet_pool Output collection of predicted active/lost tracklets.
             * @param tentative_tracks Output collection of predicted tentative tracklets.
             * @param dt Time interval for state propagation.
             */
            void predict_tracks(std::vector<TrackletPtr>& tracklet_pool,
                                std::vector<TrackletPtr>& tentative_tracks, float dt);

            /**
             * @brief Performs association with high-confidence detections.
             *
             * Updates active tracks and recovers lost tracks with high confidence detections.
             */
            void match_high_confidence(std::vector<TrackletPtr>& tracklet_pool,
                                       std::vector<TrackletPtr>& det_high,
                                       std::vector<TrackletPtr>& activated_tracklets,
                                       std::vector<TrackletPtr>& refind_tracklets,
                                       std::vector<TrackletPtr>& track_remaining,
                                       std::vector<TrackletPtr>& det_high_remaining);

            /**
             * @brief Performs association with low-confidence detections.
             *
             * Associates unmatched tracks with low-confidence detections and marks lost tracks.
             */
            void match_low_confidence(std::vector<TrackletPtr>& track_remaining,
                                      std::vector<TrackletPtr>& det_low,
                                      std::vector<TrackletPtr>& activated_tracklets,
                                      std::vector<TrackletPtr>& lost_tracklets_curr,
                                      std::vector<TrackletPtr>& det_low_remaining, float dt);

            /**
             * @brief Recovers lost tracks using visual similarity.
             *
             * @param lost_tracks Collection of tracklets in Lost state.
             * @param det_remaining Collection of unmatched detections.
             * @param activated_tracklets Output list for successfully recovered tracklets.
             * @param det_final_remaining Output list for remaining detections.
             */
            void match_lost_recovery(std::vector<TrackletPtr>& lost_tracks,
                                     std::vector<TrackletPtr>& det_remaining,
                                     std::vector<TrackletPtr>& activated_tracklets,
                                     std::vector<TrackletPtr>& det_final_remaining);

            /**
             * @brief Processes association for unconfirmed tentative tracks.
             *
             * Evaluates confirmation streaks and handles track promotion or removal.
             */
            void match_tentative(std::vector<TrackletPtr>& tentative_tracks,
                                 std::vector<TrackletPtr>& det_high_remaining,
                                 std::vector<TrackletPtr>& det_low_remaining,
                                 std::vector<TrackletPtr>& activated_tracklets,
                                 std::vector<TrackletPtr>& tentative_next,
                                 std::vector<TrackletPtr>& removed_tentative_curr,
                                 std::vector<TrackletPtr>& det_for_new_init);

            /**
             * @brief Initializes new tentative tracks from detections.
             *
             * @param det_high_remaining Qualifying detections for track initiation.
             * @param tentative_curr Output collection for the newly created tracklets.
             */
            void initialize_tentative(const std::vector<TrackletPtr>& det_high_remaining,
                                      std::vector<TrackletPtr>& tentative_curr);

            /**
             * @brief Suppresses new detections overlapping confirmed tracks.
             * @param det_for_new_init Detections eligible for initiation.
             * @param confirmed Confirmed tracked tracklets to test overlap against.
             */
            void suppress_overlapping_inits(std::vector<TrackletPtr>& det_for_new_init,
                                            const std::vector<TrackletPtr>& confirmed) const;

            /**
             * @brief Handles unconfirmed tracks in non-tentative mode.
             *
             * @param non_active_tracklets Tracks awaiting confirmation.
             * @param det_high_remaining Unclaimed high-confidence detections.
             * @param activated_tracklets Output list for successfully updated tracks.
             * @param removed_tracklets_curr Output list for tracks qualifying for removal.
             */
            void match_unconfirmed(std::vector<TrackletPtr>& non_active_tracklets,
                                   std::vector<TrackletPtr>& det_high_remaining,
                                   std::vector<TrackletPtr>& activated_tracklets,
                                   std::vector<TrackletPtr>& removed_tracklets_curr);

            /**
             * @brief Identifies and removes aged-out lost tracks.
             *
             * @param removed_tracklets_curr Output collection for removed tracklets.
             */
            void cleanup_lost(std::vector<TrackletPtr>& removed_tracklets_curr);

            /**
             * @brief Finalizes internal tracking pools.
             *
             * @param activated_tracklets Updated tracked tracklets.
             * @param refind_tracklets Recovered lost tracklets.
             * @param lost_tracklets_curr Newly lost tracklets.
             * @param removed_tracklets_curr Tracklets qualifying for removal.
             */
            void finalize_lists(const std::vector<TrackletPtr>& activated_tracklets,
                                const std::vector<TrackletPtr>& refind_tracklets,
                                const std::vector<TrackletPtr>& lost_tracklets_curr,
                                const std::vector<TrackletPtr>& removed_tracklets_curr);

            /**
             * @brief Merges two tracklet collections.
             *
             * @param a First tracklet collection.
             * @param b Second tracklet collection.
             * @return Merged unique collection.
             */
            [[nodiscard]] std::vector<TrackletPtr>
            joint_tracklets(const std::vector<TrackletPtr>& __restrict a,
                            const std::vector<TrackletPtr>& __restrict b) const;

            /**
             * @brief Subtracts one tracklet collection from another.
             *
             * @param a Source collection.
             * @param b Collection to subtract.
             * @return Collection of elements in A not present in B.
             */
            [[nodiscard]] std::vector<TrackletPtr>
            sub_tracklets(const std::vector<TrackletPtr>& __restrict a,
                          const std::vector<TrackletPtr>& __restrict b) const;

            /**
             * @brief Removes spatially redundant tracklets.
             *
             * @param a First tracklet set.
             * @param b Second tracklet set.
             * @param a_res Output of first set after cleanup.
             * @param b_res Output of second set after cleanup.
             */
            void remove_duplicate_tracklets(const std::vector<TrackletPtr>& __restrict a,
                                            const std::vector<TrackletPtr>& __restrict b,
                                            std::vector<TrackletPtr>& __restrict a_res,
                                            std::vector<TrackletPtr>& __restrict b_res) const;

            /**
             * @brief Fills a cost matrix for one stage, recording its zone and cell count.
             *
             * @param metric Cost calculator of the stage.
             * @param tracks Rows of the matrix.
             * @param detections Columns of the matrix.
             * @param out Matrix filled in place.
             */
            void calculate_costs(CostCalculator& metric, const std::vector<TrackletPtr>& tracks,
                                 const std::vector<TrackletPtr>& detections,
                                 CostMatrix& out) const;

            /**
             * @brief Creates a state predictor instance.
             *
             * @return Unique pointer to the state predictor.
             */
            [[nodiscard]] std::unique_ptr<StatePredictor> create_kalman_filter() const;
    };

} // namespace SushiTrack
