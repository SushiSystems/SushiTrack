/**
 * @file      Tracklet.cpp
 * @brief     Implementation of the Tracklet class for managing individual track lifecycles.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      28.04.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */

#include "SushiTrack/Tracklet.h"
#include "SushiTrack/Logger.h"

#include <cmath>
#include <cstddef>
#include <utility>
#include <algorithm>

namespace SushiTrack
{
    /**
     * @brief Performs L2 normalization on a feature vector.
     * 
     * @param v Reference to the vector to be normalized.
     */
    void Tracklet::normalizeL2(std::vector<float>& v)
    {
        float norm = 0.f;
        for (float f : v)
        {
            norm += f * f;
        }
        norm = std::sqrt(norm);
        if (norm > 1e-8f)
        {
            for (float& f : v)
            {
                f /= norm;
            }
        }
    }

    /**
     * @brief Initializes a new tracklet instance.
     * 
     * @param kf State predictor instance (Kalman Filter).
     * @param rect Initial geometric bounding box.
     * @param label Object class identifier.
     * @param score Detection confidence score.
     * @param feat Visual feature vector.
     * @param alpha Exponential moving average weight for feature updates.
     * @param enable_xyah Coordinate system flag.
     * @param max_feature_history Maximum number of features to store in history.
     */
    Tracklet::Tracklet(std::unique_ptr<IStatePredictor> kf,
                    const Rect<float>& rect,
                    int label, float score,
                    std::vector<float> feat,
                    float alpha,
                    bool enable_xyah,
                    int max_feature_history)
        : kf_(std::move(kf)),
        mean_(KalmanTypes::StateMean::Zero()),
        covariance_(KalmanTypes::StateCov::Zero()),
        rect_(rect),
        state_(TrackState::New),
        is_activated_(false),
        is_confirmed_(false),
        score_(score),
        label_(label),
        track_id_(0),
        frame_id_(0),
        start_frame_id_(0),
        tracklet_len_(0),
        hits_(0),
        alpha_(alpha),
        enable_xyah_(enable_xyah),
        max_feature_history_(static_cast<size_t>(max_feature_history))
    {
        if (!feat.empty())
        {
            updateFeatures(std::move(feat));
        }
    }

    /**
     * @brief Transitions the tracklet to the Tracked state.
     *
     * The is_activated flag is set only on the very first frame of the sequence;
     * tracks created later are kept inactive until they are matched again on the
     * subsequent frame. This implicit one-frame validation suppresses single-
     * frame ghost outputs that would otherwise propagate to consumers as false
     * positives, and matches the convention used by the BYTE reference
     * implementation when the explicit tentative pipeline is disabled.
     *
     * @param kf State predictor instance for the tracklet.
     * @param frame_id Current global frame identifier.
     * @param track_id Unique identifier for the track.
     */
    void Tracklet::activate(std::unique_ptr<IStatePredictor> kf, size_t frame_id, size_t track_id)
    {
        kf_ = std::move(kf);
        const KalmanTypes::DetectBox initial_meas = getMeasurement();
        kf_->initiate(mean_, covariance_, initial_meas, score_);
        updateRect();
        state_ = TrackState::Tracked;
        is_activated_ = (frame_id == 1);
        is_confirmed_ = (frame_id == 1);
        track_id_ = track_id;
        frame_id_ = frame_id;
        start_frame_id_ = frame_id;
        tracklet_len_ = 1;
        hits_ = 1;
        accumulated_lost_time_ = 0.0f;
    }

    /**
     * @brief Transitions the tracklet to the Tentative state.
     * 
     * @param kf State predictor instance.
     * @param frame_id Current frame identifier.
     * @param initial_id Initial tracking identifier (may be zero).
     */
    void Tracklet::activateTentative(std::unique_ptr<IStatePredictor> kf, size_t frame_id, size_t initial_id)
    {
        kf_ = std::move(kf);
        const KalmanTypes::DetectBox initial_meas = getMeasurement();
        kf_->initiate(mean_, covariance_, initial_meas, score_);
        updateRect();
        state_ = TrackState::Tentative;
        is_activated_ = false;
        is_confirmed_ = false;
        track_id_ = initial_id;
        frame_id_ = frame_id;
        start_frame_id_ = frame_id;
        tracklet_len_ = 1;
        hits_ = 1;
        accumulated_lost_time_ = 0.0f;
    }
    
