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
             */
            Tracklet(std::unique_ptr<IStatePredictor> kf,
                   const Rect<float>& rect, int label, float score,
                   std::vector<float> feat = {}, float alpha = 0.9f,
                   bool enable_xyah = false, int max_feature_history = 100);

            /**
             * @brief Destroys the Tracklet instance.
             */
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

            float last_dt_ = 1.0f;

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

            static void normalizeL2(std::vector<float>& v);

            mutable std::mutex state_mutex_;
    };

} // namespace SushiTrack
