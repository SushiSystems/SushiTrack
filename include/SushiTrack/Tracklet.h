/**
 * @file      Tracklet.h
 * @brief     Management of individual track states, properties, and feature history.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      28.04.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */

#pragma once

#include "SushiTrack/IStatePredictor.h"
#include "SushiTrack/ITimingPolicy.h"
#include "SushiTrack/Rect.h"
#include "SushiTrack/KalmanTypes.h"
#include "SushiTrack/TrackFSM.h"

#include <cstdint>
#include <cstddef>
#include <deque>
#include <memory>
#include <vector>

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
             * @param kf State predictor instance (Kalman Filter).
             * @param rect Initial geometric bounding box.
             * @param label Object class identifier.
             * @param score Detection confidence score.
             * @param feat Visual feature vector.
             * @param alpha EMA weight for feature updates.
             * @param enable_xyah Coordinate system flag.
             */
            Tracklet(std::unique_ptr<IStatePredictor> kf,
                   const Rect<float>& rect, int label, float score,
                   std::vector<float> feat = {}, float alpha = 0.9f,
                   bool enable_xyah = false, int max_feature_history = 100);

            ~Tracklet() = default;

            const Rect<float>& getRect()       const { return rect_; }
            TrackState         getTrackState() const { return state_; }
            bool               isActivated()   const { return is_activated_; }
            bool               isConfirmed()   const { return is_confirmed_; }

            float              getScore()      const { return score_; }
            int                getLabel()      const { return label_; }
            size_t             getTrackId()    const { return track_id_; }
            size_t             getFrameId()    const { return frame_id_; }
            size_t             getStartFrameId() const { return start_frame_id_; }
            size_t             getTrackletLength() const { return tracklet_len_; }
            int                getHits() const { return hits_; }
            TrackFSM*          getFSM() const { return fsm_.get(); }
            KinematicState     getKinematicState() const { return fsm_ ? fsm_->getCurrentState() : KinematicState::Cruising; }

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
             * @brief Re-associates a lost track with a new observation.
             *
             * When Observation-Centric Online Smoothing is enabled on the
             * tracklet and a saved Kalman state from the last observation is
             * available, the predictor is rolled back to that state and a
             * sequence of virtual measurements interpolated between the saved
             * observation and the new one is replayed before the actual update.
             * This corrects the trajectory drift accumulated during the lost
             * period.
             *
             * @param new_track New observation data.
             * @param frame_id Current frame identifier.
             * @param new_track_id New tracking identifier (optional).
             * @param dt_per_step Per-step time interval used for the smoothing
             *                    replay. Has no effect when smoothing is disabled.
             */
            void reActivate(const Tracklet& new_track, size_t frame_id, int new_track_id = -1, float dt_per_step = 1.0f);

            /** 
             * @brief Projects the motion state to a future timestamp. 
             * 
             * @param dt Time interval for prediction.
             */
            void predict(float dt = 1.0f);

            /** 
             * @brief Refines the motion state and bounding box using a new observation. 
             * 
             * @param new_track Matched detection data.
             * @param frame_id Current frame identifier.
             * @param update_features Flag for updating visual features.
             */
            void update(const Tracklet& new_track, size_t frame_id, bool update_features = true);

            /**
             * @brief Transitions the track to the lost state.
             *
             * Initializes the accumulated lost-time counter with the time interval
             * of the frame in which the transition occurs. This is required so that
             * the wall-clock and frame-counted expiration policies remove the track
             * on the same frame: the marking frame is itself a frame spent without
             * an observation, so it must be counted. Defaulting the parameter to
             * zero preserves the legacy call signature.
             *
             * @param dt Time interval of the marking frame.
             */
            void markAsLost(float dt = 0.0f);

            /** 
             * @brief Transitions the track to the removed state. 
             */
            void markAsRemoved();

            void setTrackId(size_t id) { track_id_ = id; }
            void setFSM(std::unique_ptr<TrackFSM> fsm) { fsm_ = std::move(fsm); }
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

            KalmanTypes::DetectBox getMeasurement() const;
            const KalmanTypes::StateMean& getMean() const { return mean_; }
            const KalmanTypes::StateCov&  getCovariance() const { return covariance_; }

            /**
             * @brief Reports whether at least one raw observation has been
             *        recorded for this tracklet.
             */
            bool hasLastObservation() const { return has_observation_; }

            /**
             * @brief Retrieves the last raw observation that was applied to
             *        this tracklet.
             *
             * @return Reference to the most recent observation in measurement
             *         space. Only valid when hasLastObservation() returns true.
             */
            const KalmanTypes::DetectBox& getLastObservation() const { return last_observation_; }

            /**
             * @brief Retrieves the frame identifier associated with the last
             *        recorded observation.
             */
            size_t getLastObservationFrameId() const { return last_observation_frame_id_; }

            /**
             * @brief Computes the unit-direction vector from an earlier
             *        observation to the most recent one.
             *
             * Walks the observation history from the most recent entry backwards
             * until an observation at least delta_t frames older is found. The
             * resulting two-dimensional unit vector is used by the Observation-
             * Centric Momentum cost decorator to penalize candidates that lie
             * opposite the recent direction of motion.
             *
             * @param delta_t Minimum frame separation between the reference
             *                observation and the latest one.
             * @param out_direction Output unit-direction vector. Set to the
             *                      zero vector when not enough history exists.
             * @return True when a valid direction could be computed.
             */
            bool getObservationDirection(int delta_t, Eigen::Matrix<float, 1, 2>& out_direction) const;

            /**
             * @brief Reports the number of frames spent in the lost state
             *        since the last observation.
             */
            size_t getLostFrames() const { return lost_frames_; }

            /**
             * @brief Configures the maximum number of past observations that
             *        will be retained for momentum estimation and smoothing.
             *
             * @param n Capacity of the observation ring buffer.
             */
            void setMaxObservationHistory(int n);

            /**
             * @brief Enables or disables Observation-Centric Online Smoothing
             *        for this tracklet.
             *
             * @param v True to enable smoothing during reActivate.
             */
            void setOOSEnabled(bool v) { enable_oos_ = v; }

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
            int         label_;
            size_t      track_id_;
            size_t      frame_id_;
            size_t      start_frame_id_;
            size_t      tracklet_len_;
            int         hits_;

            std::unique_ptr<TrackFSM>    fsm_;
            std::shared_ptr<ITimingPolicy> timing_policy_;

            std::vector<float> curr_feat_;
            std::vector<float> smooth_feat_;
            
            size_t max_feature_history_;
            std::deque<std::vector<float>> feature_history_;
            float accumulated_lost_time_ = 0.0f;
            size_t lost_frames_ = 0;

            /**
             * @brief Bounded observation history retained for momentum and
             *        smoothing computations.
             *
             * Each entry holds the frame identifier and the raw measurement
             * vector recorded at that frame. The buffer is trimmed from the
             * front whenever its size exceeds max_observation_history_.
             */
            std::deque<std::pair<size_t, KalmanTypes::DetectBox>> observation_history_;
            size_t max_observation_history_ = 8;

            KalmanTypes::DetectBox last_observation_; ///< Most recent raw observation in measurement space.
            size_t last_observation_frame_id_ = 0;    ///< Frame identifier of the most recent observation.
            bool   has_observation_ = false;          ///< Set on first call to recordObservation.

            KalmanTypes::StateMean saved_mean_;       ///< Predictor mean snapshot at the last observation.
            KalmanTypes::StateCov  saved_covariance_; ///< Predictor covariance snapshot at the last observation.
            size_t saved_frame_id_ = 0;               ///< Frame identifier of the saved snapshot.
            bool   has_saved_state_ = false;          ///< True once a snapshot has been captured.
            bool   enable_oos_ = false;               ///< Toggle for Observation-Centric Online Smoothing on reActivate.

            float              alpha_;
            bool               enable_xyah_;

            void addFeatureToHistory(const std::vector<float>& feat) 
            {
                if (feature_history_.size() >= max_feature_history_)
                    feature_history_.pop_front();
                feature_history_.push_back(feat);
            }

            void updateRect();
            void updateRectXyah();

            /**
             * @brief Records a raw observation in the bounded history buffer
             *        and updates the last-observation snapshot.
             *
             * @param meas Measurement vector applied to the predictor.
             * @param frame_id Frame identifier associated with the measurement.
             */
            void recordObservation(const KalmanTypes::DetectBox& meas, size_t frame_id);

            /**
             * @brief Captures the current predictor mean and covariance for
             *        later restoration by Observation-Centric Online Smoothing.
             *
             * @param frame_id Frame identifier of the snapshot.
             */
            void saveStateSnapshot(size_t frame_id);

            static void normalizeL2(std::vector<float>& v);
    };

} // namespace SushiTrack