    /**
     * @brief Promotes a tentative tracklet to a confirmed tracked state.
     * 
     * @param frame_id Current frame identifier.
     * @param final_id Assigned unique tracking identifier.
     */
    void Tracklet::confirm(size_t frame_id, size_t final_id)
    {
        state_ = TrackState::Tracked;
        is_activated_ = true;
        is_confirmed_ = true;
        if (final_id > 0)
        {
            track_id_ = final_id;
        }
        frame_id_ = frame_id;
    }

    /**
     * @brief Recovers a lost tracklet using new observation data.
     *
     * @param new_track Observed detection data wrapped in a tracklet container.
     * @param frame_id Current frame identifier.
     * @param new_track_id New tracking identifier (optional).
     */
    void Tracklet::reActivate(const Tracklet& new_track, size_t frame_id, int new_track_id)
    {
        const KalmanTypes::DetectBox new_meas = new_track.getMeasurement();

        kf_->update(mean_, covariance_, new_meas, new_track.getScore());

        updateRect();
        state_ = TrackState::Tracked;
        is_activated_ = true;
        is_confirmed_ = true;
        score_ = new_track.getScore();
        label_ = new_track.getLabel();

        if (!new_track.getCurrFeat().empty())
        {
            updateFeatures(new_track.getCurrFeat());
        }

        if (new_track_id >= 0)
        {
            track_id_ = static_cast<size_t>(new_track_id);
        }

        frame_id_ = frame_id;
        hits_++;
        tracklet_len_ = frame_id - start_frame_id_ + 1;
        accumulated_lost_time_ = 0.0f;

        // Successful recovery invalidates any occluder snapshot taken at the
        // moment this track went dark. Retaining the snapshot would bias
        // subsequent matches on the basis of a relationship that has already
        // been resolved by the recovery event itself.
        occluder_id_             = -1;
        occluder_snapshot_frame_ = 0;
        occluder_snapshot_time_  = 0.0f;
        interaction_state_       = InteractionState::Free;

        if (fsm_)
        {
            fsm_->updateState(mean_, false, tracklet_len_, last_dt_);
        }
    }

    /**
     * @brief Predicts the next state of the tracklet.
     * 
     * @param dt Time interval for prediction.
     */
    void Tracklet::predict(float dt)
    {
        std::lock_guard<std::mutex> lock(state_mutex_);

        last_dt_ = dt;

        if (state_ != TrackState::Tracked)
        {
            // Match the reference ByteTrack STrack::predict policy: when a
            // track is not actively being observed, freeze the size-velocity
            // components only. Position velocity is left intact so that a
            // walking target keeps propagating through brief occlusions and
            // gets re-acquired close to its true position when a detection
            // reappears. The previous policy zeroed aspect/height velocity
            // AND damped (x, y) velocity by 5% per frame, which made Lost
            // tracks stall and produced wrong-position re-associations on
            // clean sequences (visible as elevated IDSW on MOT17-04-SDP).
            if (enable_xyah_)
            {
                // state = [x, y, a, h, vx, vy, va, vh]
                // freeze height velocity, leave (vx, vy, va) intact.
                mean_[7] = 0.f;
            }
            else
            {
                // state = [x, y, w, h, vx, vy, vw, vh]
                // freeze size velocities (vw, vh), leave (vx, vy) intact.
                mean_[6] = 0.f;
                mean_[7] = 0.f;
            }
        }

        if (fsm_ && kf_) 
        {
            KinematicState old_fsm = fsm_->getCurrentState();
            fsm_->updateState(mean_, state_ == TrackState::Lost, tracklet_len_, dt);
            KinematicState new_fsm = fsm_->getCurrentState();
            if (old_fsm != new_fsm) 
            {
                SUSHITRACK_LOG_DEBUG("[Tracklet] Track %llu FSM state: %d -> %d", 
                    (unsigned long long)track_id_, (int)old_fsm, (int)new_fsm);
            }
            fsm_->applyToPredictor(kf_.get(), dt);
        }

        const float kf_dt = timing_policy_ ? timing_policy_->toKfDt(dt) : 1.0f;
        kf_->predict(mean_, covariance_, kf_dt);
        updateRect();

        if (state_ == TrackState::Lost || state_ == TrackState::Tentative)
        {
            accumulated_lost_time_ += dt;
        }
    }

