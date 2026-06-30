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
                    int max_feature_history,
                    int ocm_delta_t,
                    bool enable_oru)
        : kf_(std::move(kf)),
        mean_(KalmanTypes::StateMean::Zero()),
        covariance_(KalmanTypes::StateCov::Zero()),
        rect_(rect),
        state_(TrackState::New),
        is_activated_(false),
        is_confirmed_(false),
        score_(score),
        score_smooth_(score),
        label_(label),
        track_id_(0),
        frame_id_(0),
        start_frame_id_(0),
        tracklet_len_(0),
        hits_(0),
        consecutive_hits_(0),
        alpha_(alpha),
        enable_xyah_(enable_xyah),
        enable_oru_(enable_oru),
        max_feature_history_(static_cast<size_t>(max_feature_history)),
        ocm_delta_t_(std::max(1, ocm_delta_t))
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
        consecutive_hits_ = 1;
        score_smooth_ = score_;
        accumulated_lost_time_ = 0.0f;
        recordObservation(rect_, frame_id);
        captureCheckpoint();
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
        consecutive_hits_ = 1;
        score_smooth_ = score_;
        accumulated_lost_time_ = 0.0f;
        recordObservation(rect_, frame_id);
        captureCheckpoint();
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

        // Hold the state mutex for the whole mutation (mirrors update()) so concurrent readers never see a torn mean_/covariance_.
        std::lock_guard<std::mutex> lock(state_mutex_);

        // ORU bridges the unobserved gap along a virtual straight-line trajectory;
        // it falls back to a plain update when disabled or the gap is too small.
        const int gap = frames_since_observation_;
        if (!recoveryReupdate(new_meas, gap, new_track.getScore()))
            kf_->update(mean_, covariance_, new_meas, new_track.getScore());

        updateRect();
        state_ = TrackState::Tracked;
        is_activated_ = true;
        is_confirmed_ = true;
        score_ = new_track.getScore();
        score_smooth_ = kScoreSmoothingMomentum * score_smooth_ + (1.f - kScoreSmoothingMomentum) * score_;
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
        consecutive_hits_++;
        tracklet_len_ = frame_id - start_frame_id_ + 1;
        accumulated_lost_time_ = 0.0f;
        recordObservation(new_track.getRect(), frame_id);
        captureCheckpoint();

        refreshRegime(false);
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
        frames_since_observation_++;

        if (state_ != TrackState::Tracked)
        {
            // ByteTrack STrack::predict policy: on an unobserved track freeze only the size velocities, keeping (vx, vy) so it propagates through brief occlusions.
            if (enable_xyah_)
            {
                // [x, y, a, h, vx, vy, va, vh]: freeze height velocity only.
                mean_[7] = 0.f;
            }
            else
            {
                // [x, y, w, h, vx, vy, vw, vh]: freeze (vw, vh) only.
                mean_[6] = 0.f;
                mean_[7] = 0.f;
            }
        }

        const float kf_dt = timing_policy_ ? timing_policy_->toKfDt(dt) : 1.0f;
        kf_->predict(mean_, covariance_, kf_dt);
        updateRect();

        // Unobserved frame: drive the IMM regime into Coasting. The per-mode
        // process noise is intrinsic to the IMM, so there is nothing to set on
        // the predictor before predicting (unlike the removed hand-switched path).
        if (state_ == TrackState::Lost)
            refreshRegime(true);

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
        score_smooth_ = kScoreSmoothingMomentum * score_smooth_ + (1.f - kScoreSmoothingMomentum) * score_;
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
        consecutive_hits_++;
        tracklet_len_ = frame_id - start_frame_id_ + 1;
        accumulated_lost_time_ = 0.0f;
        recordObservation(new_track.getRect(), frame_id);
        captureCheckpoint();

        refreshRegime(false);
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
        refreshRegime(true);
    }

    /**
     * @brief Refreshes the IMM-derived motion regime from the predictor.
     *
     * Reports the regime semantics (Stationary / Cruising / Maneuvering /
     * Coasting) from the IMM mode probabilities so downstream consumers read a
     * single regime contract.
     *
     * @param is_lost True when the track currently has no observation.
     */
    void Tracklet::refreshRegime(bool is_lost)
    {
        if (!kf_) return;
        const float m = kf_->maneuverProbability();
        if (m < 0.0f) return;   // single-model predictor: leave the Cruising default in charge

        maneuver_prob_   = m;
        stationary_prob_ = kf_->stationaryProbability();

        if (is_lost)
        {
            // Capture the pre-dark regime once so recovery can restore it.
            if (imm_state_ != KinematicState::Coasting)
                imm_pre_coast_ = imm_state_;
            imm_state_ = KinematicState::Coasting;
            return;
        }

        if (stationary_prob_ > kRegimeStationaryProb)
            imm_state_ = KinematicState::Stationary;
        else if (maneuver_prob_ > kRegimeManeuverProb)
            imm_state_ = KinematicState::Maneuvering;
        else
            imm_state_ = KinematicState::Cruising;
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
     * @brief Records a raw observation and refreshes the observation-centric direction.
     *
     * @param obs_rect Raw detection box of the matched observation.
     * @param frame_id Frame in which the observation occurred.
     */
    void Tracklet::recordObservation(const Rect<float>& obs_rect, size_t frame_id)
    {
        last_observation_     = obs_rect;
        has_last_observation_ = true;
        frames_since_observation_ = 0;

        const float cx = obs_rect.x() + obs_rect.width()  * 0.5f;
        const float cy = obs_rect.y() + obs_rect.height() * 0.5f;

        obs_history_.push_back({frame_id, cx, cy});
        while (obs_history_.size() > static_cast<size_t>(ocm_delta_t_) + 1)
            obs_history_.pop_front();

        if (obs_history_.size() < 2)
        {
            obs_velocity_valid_ = false;
            obs_speed_ = 0.0f;
            return;
        }

        // Direction is measured from the oldest sample in the window (raw observations), not from Kalman velocity, so it stays reliable under nonlinear/low-frame-rate motion.
        const ObsSample& prev = obs_history_.front();
        const float dx = cx - prev.cx;
        const float dy = cy - prev.cy;
        const float dist = std::sqrt(dx * dx + dy * dy);
        const size_t frame_gap = (frame_id > prev.frame_id) ? (frame_id - prev.frame_id) : 1;
        obs_speed_ = dist / static_cast<float>(std::max<size_t>(1, frame_gap));

        if (dist > 1e-3f)
        {
            obs_vx_ = dx / dist;
            obs_vy_ = dy / dist;
            obs_velocity_valid_ = true;
        }
        else
        {
            obs_velocity_valid_ = false;
        }
    }

    /**
     * @brief Captures the estimator state at a real observation for ORU rollback.
     *
     * Lock-free: callers either run single-threaded at track birth (activate*)
     * or already hold the state mutex (update/reActivate), matching
     * recordObservation. The Tracklet owns the external mean/covariance; the
     * predictor checkpoints whatever internal state the projection cannot
     * reconstruct (the IMM mode bank), so together they snapshot the whole
     * estimator.
     */
    void Tracklet::captureCheckpoint()
    {
        last_obs_mean_  = mean_;
        last_obs_cov_   = covariance_;
        if (kf_) kf_->saveCheckpoint();
        has_checkpoint_ = true;
    }

    /**
     * @brief Observation-centric Re-Update along the virtual recovery trajectory.
     *
     * @param new_meas Recovery detection in measurement space.
     * @param gap Unobserved frames bridged.
     * @param score Recovery detection confidence.
     * @return True when ORU ran; false to signal the caller to do a plain update.
     */
    bool Tracklet::recoveryReupdate(const KalmanTypes::DetectBox& new_meas, int gap, float score)
    {
        if (!enable_oru_ || !has_checkpoint_ || !kf_ || gap < 2) return false;

        // Roll the estimator back to the last-observation posterior.
        mean_       = last_obs_mean_;
        covariance_ = last_obs_cov_;
        kf_->restoreCheckpoint();

        const KalmanTypes::DetectBox z1 = toMeasurement(last_observation_); // pre-loss observation
        const KalmanTypes::DetectBox z2 = new_meas;                         // recovery detection

        // Spread the accumulated lost time evenly across the gap (collapses to a
        // unit step in frame-counted mode, where toKfDt is constant).
        const float per_frame_dt = accumulated_lost_time_ / static_cast<float>(gap);
        const float step_kf_dt   = timing_policy_ ? timing_policy_->toKfDt(per_frame_dt) : 1.0f;

        // Replay predict+update along the straight line; the final step (i == gap)
        // lands on the real recovery observation z2.
        for (int i = 1; i <= gap; ++i)
        {
            const float t = static_cast<float>(i) / static_cast<float>(gap);
            const KalmanTypes::DetectBox virtual_z = z1 + t * (z2 - z1);
            kf_->predict(mean_, covariance_, step_kf_dt);
            kf_->update(mean_, covariance_, virtual_z, score);
        }
        return true;
    }

    /**
     * @brief Converts a box to the predictor's measurement vector (matches getMeasurement).
     */
    KalmanTypes::DetectBox Tracklet::toMeasurement(const Rect<float>& r) const
    {
        if (enable_xyah_)
        {
            float cx = r.x() + r.width() * 0.5f;
            float cy = r.y() + r.height() * 0.5f;
            float h  = r.height();
            float a  = r.width() / std::max(1e-6f, h);
            KalmanTypes::DetectBox m;
            m << cx, cy, a, h;
            return m;
        }
        return r.getXywh();
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
        return toMeasurement(rect_);
    }

} // namespace SushiTrack
