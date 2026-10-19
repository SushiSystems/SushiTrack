/**************************************************************************/
/* tracklet.hpp                                                           */
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
 * @file tracklet.hpp
 * @brief Management of individual track states, properties, and feature history.
 * @author Mustafa Garip
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <vector>

#include <SushiTrack/detection/rect.hpp>
#include <SushiTrack/filtering/kalman_types.hpp>
#include <SushiTrack/filtering/state_predictor.hpp>
#include <SushiTrack/tracking/kinematic_state.hpp>
#include <SushiTrack/tracking/timing_policy.hpp>
#include <SushiTrack/tracking/track_state.hpp>

namespace SushiTrack
{
    /**
     * @brief Maintains the state, motion estimation, and visual features of a single object.
     *
     * Manages the transition between lifecycle states and coordinates with motion
     * predictors to update object trajectories. Stores bounding box coordinates,
     * confidence scores, and visual feature history.
     */
    class Tracklet final
    {
        public:
            EIGEN_MAKE_ALIGNED_OPERATOR_NEW

            /**
             * @brief Initializes a tracklet instance with initial detection data.
             *
             * Sets state predictor, geometry, visual features, and tracking options.
             */
            Tracklet(std::unique_ptr<StatePredictor> kf, const Rect<float>& rect, int label,
                     float score, std::vector<float> feat = {}, float alpha = 0.9f,
                     bool enable_xyah = false, int max_feature_history = 100, int ocm_delta_t = 3,
                     bool enable_oru = false);

            /**
             * @brief Destroys the Tracklet instance.
             */
            ~Tracklet() = default;

            [[nodiscard]] const Rect<float>& get_rect() const { return rect_; }
            [[nodiscard]] TrackState get_track_state() const { return state_; }
            [[nodiscard]] bool is_activated() const { return is_activated_; }
            [[nodiscard]] bool is_confirmed() const { return is_confirmed_; }

            [[nodiscard]] float get_score() const { return score_; }
            [[nodiscard]] float get_smooth_score() const { return score_smooth_; }
            [[nodiscard]] int get_label() const { return label_; }
            [[nodiscard]] size_t get_track_id() const { return track_id_; }
            [[nodiscard]] size_t get_frame_id() const { return frame_id_; }
            [[nodiscard]] size_t get_start_frame_id() const { return start_frame_id_; }
            [[nodiscard]] size_t get_tracklet_length() const { return tracklet_len_; }
            [[nodiscard]] int get_hits() const { return hits_; }

            /**
             * @brief Returns the length of the current uninterrupted match streak.
             *
             * Reset whenever the track survives a frame without an observation.
             */
            [[nodiscard]] int get_consecutive_hits() const { return consecutive_hits_; }

            /**
             * @brief True when an IMM predictor is driving the motion regime.
             *
             * An IMM reports a non-negative manoeuvre probability; single-model
             * predictors (KalmanFilter/NSA) return the negative sentinel, so the
             * regime getters fall back to the Cruising default.
             */
            [[nodiscard]] bool imm_active() const
            {
                return kf_ && kf_->maneuver_probability() >= 0.0f;
            }

            /**
             * @brief Continuous [0, 1] manoeuvre likelihood for the track.
             *
             * Sourced from the IMM mode probabilities (mass on the CA + CT
             * models); 0 when no IMM predictor is present. This is the
             * continuous regime signal consumed by OCM, lost-buffer scaling and
             * association cost shaping.
             */
            [[nodiscard]] float get_maneuver_probability() const
            {
                return imm_active() ? maneuver_prob_ : 0.0f;
            }

            [[nodiscard]] KinematicState get_kinematic_state() const
            {
                return imm_active() ? imm_state_ : KinematicState::Cruising;
            }

            /**
             * @brief Retrieves the last observed motion regime.
             *
             * Resolves to the pre-loss state if coasting, or the active state.
             */
            [[nodiscard]] KinematicState get_last_seen_state() const
            {
                if (imm_active())
                    return imm_state_ == KinematicState::Coasting ? imm_pre_coast_ : imm_state_;
                return KinematicState::Cruising;
            }

            /**
             * @brief Returns the last raw matched detection box.
             *
             * Unmodified box of the most recently associated observation.
             */
            [[nodiscard]] const Rect<float>& get_last_observation() const
            {
                return last_observation_;
            }
            [[nodiscard]] bool has_last_observation() const { return has_last_observation_; }

