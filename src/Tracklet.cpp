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
        is_confirmed_ = (track_id != 0);
        track_id_ = track_id;
        frame_id_ = frame_id;
        start_frame_id_ = frame_id;
        tracklet_len_ = 1;
        hits_ = 1;

        recordObservation(initial_meas, frame_id);
        saveStateSnapshot(frame_id);
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

        recordObservation(initial_meas, frame_id);
        saveStateSnapshot(frame_id);
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
     * When Observation-Centric Online Smoothing is enabled and a saved
     * predictor snapshot from the last successful observation is available,
     * the predictor is rolled back to that snapshot and a sequence of virtual
     * measurements interpolated between the saved observation and the current
     * one is replayed. The replay applies one Kalman predict-update cycle per
     * lost frame, which corrects the trajectory drift accumulated during the
     * unobserved period and prevents the recovered box from snapping to the
     * predictor's free-running estimate.
     *
     * @param new_track Observed detection data wrapped in a tracklet container.
     * @param frame_id Current frame identifier.
     * @param new_track_id New tracking identifier (optional).
     * @param dt_per_step Per-step time interval used by the smoothing replay.
     */
    void Tracklet::reActivate(const Tracklet& new_track, size_t frame_id, int new_track_id, float dt_per_step)
    {
        const KalmanTypes::DetectBox new_meas = new_track.getMeasurement();

        if (enable_oos_ && has_saved_state_ && frame_id > saved_frame_id_)
        {
            mean_ = saved_mean_;
            covariance_ = saved_covariance_;

            const size_t time_gap = frame_id - saved_frame_id_;
            const KalmanTypes::DetectBox start_meas = last_observation_;

            const float kf_dt = timing_policy_ ? timing_policy_->toKfDt(dt_per_step) : 1.0f;

            for (size_t i = 1; i < time_gap; ++i)
            {
                const float alpha = static_cast<float>(i) / static_cast<float>(time_gap);
                KalmanTypes::DetectBox virt = (1.0f - alpha) * start_meas + alpha * new_meas;

                kf_->predict(mean_, covariance_, kf_dt);
                kf_->update(mean_, covariance_, virt, new_track.getScore());
            }

            kf_->predict(mean_, covariance_, kf_dt);
            kf_->update(mean_, covariance_, new_meas, new_track.getScore());
        }
        else
        {
            kf_->update(mean_, covariance_, new_meas, new_track.getScore());
        }

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
        lost_frames_ = 0;

        recordObservation(new_meas, frame_id);
        saveStateSnapshot(frame_id);
    }

    /**
     * @brief Predicts the next state of the tracklet.
     * 
     * @param dt Time interval for prediction.
     */
    void Tracklet::predict(float dt) 
    {
        if (state_ != TrackState::Tracked) 
        {
            mean_[6] = 0.f;
            mean_[7] = 0.f;
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
            lost_frames_++;
        }
    }

    /**
     * @brief Updates the tracklet state with a matched observation.
     *
     * Records the raw measurement in the bounded observation history and
     * captures a snapshot of the predictor distribution. The history is used
     * by the Observation-Centric Momentum cost decorator to estimate the
     * track's recent direction of motion. The snapshot is consumed by the
     * Observation-Centric Online Smoothing replay during reActivate and is
     * always maintained so that enabling smoothing is a runtime decision.
     *
     * @param new_track Matched detection data.
     * @param frame_id Current frame identifier.
     * @param update_features Flag for updating visual features with the observation.
     */
    void Tracklet::update(const Tracklet& new_track, size_t frame_id, bool update_features)
    {
        const KalmanTypes::DetectBox new_meas = new_track.getMeasurement();

        kf_->update(mean_, covariance_, new_meas, new_track.getScore());
        updateRect();

        score_ = new_track.getScore();
        label_ = new_track.getLabel();

        if (update_features && !new_track.getCurrFeat().empty())
        {
            updateFeatures(new_track.getCurrFeat());
        }

        state_ = TrackState::Tracked;
        is_activated_ = true;
        is_confirmed_ = true;
        frame_id_ = frame_id;
        hits_++;
        tracklet_len_ = frame_id - start_frame_id_ + 1;
        accumulated_lost_time_ = 0.0f;
        lost_frames_ = 0;

        recordObservation(new_meas, frame_id);
        saveStateSnapshot(frame_id);
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

    /**
     * @brief Records a raw observation in the bounded history buffer and
     *        refreshes the last-observation snapshot.
     *
     * The buffer is trimmed from the front so that no more than
     * max_observation_history_ entries are retained.
     *
     * @param meas Measurement vector applied to the predictor.
     * @param frame_id Frame identifier associated with the measurement.
     */
    void Tracklet::recordObservation(const KalmanTypes::DetectBox& meas, size_t frame_id)
    {
        last_observation_ = meas;
        last_observation_frame_id_ = frame_id;
        has_observation_ = true;

        observation_history_.emplace_back(frame_id, meas);
        while (observation_history_.size() > max_observation_history_)
        {
            observation_history_.pop_front();
        }
    }

    /**
     * @brief Captures the predictor distribution at the moment of the last
     *        successful observation.
     *
     * The snapshot is restored by reActivate when Observation-Centric Online
     * Smoothing is enabled.
     *
     * @param frame_id Frame identifier of the snapshot.
     */
    void Tracklet::saveStateSnapshot(size_t frame_id)
    {
        saved_mean_ = mean_;
        saved_covariance_ = covariance_;
        saved_frame_id_ = frame_id;
        has_saved_state_ = true;
    }

    /**
     * @brief Configures the maximum capacity of the observation history buffer.
     *
     * @param n Maximum number of observations retained. Values smaller than
     *          two are clamped to two so a velocity direction can always be
     *          computed when at least two observations exist.
     */
    void Tracklet::setMaxObservationHistory(int n)
    {
        if (n < 2) n = 2;
        max_observation_history_ = static_cast<size_t>(n);
        while (observation_history_.size() > max_observation_history_)
        {
            observation_history_.pop_front();
        }
    }

    /**
     * @brief Computes the unit-direction vector from an earlier observation
     *        to the most recent one.
     *
     * The earlier observation is selected as the most recent entry whose
     * frame separation from the latest observation is at least delta_t. This
     * provides a velocity-direction estimate that is robust to single-frame
     * jitter and short observation gaps.
     *
     * @param delta_t Minimum frame separation between the reference observation
     *                and the latest one.
     * @param out_direction Output unit-direction vector. Set to the zero
     *                      vector when no valid direction can be computed.
     * @return True when a valid direction was produced.
     */
    bool Tracklet::getObservationDirection(int delta_t, Eigen::Matrix<float, 1, 2>& out_direction) const
    {
        out_direction.setZero();

        if (observation_history_.size() < 2 || !has_observation_)
        {
            return false;
        }

        const size_t latest_frame = last_observation_frame_id_;
        const float latest_cx = last_observation_(0);
        const float latest_cy = last_observation_(1);

        const auto& history = observation_history_;
        for (auto it = history.rbegin() + 1; it != history.rend(); ++it)
        {
            const size_t frame_id = it->first;
            const long gap = static_cast<long>(latest_frame) - static_cast<long>(frame_id);
            if (gap < delta_t) continue;

            const float ref_cx = it->second(0);
            const float ref_cy = it->second(1);

            const float dx = latest_cx - ref_cx;
            const float dy = latest_cy - ref_cy;
            const float norm = std::sqrt(dx * dx + dy * dy);
            if (norm < 1e-6f) return false;

            out_direction(0, 0) = dx / norm;
            out_direction(0, 1) = dy / norm;
            return true;
        }

        return false;
    }

} // namespace SushiTrack
