/****************************************************************/
/* tracklet.hpp                                                 */
/* SushiTrack - https://github.com/SushiSystems/SushiTrack      */
/* Copyright (c) 2026 Sushi Systems                             */
/* Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.    */
/* Commercial use requires a licence from Sushi Systems.        */
/* Holds code ported from ByteTrack-cpp (MIT), see NOTICE.md    */
/* Copyright (c) 2022 Vertical Beach (lp6m, medalotte)          */
/* Copyright (c) 2021 Yifu Zhang                                */
/****************************************************************/

/**
 * @file tracklet.hpp
 * @brief Management of individual track states, properties, and feature history.
 * @author Mustafa Garip
 */

#pragma once

#include <SushiTrack/i_state_predictor.hpp>
#include <SushiTrack/i_timing_policy.hpp>
#include <SushiTrack/rect.hpp>
#include <SushiTrack/kalman_types.hpp>
#include <SushiTrack/kinematic_state.hpp>

#include <cstdint>
#include <cstddef>
#include <deque>
#include <memory>
#include <vector>
#include <mutex>

namespace SushiTrack
{
    /**
     * @brief Internal states for track lifecycle management.
     */
    enum class TrackState
    {
        New       = 0,  ///< Initial state for detected entities.
        Tracked   = 1,  ///< Active state for verified tracks.
        Lost      = 2,  ///< State for tracks missing observations within persistence limits.
        Removed   = 3,  ///< Terminal state for track termination.
        Tentative = 4   ///< Intermediate state for tracks awaiting confirmation.
    };

    /**
     * @brief Maintains the state, motion estimation, and visual features of a single object.
     * 
     * Manages the transition between lifecycle states and coordinates with motion 
     * predictors to update object trajectories. Stores bounding box coordinates, 
     * confidence scores, and visual feature history.
     */
    class Tracklet
    {
        public:
            EIGEN_MAKE_ALIGNED_OPERATOR_NEW

            /**
             * @brief Initializes a tracklet instance with initial detection data.
             *
             * Sets state predictor, geometry, visual features, and tracking options.
             */
            Tracklet(std::unique_ptr<IStatePredictor> kf,
                   const Rect<float>& rect, int label, float score,
                   std::vector<float> feat = {}, float alpha = 0.9f,
                   bool enable_xyah = false, int max_feature_history = 100,
                   int ocm_delta_t = 3, bool enable_oru = false);

            /**
             * @brief Destroys the Tracklet instance.
             */
            ~Tracklet() = default;

            const Rect<float>& getRect()       const { return rect_; }
            TrackState         getTrackState() const { return state_; }
            bool               isActivated()   const { return is_activated_; }
            bool               isConfirmed()   const { return is_confirmed_; }

            float              getScore()      const { return score_; }
            float              getSmoothScore() const { return score_smooth_; }
            int                getLabel()      const { return label_; }
            size_t             getTrackId()    const { return track_id_; }
            size_t             getFrameId()    const { return frame_id_; }
            size_t             getStartFrameId() const { return start_frame_id_; }
            size_t             getTrackletLength() const { return tracklet_len_; }
            int                getHits() const { return hits_; }

            /**
             * @brief Returns the length of the current uninterrupted match streak.
             *
             * Reset whenever the track survives a frame without an observation.
             */
            int                getConsecutiveHits() const { return consecutive_hits_; }

            /**
             * @brief True when an IMM predictor is driving the motion regime.
             *
             * An IMM reports a non-negative manoeuvre probability; single-model
             * predictors (KalmanFilter/NSA) return the negative sentinel, so the
             * regime getters fall back to the Cruising default.
             */
            bool immActive() const { return kf_ && kf_->maneuverProbability() >= 0.0f; }

            /**
             * @brief Continuous [0, 1] manoeuvre likelihood for the track.
             *
             * Sourced from the IMM mode probabilities (mass on the CA + CT
             * models); 0 when no IMM predictor is present. This is the
             * continuous regime signal consumed by OCM, lost-buffer scaling and
             * association cost shaping.
             */
            float getManeuverProbability() const
            {
                return immActive() ? maneuver_prob_ : 0.0f;
            }

            KinematicState     getKinematicState() const
            {
                return immActive() ? imm_state_ : KinematicState::Cruising;
            }

            /**
             * @brief Retrieves the last observed motion regime.
             *
             * Resolves to the pre-loss state if coasting, or the active state.
             */
            KinematicState     getLastSeenState() const
            {
                if (immActive())
                    return imm_state_ == KinematicState::Coasting ? imm_pre_coast_ : imm_state_;
                return KinematicState::Cruising;
            }