            /**
             * @brief Components of the unit vector of observed travel direction.
             *
             * Computed across observations over the configured observation window.
             */
            [[nodiscard]] float get_obs_velocity_x() const { return obs_vx_; }
            [[nodiscard]] float get_obs_velocity_y() const { return obs_vy_; }

            /** @brief Observed speed magnitude in pixels per observed frame; zero for a near-stationary track. */
            [[nodiscard]] float get_obs_speed() const { return obs_speed_; }
            [[nodiscard]] bool has_obs_velocity() const { return obs_velocity_valid_; }

            /** @brief Frames elapsed since the last associated observation; zero on a freshly matched frame, incremented by each unobserved predict(). */
            [[nodiscard]] int get_frames_since_observation() const
            {
                return frames_since_observation_;
            }

            [[nodiscard]] const std::vector<float>& get_smooth_feat() const { return smooth_feat_; }
            [[nodiscard]] const std::vector<float>& get_curr_feat() const { return curr_feat_; }
            [[nodiscard]] bool has_feature() const { return !smooth_feat_.empty(); }

            /** 
             * @brief Sets the track to the active state and assigns identifiers.
             *
             * @param kf State predictor instance.
             * @param frame_id Current frame identifier.
             * @param track_id Unique tracking identifier.
             */
            void activate(std::unique_ptr<StatePredictor> kf, size_t frame_id, size_t track_id);

            /** 
             * @brief Sets the track to the tentative state for validation.
             *
             * @param kf State predictor instance.
             * @param frame_id Current frame identifier.
             * @param initial_id Initial tracking identifier.
             */
            void activate_tentative(std::unique_ptr<StatePredictor> kf, size_t frame_id,
                                    size_t initial_id);

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
            void re_activate(const Tracklet& new_track, size_t frame_id, int new_track_id = -1);

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
            void update(const Tracklet& new_track, size_t frame_id, bool update_features = true,
                        bool keep_state = false);

            /**
             * @brief Transitions the track to the lost state.
             * @param dt Time interval of the marking frame for lost-time counter.
             */
            void mark_as_lost(float dt = 0.0f);

            /**
             * @brief Transitions the track to the removed state.
             */
            void mark_as_removed();

            /**
             * @brief Resets the consecutive-match streak for tentative confirmation.
             */
            void reset_consecutive_hits() { consecutive_hits_ = 0; }

            /**
             * @brief Sets the internal track identifier.
             * @param id Unique tracking identifier.
             */
            void set_track_id(size_t id) { track_id_ = id; }

            /**
             * @brief Assigns a timing policy to the tracklet.
             * @param policy Shared pointer to the TimingPolicy instance.
             */
            void set_timing_policy(std::shared_ptr<TimingPolicy> policy)
            {
                timing_policy_ = std::move(policy);
            }

            /**
             * @brief Retrieves the accumulated time spent in the lost state.
             *
             * @return Total lost time in seconds or frame count.
             */
            [[nodiscard]] float get_accumulated_lost_time() const { return accumulated_lost_time_; }

            /**
             * @brief Updates visual features using exponential moving average.
             *
             * @param feat New observation feature vector.
             */
            void update_features(std::vector<float> feat);

            /** 
             * @brief Calculates maximum cosine similarity against feature history.
             *
             * @param detection_feat Feature vector for comparison.
             * @return Maximum similarity score.
             */
            [[nodiscard]] float
            get_max_feature_similarity(const std::vector<float>& detection_feat) const;

            [[nodiscard]] const std::deque<std::vector<float>>& get_feature_history() const
            {
                return feature_history_;
            }

            /**
             * @brief Retrieves a thread-safe copy of the current state mean.
             * @return State mean vector.
             */
            [[nodiscard]] KalmanTypes::StateMean get_mean() const;

            /**
             * @brief Retrieves a thread-safe copy of the current state covariance.
             * @return State covariance matrix.
             */
            [[nodiscard]] KalmanTypes::StateCov get_covariance() const;

            /**
             * @brief Formats the current state as a measurement vector for the predictor.
             * @return Measurement vector in the configured coordinate format.
             */
            [[nodiscard]] KalmanTypes::DetectBox get_measurement() const;

            mutable uint64_t visit_id_ = 0;

        private:
            std::unique_ptr<StatePredictor> kf_;
            KalmanTypes::StateMean mean_;
            KalmanTypes::StateCov covariance_;

