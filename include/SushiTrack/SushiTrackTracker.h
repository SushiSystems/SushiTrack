/**
 * @file      SushiTrackTracker.h
 * @brief     Core tracking algorithm orchestration and lifecycle management.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      28.04.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */

#pragma once

#include "SushiTrack/Tracklet.h"
#include "SushiTrack/ICostCalculator.h"
#include "SushiTrack/IAssignmentSolver.h"
#include "SushiTrack/IKalmanFactory.h"
#include "SushiTrack/ITimingPolicy.h"
#include "SushiTrack/TrackerConfig.h"
#include "SushiTrack/Object.h"
#include "SushiTrack/CostMatrix.h"

#include <cstdint>
#include <cstddef>
#include <memory>
#include <vector>

namespace SushiTrack
{
    /**
     * @brief Manages object tracking logic and component coordination.
     * 
     * Integrates motion models, similarity metrics, and assignment solvers to 
     * maintain object identities across frames. Processes input detections and 
     * updates internal track representations based on configured association 
     * strategies.
     */
    class SushiTrackTracker
    {
        public:
            using TrackletPtr = std::shared_ptr<Tracklet>;

            /**
             * @brief Initializes the tracker with required logic components.
             * 
             * @param config Configuration parameters for the tracking algorithm.
             * @param kf_factory Factory for generating state predictor instances.
             * @param solver Implementation of the bipartite assignment solver.
             * @param high_conf_metric Metric for high-confidence association.
             * @param low_conf_metric Metric for low-confidence association.
             * @param tentative_metric Metric for unconfirmed track association.
             * @param duplicate_metric Metric for identifying redundant tracklets.
             * @param lost_recovery_metric Optional metric for visual recovery of lost tracks.
             */
            SushiTrackTracker(const TrackerConfig& config,
                            std::shared_ptr<IKalmanFactory> kf_factory,
                            std::unique_ptr<IAssignmentSolver> solver,
                            std::shared_ptr<ICostCalculator> high_conf_metric,
                            std::shared_ptr<ICostCalculator> low_conf_metric,
                            std::shared_ptr<ICostCalculator> tentative_metric,
                            std::shared_ptr<ICostCalculator> duplicate_metric,
                            std::shared_ptr<ICostCalculator> lost_recovery_metric = nullptr);

            /**
             * @brief Destroys the SushiTrackTracker instance.
             */
            ~SushiTrackTracker() = default;

            /**
             * @brief Updates track states using current frame detections.
             *
             * @param objects Collection of detected objects for the current frame.
             * @param dt Time interval elapsed since the last update cycle.
             * @return Collection of tracklets across all lifecycle states (Tracked, Lost, Tentative, Removed).
             */
            std::vector<TrackletPtr> update(const ObjectVector& objects, float dt = 1.0f);

            /**
             * @brief Retrieves all tracklets managed by the tracker across all lifecycle states.
             * 
             * @return Collection of managed tracklets (Tracked, Lost, Tentative).
             */
            std::vector<TrackletPtr> getTracks() const;

            /**
             * @brief Retrieves the current global frame identifier.
             * 
             * @return Current frame sequence index.
             */
            size_t getGlobalFrameId() const { return frame_id_; }

        private:
            TrackerConfig                           config_;                ///< Configuration parameters.
            std::shared_ptr<ITimingPolicy>          timing_policy_;         ///< Timing policy implementation.
            std::shared_ptr<IKalmanFactory>         kf_factory_;            ///< State predictor factory.
            std::unique_ptr<IAssignmentSolver>      solver_;                ///< Assignment solver implementation.
            
            std::shared_ptr<ICostCalculator>        high_conf_metric_;      ///< Association cost calculator (Stage 1).
            std::shared_ptr<ICostCalculator>        low_conf_metric_;       ///< Association cost calculator (Stage 2).
            std::shared_ptr<ICostCalculator>        tentative_metric_;      ///< Association cost calculator (Tentative).
            std::shared_ptr<ICostCalculator>        duplicate_metric_;      ///< Association cost calculator (Duplicates).
            std::shared_ptr<ICostCalculator>        lost_recovery_metric_;  ///< Association cost calculator (Recovery).

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
            void predictTracks(std::vector<TrackletPtr>& tracklet_pool, std::vector<TrackletPtr>& tentative_tracks, float dt);
            
            /**
             * @brief Performs association with high-confidence detections.
             * 
             * @param tracklet_pool Collection of candidate tracklets.
             * @param det_high Collection of high-confidence observations.
             * @param activated_tracklets Output list for successfully updated tracks.
             * @param refind_tracklets Output list for successfully recovered lost tracks.
             * @param track_remaining Output list for unmatched tracklets.
             * @param det_high_remaining Output list for unmatched high-confidence detections.
             */
            void matchHighConfidence(std::vector<TrackletPtr>& tracklet_pool, std::vector<TrackletPtr>& det_high,
                                    std::vector<TrackletPtr>& activated_tracklets, std::vector<TrackletPtr>& refind_tracklets,
                                    std::vector<TrackletPtr>& track_remaining, std::vector<TrackletPtr>& det_high_remaining);
            