    /**
     * @brief Updates the tracklet state with a matched observation.
     *
     * The lifecycle flag transition (state_=Tracked, is_activated_=true,
     * is_confirmed_=true) is gated by @p keep_state. The tentative pipeline
     * passes keep_state=true so a freshly matched but not-yet-confirmed
     * tentative does not leak as an activated track output for the duration
     * of the hit-accumulation window. The flags are committed by
     * Tracklet::confirm(...) once the dynamic hit threshold is met.
     *
     * @param new_track Matched detection data.
     * @param frame_id Current frame identifier.
     * @param update_features Flag for updating visual features with the observation.
     * @param keep_state When true, suppresses the lifecycle flag transition.
     */
    void Tracklet::update(const Tracklet& new_track, size_t frame_id,
                          bool update_features, bool keep_state)
    {
        const KalmanTypes::DetectBox new_meas = new_track.getMeasurement();

        std::lock_guard<std::mutex> lock(state_mutex_);

        kf_->update(mean_, covariance_, new_meas, new_track.getScore());
        updateRect();

        score_ = new_track.getScore();
        label_ = new_track.getLabel();

        if (update_features && !new_track.getCurrFeat().empty())
        {
            updateFeatures(new_track.getCurrFeat());
        }

        if (!keep_state)
        {
            state_ = TrackState::Tracked;
            is_activated_ = true;
            is_confirmed_ = true;
        }
        frame_id_ = frame_id;
        hits_++;
        tracklet_len_ = frame_id - start_frame_id_ + 1;
        accumulated_lost_time_ = 0.0f;

        // A confirmed observation supersedes any prior occluder snapshot. The
        // snapshot is only meaningful while the track is in Lost; once it is
        // back under direct observation the recorded relationship is stale.
        if (!keep_state)
        {
            occluder_id_             = -1;
            occluder_snapshot_frame_ = 0;
            occluder_snapshot_time_  = 0.0f;
            interaction_state_       = InteractionState::Free;
        }

        if (fsm_)
        {
            fsm_->updateState(mean_, false, tracklet_len_, last_dt_);
        }
    }