            Rect<float> rect_;
            TrackState state_;
            bool is_activated_;
            bool is_confirmed_;
            float score_;
            float score_smooth_;
            int label_;
            size_t track_id_;
            size_t frame_id_;
            size_t start_frame_id_;
            size_t tracklet_len_;
            int hits_;
            int consecutive_hits_;

            std::shared_ptr<TimingPolicy> timing_policy_;

            // IMM-derived motion regime carrying the KinematicState contract.
            KinematicState imm_state_ = KinematicState::Cruising;
            KinematicState imm_pre_coast_ =
                KinematicState::Cruising; ///< Regime before going Lost, restored on recovery.
            float maneuver_prob_ = 0.0f;
            float stationary_prob_ = 0.0f;

            static constexpr float K_REGIME_STATIONARY_PROB =
                0.5f; ///< Near-constant-position mass above which the regime reads Stationary.
            static constexpr float K_REGIME_MANEUVER_PROB =
                0.5f; ///< CA+CT mass above which the regime reads Maneuvering.
            static constexpr float K_REGIME_STATIONARY_SPEED_FRAC =
                0.02f; ///< Combined speed below this fraction of box height per step also reads Stationary; the CV mode subsumes a motionless target so the NCP mode mass alone under-triggers.

            /**
             * @brief Refreshes the IMM-derived regime from predictor probabilities.
             * @param is_lost True when the track currently has no observation.
             */
            void refresh_regime(bool is_lost);

            std::vector<float> curr_feat_;
            std::vector<float> smooth_feat_;

            size_t max_feature_history_;
            std::deque<std::vector<float>> feature_history_;
            float accumulated_lost_time_ = 0.0f;
            int frames_since_observation_ =
                0; ///< Tracklet-owned shared counter; consumed independently by OCM's velocity gate and the tentative frame-counted age budget, not OCM-private.

            Rect<float> last_observation_;
            bool has_last_observation_ = false;
            int ocm_delta_t_ = 3;
            float obs_vx_ = 0.0f;
            float obs_vy_ = 0.0f;
            float obs_speed_ = 0.0f;
            bool obs_velocity_valid_ = false;

            struct ObsSample
            {
                    size_t frame_id;
                    float cx;
                    float cy;
            };
            std::deque<ObsSample>
                obs_history_; ///< Recent observation centres, capped to ocm_delta_t_ + 1, used to measure the direction of travel from raw detections.

            float last_dt_ = 1.0f;

            float alpha_;
            bool enable_xyah_;
            bool enable_oru_ = false;

            // Checkpoint captured at last observation for ORU virtual trajectory replay.
            KalmanTypes::StateMean last_obs_mean_;
            KalmanTypes::StateCov last_obs_cov_;
            bool has_checkpoint_ = false;

            /** @brief Captures the estimator state (Tracklet mean/cov + predictor internals) at a real observation. */
            void capture_checkpoint();

            /**
             * @brief Replays predict/update steps along virtual recovery trajectory.
             * @param new_meas Recovery detection in measurement space.
             * @param gap Unobserved frames bridged.
             * @param score Recovery detection confidence.
             * @return True if recovery re-update succeeded.
             */
            [[nodiscard]] bool recovery_reupdate(const KalmanTypes::DetectBox& new_meas, int gap,
                                                 float score);

            /** @brief Converts a box to the predictor's measurement vector (matches get_measurement, lock-free). */
            [[nodiscard]] KalmanTypes::DetectBox to_measurement(const Rect<float>& r) const;

            static constexpr float K_SCORE_SMOOTHING_MOMENTUM =
                0.7f; ///< EMA weight for the tentative confirmation score; smoothing stops a one-frame spike from collapsing the required hit count.

            void add_feature_to_history(const std::vector<float>& feat)
            {
                if (feature_history_.size() >= max_feature_history_)
                    feature_history_.pop_front();
                feature_history_.push_back(feat);
            }

            void update_rect();
            void update_rect_xyah();

            /**
             * @brief Records a raw observation and refreshes direction estimate.
             * @param obs_rect Raw detection box of the matched observation.
             * @param frame_id Frame in which the observation occurred.
             */
            void record_observation(const Rect<float>& obs_rect, size_t frame_id);

            static void normalize_l_2(std::vector<float>& v);

            mutable std::mutex state_mutex_;
    };

} // namespace SushiTrack