            /**
             * @brief Performs association with low-confidence detections.
             *
             * The dt parameter is forwarded to Tracklet::markAsLost so that the
             * accumulated lost-time counter starts from the marking frame, which
             * is required to keep wall-clock and frame-counted expirations aligned.
             *
             * @param track_remaining Collection of unmatched tracks.
             * @param det_low Collection of low-confidence observations.
             * @param activated_tracklets Output list for successfully updated tracks.
             * @param lost_tracklets_curr Output list for tracks newly marked as lost.
             * @param det_low_remaining Output list for unmatched low-confidence detections.
             * @param dt Time interval of the current frame.
             */
            void matchLowConfidence(std::vector<TrackletPtr>& track_remaining, std::vector<TrackletPtr>& det_low,
                                    std::vector<TrackletPtr>& activated_tracklets,
                                    std::vector<TrackletPtr>& lost_tracklets_curr,
                                    std::vector<TrackletPtr>& det_low_remaining,
                                    float dt);
                                    
            /**
             * @brief Recovers lost tracks using visual similarity.
             * 
             * @param lost_tracks Collection of tracklets in Lost state.
             * @param det_remaining Collection of unmatched detections.
             * @param activated_tracklets Output list for successfully recovered tracklets.
             * @param det_final_remaining Output list for remaining detections.
             */
            void matchLostRecovery(std::vector<TrackletPtr>& lost_tracks, std::vector<TrackletPtr>& det_remaining,
                                   std::vector<TrackletPtr>& activated_tracklets,
                                   std::vector<TrackletPtr>& det_final_remaining);
            
            /**
             * @brief Processes association for unconfirmed tentative tracks.
             * 
             * @param tentative_tracks Predicted tentative tracklets.
             * @param det_high_remaining Unclaimed high-confidence detections.
             * @param det_low_remaining Unclaimed low-confidence detections.
             * @param activated_tracklets Output list for tracks promoted to confirmed.
             * @param tentative_next Output list for tracks remaining tentative.
             * @param removed_tentative_curr Output list for tentative tracks that aged out.
             * @param det_for_new_init Output list for detections qualifying for track initiation.
             */
            void matchTentative(std::vector<TrackletPtr>& tentative_tracks,
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
            void initializeTentative(const std::vector<TrackletPtr>& det_high_remaining, std::vector<TrackletPtr>& tentative_curr);

            /**
             * @brief Drops new-track detections that overlap a confirmed track above the
             *        tentative_init_suppress_iou gate, so a fresh tentative cannot duplicate
             *        an already-tracked target and split its ID on confirmation.
             *
             * @param det_for_new_init In/out collection of detections eligible for initiation.
             * @param confirmed Confirmed tracked tracklets to test overlap against.
             */
            void suppressOverlappingInits(std::vector<TrackletPtr>& det_for_new_init,
                                          const std::vector<TrackletPtr>& confirmed) const;

            /**
             * @brief Handles unconfirmed tracks in non-tentative mode.
             * 
             * @param non_active_tracklets Tracks awaiting confirmation.
             * @param det_high_remaining Unclaimed high-confidence detections.
             * @param activated_tracklets Output list for successfully updated tracks.
             * @param removed_tracklets_curr Output list for tracks qualifying for removal.
             */
            void matchUnconfirmed(std::vector<TrackletPtr>& non_active_tracklets, std::vector<TrackletPtr>& det_high_remaining,
                                std::vector<TrackletPtr>& activated_tracklets, std::vector<TrackletPtr>& removed_tracklets_curr);
            
            /**
             * @brief Identifies and removes aged-out lost tracks.
             * 
             * @param removed_tracklets_curr Output collection for removed tracklets.
             */
            void cleanupLost(std::vector<TrackletPtr>& removed_tracklets_curr);
            
            /**
             * @brief Finalizes internal tracking pools.
             * 
             * @param activated_tracklets Updated tracked tracklets.
             * @param refind_tracklets Recovered lost tracklets.
             * @param lost_tracklets_curr Newly lost tracklets.
             * @param removed_tracklets_curr Tracklets qualifying for removal.
             */
            void finalizeLists(const std::vector<TrackletPtr>& activated_tracklets, const std::vector<TrackletPtr>& refind_tracklets,
                            const std::vector<TrackletPtr>& lost_tracklets_curr, const std::vector<TrackletPtr>& removed_tracklets_curr);

            /**
             * @brief Merges two tracklet collections.
             * 
             * @param a First tracklet collection.
             * @param b Second tracklet collection.
             * @return Merged unique collection.
             */
            std::vector<TrackletPtr> jointTracklets(const std::vector<TrackletPtr>& __restrict a, const std::vector<TrackletPtr>& __restrict b) const;

            /**
             * @brief Subtracts one tracklet collection from another.
             * 
             * @param a Source collection.
             * @param b Collection to subtract.
             * @return Collection of elements in A not present in B.
             */
            std::vector<TrackletPtr> subTracklets  (const std::vector<TrackletPtr>& __restrict a, const std::vector<TrackletPtr>& __restrict b) const;
            
            /**
             * @brief Removes spatially redundant tracklets.
             * 
             * @param a First tracklet set.
             * @param b Second tracklet set.
             * @param a_res Output of first set after cleanup.
             * @param b_res Output of second set after cleanup.
             */
            void removeDuplicateTracklets(const std::vector<TrackletPtr>& __restrict a, const std::vector<TrackletPtr>& __restrict b,
                                        std::vector<TrackletPtr>& __restrict a_res, std::vector<TrackletPtr>& __restrict b_res) const;

            /**
             * @brief Creates a state predictor instance.
             *
             * @return Unique pointer to the state predictor.
             */
            std::unique_ptr<IStatePredictor> createKalmanFilter() const;
    };

} // namespace SushiTrack