            /**
             * @brief Returns the last raw matched detection box.
             *
             * Unmodified box of the most recently associated observation.
             */
            const Rect<float>& getLastObservation() const { return last_observation_; }
            bool  hasLastObservation() const { return has_last_observation_; }

            /**
             * @brief Components of the unit vector of observed travel direction.
             *
             * Computed across observations over the configured observation window.
             */
            float getObsVelocityX() const { return obs_vx_; }
            float getObsVelocityY() const { return obs_vy_; }

            /** @brief Observed speed magnitude in pixels per observed frame; zero for a near-stationary track. */
            float getObsSpeed() const { return obs_speed_; }
            bool  hasObsVelocity() const { return obs_velocity_valid_; }

            /** @brief Frames elapsed since the last associated observation; zero on a freshly matched frame, incremented by each unobserved predict(). */
            int   getFramesSinceObservation() const { return frames_since_observation_; }

            const std::vector<float>& getSmoothFeat() const { return smooth_feat_; }
            const std::vector<float>& getCurrFeat()   const { return curr_feat_;   }
            bool hasFeature() const { return !smooth_feat_.empty(); }

            /** 
             * @brief Sets the track to the active state and assigns identifiers. 
             * 
             * @param kf State predictor instance.
             * @param frame_id Current frame identifier.
             * @param track_id Unique tracking identifier.
             */
            void activate(std::unique_ptr<IStatePredictor> kf, size_t frame_id, size_t track_id);

            /** 
             * @brief Sets the track to the tentative state for validation. 
             * 
             * @param kf State predictor instance.
             * @param frame_id Current frame identifier.
             * @param initial_id Initial tracking identifier.
             */
            void activateTentative(std::unique_ptr<IStatePredictor> kf, size_t frame_id, size_t initial_id);

            /** 
             * @brief Upgrades a tentative track to the confirmed state. 
             * 
             * @param frame_id Current frame identifier.
             * @param final_id Assigned unique tracking identifier.
             */
            void confirm(size_t frame_id, size_t final_id);

            /**
             * @brief Recovers a lost tracklet using new observation data.
             * 
             * @param new_track Observed detection data wrapped in a tracklet container.
             * @param frame_id Current frame identifier.
             * @param new_track_id New tracking identifier (optional).
             */
            void reActivate(const Tracklet& new_track, size_t frame_id, int new_track_id = -1);

            /** 
             * @brief Projects the motion state to a future timestamp. 
             * 
             * @param dt Time interval for prediction.
             */
            void predict(float dt = 1.0f);

            /**
             * @brief Refines motion state and bounding box with a new observation.
             * @param new_track Matched detection data.
             * @param frame_id Current frame identifier.
             * @param update_features Flag for updating visual features.
             * @param keep_state When true, suppresses lifecycle flag transition.
             */
            void update(const Tracklet& new_track, size_t frame_id,
                        bool update_features = true, bool keep_state = false);

            /**
             * @brief Transitions the track to the lost state.
             * @param dt Time interval of the marking frame for lost-time counter.
             */
            void markAsLost(float dt = 0.0f);

            /**
             * @brief Transitions the track to the removed state.
             */
            void markAsRemoved();

            /**
             * @brief Resets the consecutive-match streak for tentative confirmation.
             */
            void resetConsecutiveHits() { consecutive_hits_ = 0; }

            /**
             * @brief Sets the internal track identifier.
             * @param id Unique tracking identifier.
             */
            void setTrackId(size_t id) { track_id_ = id; }

            /**
             * @brief Assigns a timing policy to the tracklet.
             * @param policy Shared pointer to the ITimingPolicy instance.
             */
            void setTimingPolicy(std::shared_ptr<ITimingPolicy> policy) { timing_policy_ = std::move(policy); }

            /**
             * @brief Retrieves the accumulated time spent in the lost state.
             *
             * @return Total lost time in seconds or frame count.
             */
            float getAccumulatedLostTime() const { return accumulated_lost_time_; }

            /**
             * @brief Updates visual features using exponential moving average.
             * 
             * @param feat New observation feature vector.
             */
            void updateFeatures(std::vector<float> feat);

            /** 
             * @brief Calculates maximum cosine similarity against feature history. 
             * 
             * @param detection_feat Feature vector for comparison.
             * @return Maximum similarity score.
             */
            float getMaxFeatureSimilarity(const std::vector<float>& detection_feat) const;

            const std::deque<std::vector<float>>& getFeatureHistory() const {
                return feature_history_;
            }
            
            /**
             * @brief Retrieves a thread-safe copy of the current state mean.
             * @return State mean vector.
             */
            KalmanTypes::StateMean getMean() const;

            /**
             * @brief Retrieves a thread-safe copy of the current state covariance.
             * @return State covariance matrix.
             */
            KalmanTypes::StateCov  getCovariance() const;