    /**
     * @brief Retrieves a thread-safe copy of the current state mean.
     * 
     * @return State mean vector.
     */
    KalmanTypes::StateMean Tracklet::getMean() const 
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        return mean_; // Referans değil, kopyası (return-by-value) döner
    }

    /**
     * @brief Retrieves a thread-safe copy of the current state covariance.
     * 
     * @return State covariance matrix.
     */
    KalmanTypes::StateCov Tracklet::getCovariance() const 
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        return covariance_; // Referans değil, kopyası (return-by-value) döner
    }

    /**
     * @brief Marks the tracklet as lost.
     *
     * The accumulated lost-time counter is initialized with the time interval of
     * the current frame so that the marking frame itself contributes to the
     * lifetime budget. This keeps the wall-clock expiration aligned with the
     * frame-counted expiration, where the same frame is implicitly part of the
     * frame-difference computation.
     *
     * @param dt Time interval of the frame triggering the transition.
     */
    void Tracklet::markAsLost(float dt) 
    {
        state_ = TrackState::Lost;
        is_activated_ = false;
        accumulated_lost_time_ = dt;
    }

    /**
     * @brief Marks the tracklet for removal from the tracking pool.
     */
    void Tracklet::markAsRemoved()
    {
        state_ = TrackState::Removed;
        is_activated_ = false;
    }

    /**
     * @brief Updates the tracklet's visual feature history and smoothed feature.
     * 
     * @param feat New observation feature vector.
     */
    void Tracklet::updateFeatures(std::vector<float> feat) 
    {
        curr_feat_ = std::move(feat);
        normalizeL2(curr_feat_);

        addFeatureToHistory(curr_feat_);

        if (smooth_feat_.empty() || smooth_feat_.size() != curr_feat_.size())
            smooth_feat_ = curr_feat_;
        else
        { 
            for (size_t i = 0; i < smooth_feat_.size(); ++i)
                smooth_feat_[i] = alpha_ * smooth_feat_[i] + (1.f - alpha_) * curr_feat_[i];
            normalizeL2(smooth_feat_);
        }
    }

    /**
     * @brief Computes maximum cosine similarity against the feature history.
     * 
     * @param detection_feat Input feature vector for comparison.
     * @return Maximum similarity score in the range [0, 1].
     */
    float Tracklet::getMaxFeatureSimilarity(const std::vector<float>& detection_feat) const {
        if (feature_history_.empty() && smooth_feat_.empty()) return 1.0f;

        float max_similarity = 0.0f;

        // Check against historical feature collection
        for (const auto& hist_feat : feature_history_) 
        {
            if (hist_feat.size() != detection_feat.size()) continue;

            float dot = 0.0f;
            for (size_t i = 0; i < hist_feat.size(); ++i)
                dot += hist_feat[i] * detection_feat[i];

            max_similarity = std::max(max_similarity, dot);
        }

        // Check against smoothed feature representation
        if (!smooth_feat_.empty() && smooth_feat_.size() == detection_feat.size()) 
        {
            float dot = 0.0f;
            for (size_t i = 0; i < smooth_feat_.size(); ++i)
                dot += smooth_feat_[i] * detection_feat[i];
            max_similarity = std::max(max_similarity, dot);
        }

        return max_similarity;
    }

    /**
     * @brief Synchronizes the geometric rectangle with the current state mean.
     */
    void Tracklet::updateRect()
    {
        if (enable_xyah_)
        {
            updateRectXyah();
            return;
        }
        rect_.width()  = std::max(1.0f, mean_[2]);
        rect_.height() = std::max(1.0f, mean_[3]);
        rect_.x()      = mean_[0] - rect_.width()  * 0.5f;
        rect_.y()      = mean_[1] - rect_.height() * 0.5f;
    }

    /**
     * @brief Synchronizes the rectangle using the xyah coordinate system.
     */
    void Tracklet::updateRectXyah()
    {
        float h = std::max(1.0f, mean_[3]);
        float a = std::max(0.01f, mean_[2]);
        float w = a * h;
        rect_.width()  = w;
        rect_.height() = h;
        rect_.x()      = mean_[0] - w * 0.5f;
        rect_.y()      = mean_[1] - h * 0.5f;
    }

    /**
     * @brief Formats the current state as a measurement vector for the predictor.
     * 
     * @return Measurement vector in the configured coordinate format.
     */
    KalmanTypes::DetectBox Tracklet::getMeasurement() const
    {
        std::lock_guard<std::mutex> lock(state_mutex_);

        if (enable_xyah_)
        {
            float cx = rect_.x() + rect_.width() * 0.5f;
            float cy = rect_.y() + rect_.height() * 0.5f;
            float h  = rect_.height();
            float a  = rect_.width() / std::max(1e-6f, h);
            KalmanTypes::DetectBox m;
            m << cx, cy, a, h;
            return m;
        }
        return rect_.getXywh();
    }

} // namespace SushiTrack
