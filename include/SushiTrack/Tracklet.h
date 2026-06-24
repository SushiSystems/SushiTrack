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
             * @param kf State predictor instance (Kalman Filter).
             * @param rect Initial geometric bounding box.
             * @param label Object class identifier.
             * @param score Detection confidence score.
             * @param feat Visual feature vector.
             * @param alpha EMA weight for feature updates.
             * @param enable_xyah Coordinate system flag.
             * @param max_feature_history Maximum number of visual appearance vectors stored per tracklet.
             * @param ocm_delta_t Observation window (in observed frames) for the observation-centric direction estimate.
             */
            Tracklet(std::unique_ptr<IStatePredictor> kf,
                   const Rect<float>& rect, int label, float score,
                   std::vector<float> feat = {}, float alpha = 0.9f,
                   bool enable_xyah = false, int max_feature_history = 100,
                   int ocm_delta_t = 3);

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
             * Unlike getHits() (a monotonic lifetime match count) this counter is
             * reset to zero by resetConsecutiveHits() whenever the track survives a
             * frame without an observation. The tentative pipeline confirms on the
             * consecutive streak, not the lifetime count, so a strobing false
             * positive that only matches on alternate frames can never accumulate
             * enough streak to be promoted.
             */
            int                getConsecutiveHits() const { return consecutive_hits_; }
            TrackFSM*          getFSM() const { return fsm_.get(); }
            KinematicState     getKinematicState() const { return fsm_ ? fsm_->getCurrentState() : KinematicState::Cruising; }

            /**
             * @brief Retrieves the last observed motion regime.
             *
             * Resolves to the kinematic state recorded just before the track
             * went Lost when the FSM is currently Coasting, or the active
             * state otherwise. When no FSM is attached this falls back to
             * Cruising so that downstream policies degrade to identity
             * behaviour rather than a special-cased no-op.
             */
            KinematicState     getLastSeenState() const { return fsm_ ? fsm_->getLastSeenState() : KinematicState::Cruising; }

            /**
             * @brief Returns the last raw matched detection box.
             *
             * Unlike getRect(), which tracks the Kalman-smoothed (and, after
             * predict(), extrapolated) estimate, this is the unmodified box of
             * the most recently associated observation. It is the anchor from
             * which observation-centric momentum measures the direction to a
             * candidate detection. Valid only when hasLastObservation() is true.
             */
            const Rect<float>& getLastObservation() const { return last_observation_; }
            bool  hasLastObservation() const { return has_last_observation_; }

            /**
             * @brief Components of the unit vector of the track's observed
             *        direction of travel.
             *
             * Computed from two raw observations spanning the configured
             * observation window (not from Kalman velocity), so the direction
             * stays reliable in the low-frame-rate and nonlinear regimes where
             * the filter's velocity estimate lags. Valid only when
             * hasObsVelocity() is true.
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
             * @brief Refines the motion state and bounding box using a new observation.
             *
             * When @p keep_state is true the lifecycle flags (state_, is_activated_,
             * is_confirmed_) are left untouched. This is required by the tentative
             * matching pipeline, which must accumulate hits across multiple frames
             * before promoting a track to the confirmed/activated state. Defaulting
             * the flag to false preserves the legacy call signature for the
             * confirmed-track and lost-recovery paths.
             *
             * @param new_track Matched detection data.
             * @param frame_id Current frame identifier.
             * @param update_features Flag for updating visual features.
             * @param keep_state When true, suppresses the lifecycle flag transition.
             */
            void update(const Tracklet& new_track, size_t frame_id,
                        bool update_features = true, bool keep_state = false);

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

            /**
             * @brief Resets the consecutive-match streak used for tentative
             *        confirmation.
             *
             * Called by the tentative pipeline when a track survives a frame
             * without an observation. The monotonic lifetime hit count (hits_)
             * is intentionally left untouched so output and lifecycle bookkeeping
             * stay continuous.
             */
            void resetConsecutiveHits() { consecutive_hits_ = 0; }

            /**
             * @brief Sets the internal track identifier.
             * @param id Unique tracking identifier.
             */
            void setTrackId(size_t id) { track_id_ = id; }

            /**
             * @brief Assigns a kinematic state machine to the tracklet.
             * @param fsm Unique pointer to the TrackFSM instance.
             */
            void setFSM(std::unique_ptr<TrackFSM> fsm) { fsm_ = std::move(fsm); }

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

            std::unique_ptr<TrackFSM>    fsm_;
            std::shared_ptr<ITimingPolicy> timing_policy_;

            std::vector<float> curr_feat_;
            std::vector<float> smooth_feat_;
            
            size_t max_feature_history_;
            std::deque<std::vector<float>> feature_history_;
            float accumulated_lost_time_ = 0.0f;

            Rect<float> last_observation_;
            bool  has_last_observation_ = false;
            int   ocm_delta_t_ = 3;
            float obs_vx_ = 0.0f;
            float obs_vy_ = 0.0f;
            float obs_speed_ = 0.0f;
            bool  obs_velocity_valid_ = false;
            int   frames_since_observation_ = 0;

            struct ObsSample { size_t frame_id; float cx; float cy; };
            std::deque<ObsSample> obs_history_; ///< Recent observation centres, capped to ocm_delta_t_ + 1, used to measure the direction of travel from raw detections.

            float last_dt_ = 1.0f;

            float              alpha_;
            bool               enable_xyah_;

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
             * @brief Records a raw observation and refreshes the observation-centric
             *        direction estimate.
             *
             * Stores the box as the new last observation, resets the
             * frames-since-observation counter, and recomputes the direction of
             * travel over the configured observation window. Lock-free: callers
             * that mutate shared state (update/reActivate) already hold the state
             * mutex; the activation paths run single-threaded at track birth.
             *
             * @param obs_rect Raw detection box of the matched observation.
             * @param frame_id Frame in which the observation occurred.
             */
            void recordObservation(const Rect<float>& obs_rect, size_t frame_id);

            static void normalizeL2(std::vector<float>& v);

            mutable std::mutex state_mutex_;
    };

} // namespace SushiTrack