            /**
             * @brief Formats the current state as a measurement vector for the predictor.
             * @return Measurement vector in the configured coordinate format.
             */
            KalmanTypes::DetectBox getMeasurement() const;

            mutable uint64_t visit_id_ = 0;

        private:
            std::unique_ptr<IStatePredictor> kf_;
            KalmanTypes::StateMean       mean_;
            KalmanTypes::StateCov        covariance_;

            Rect<float> rect_;
            TrackState  state_;
            bool        is_activated_;
            bool        is_confirmed_;
            float       score_;
            float       score_smooth_;
            int         label_;
            size_t      track_id_;
            size_t      frame_id_;
            size_t      start_frame_id_;
            size_t      tracklet_len_;
            int         hits_;
            int         consecutive_hits_;

            std::shared_ptr<ITimingPolicy> timing_policy_;

            // IMM-derived motion regime carrying the KinematicState contract.
            KinematicState imm_state_      = KinematicState::Cruising;
            KinematicState imm_pre_coast_  = KinematicState::Cruising; ///< Regime before going Lost, restored on recovery.
            float          maneuver_prob_   = 0.0f;
            float          stationary_prob_ = 0.0f;

            static constexpr float kRegimeStationaryProb = 0.5f; ///< Near-constant-position mass above which the regime reads Stationary.
            static constexpr float kRegimeManeuverProb   = 0.5f; ///< CA+CT mass above which the regime reads Maneuvering.
            static constexpr float kRegimeStationarySpeedFrac = 0.02f; ///< Combined speed below this fraction of box height per step also reads Stationary; the CV mode subsumes a motionless target so the NCP mode mass alone under-triggers.

            /**
             * @brief Refreshes the IMM-derived regime from predictor probabilities.
             * @param is_lost True when the track currently has no observation.
             */
            void refreshRegime(bool is_lost);

            std::vector<float> curr_feat_;
            std::vector<float> smooth_feat_;
            
            size_t max_feature_history_;
            std::deque<std::vector<float>> feature_history_;
            float accumulated_lost_time_ = 0.0f;
            int   frames_since_observation_ = 0; ///< Tracklet-owned shared counter; consumed independently by OCM's velocity gate and the tentative frame-counted age budget, not OCM-private.

            Rect<float> last_observation_;
            bool  has_last_observation_ = false;
            int   ocm_delta_t_ = 3;
            float obs_vx_ = 0.0f;
            float obs_vy_ = 0.0f;
            float obs_speed_ = 0.0f;
            bool  obs_velocity_valid_ = false;

            struct ObsSample { size_t frame_id; float cx; float cy; };
            std::deque<ObsSample> obs_history_; ///< Recent observation centres, capped to ocm_delta_t_ + 1, used to measure the direction of travel from raw detections.

            float last_dt_ = 1.0f;

            float              alpha_;
            bool               enable_xyah_;
            bool               enable_oru_ = false;

            // Checkpoint captured at last observation for ORU virtual trajectory replay.
            KalmanTypes::StateMean last_obs_mean_;
            KalmanTypes::StateCov  last_obs_cov_;
            bool                   has_checkpoint_ = false;

            /** @brief Captures the estimator state (Tracklet mean/cov + predictor internals) at a real observation. */
            void captureCheckpoint();

            /**
             * @brief Replays predict/update steps along virtual recovery trajectory.
             * @param new_meas Recovery detection in measurement space.
             * @param gap Unobserved frames bridged.
             * @param score Recovery detection confidence.
             * @return True if recovery re-update succeeded.
             */
            bool recoveryReupdate(const KalmanTypes::DetectBox& new_meas, int gap, float score);

            /** @brief Converts a box to the predictor's measurement vector (matches getMeasurement, lock-free). */
            KalmanTypes::DetectBox toMeasurement(const Rect<float>& r) const;

            static constexpr float kScoreSmoothingMomentum = 0.7f; ///< EMA weight for the tentative confirmation score; smoothing stops a one-frame spike from collapsing the required hit count.

            void addFeatureToHistory(const std::vector<float>& feat)
            {
                if (feature_history_.size() >= max_feature_history_)
                    feature_history_.pop_front();
                feature_history_.push_back(feat);
            }

            void updateRect();
            void updateRectXyah();

            /**
             * @brief Records a raw observation and refreshes direction estimate.
             * @param obs_rect Raw detection box of the matched observation.
             * @param frame_id Frame in which the observation occurred.
             */
            void recordObservation(const Rect<float>& obs_rect, size_t frame_id);

            static void normalizeL2(std::vector<float>& v);

            mutable std::mutex state_mutex_;
    };

} // namespace SushiTrack
