/**************************************************************************/
/* tracklet.cpp                                                           */
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
 * @file tracklet.cpp
 * @brief Implements the Tracklet class for individual track lifecycle management.
 * @author Mustafa Garip
 */

#include <SushiTrack/tracklet.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>
#include <vector>

#include <SushiTrack/logger.hpp>

namespace SushiTrack
{
    /**
     * @brief Performs L2 normalization on a feature vector.
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
     * @param kf State predictor instance (Kalman Filter).
     * @param rect Initial geometric bounding box.
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
     * @param new_track Observed detection data wrapped in a tracklet container.
     * @param frame_id Current frame identifier.
     * @param new_track_id New tracking identifier (optional).
     */
    void Tracklet::reActivate(const Tracklet& new_track, size_t frame_id, int new_track_id)
    {
        const KalmanTypes::DetectBox new_meas = new_track.getMeasurement();

        // Hold state mutex for mutation so readers never see torn mean or covariance.
        std::lock_guard<std::mutex> lock(state_mutex_);

        // Bridge unobserved gap along virtual trajectory; fall back on small gaps.
        const int gap = frames_since_observation_;
        if (!recoveryReupdate(new_meas, gap, new_track.getScore()))
        {
            kf_->update(mean_, covariance_, new_meas, new_track.getScore());
        }

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
     * @param dt Time interval for prediction.
     */
    void Tracklet::predict(float dt)
    {
        std::lock_guard<std::mutex> lock(state_mutex_);

        last_dt_ = dt;
        frames_since_observation_++;

        if (state_ != TrackState::Tracked)
        {
            // Unobserved track: freeze size velocities while preserving position velocity.
            if (enable_xyah_)
            {
                mean_[7] = 0.f;
            }
            else
            {
                mean_[6] = 0.f;
                mean_[7] = 0.f;
            }
        }

        const float kf_dt = timing_policy_ ? timing_policy_->toKfDt(dt) : 1.0f;
        kf_->predict(mean_, covariance_, kf_dt);
        updateRect();

        // Drive the IMM regime into Coasting on an unobserved frame.
        if (state_ == TrackState::Lost)
        {
            refreshRegime(true);
        }

        if (state_ == TrackState::Lost || state_ == TrackState::Tentative)
        {
            accumulated_lost_time_ += dt;
        }
    }

    /**
     * @brief Updates the tracklet state with a matched observation.
     * @param new_track Matched detection data.
     * @param frame_id Current frame identifier.
     * @param update_features Flag for updating visual features with observation.
     * @param keep_state When true, suppresses the lifecycle flag transition.
     */
    void Tracklet::update(const Tracklet& new_track, size_t frame_id,
                          bool update_features, bool keep_state)
    {
        const KalmanTypes::DetectBox new_meas = new_track.getMeasurement();

        std::lock_guard<std::mutex> lock(state_mutex_);
        kf_->update(mean_, covariance_, new_meas, new_track.getScore());
        updateRect();

        if (!keep_state)
        {
            state_ = TrackState::Tracked;
            is_activated_ = true;
            is_confirmed_ = true;
        }

        score_ = new_track.getScore();
        score_smooth_ = kScoreSmoothingMomentum * score_smooth_ + (1.f - kScoreSmoothingMomentum) * score_;
        label_ = new_track.getLabel();

        if (update_features && !new_track.getCurrFeat().empty())
        {
            this->updateFeatures(new_track.getCurrFeat());
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
     * @brief Retrieves the current state mean vector.
     * @return State mean vector.
     */
    KalmanTypes::StateMean Tracklet::getMean() const 
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        return mean_;
    }

    /**
     * @brief Retrieves the current state covariance matrix.
     * @return State covariance matrix.
     */
    KalmanTypes::StateCov Tracklet::getCovariance() const 
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        return covariance_;
    }

    /**
     * @brief Marks the tracklet as lost.
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
     * @param is_lost True when the track currently has no observation.
     */
    void Tracklet::refreshRegime(bool is_lost)
    {
        if (!kf_) return;
        const float m = kf_->maneuverProbability();
        if (m < 0.0f) return;

        maneuver_prob_   = m;
        stationary_prob_ = kf_->stationaryProbability();

        if (is_lost)
        {
            if (imm_state_ != KinematicState::Coasting)
            {
                imm_pre_coast_ = imm_state_;
            }
            imm_state_ = KinematicState::Coasting;
            return;
        }

        // Near-zero speed relative to box height reads Stationary.
        const float vx = mean_(4);
        const float vy = mean_(5);
        const float height = std::max(1e-2f, mean_(3));
        const float speed_frac = std::sqrt(vx * vx + vy * vy) / height;

        if (stationary_prob_ > kRegimeStationaryProb || speed_frac < kRegimeStationarySpeedFrac)
        {
            imm_state_ = KinematicState::Stationary;
        }
        else if (maneuver_prob_ > kRegimeManeuverProb)
        {
            imm_state_ = KinematicState::Maneuvering;
        }
        else
        {
            imm_state_ = KinematicState::Cruising;
        }
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
     * @param feat New observation feature vector.
     */
    void Tracklet::updateFeatures(std::vector<float> feat) 
    {
        curr_feat_ = std::move(feat);
        normalizeL2(curr_feat_);

        addFeatureToHistory(curr_feat_);

        if (smooth_feat_.empty() || smooth_feat_.size() != curr_feat_.size())
        {
            smooth_feat_ = curr_feat_;
        }
        else
        { 
            for (size_t i = 0; i < smooth_feat_.size(); ++i)
            {
                smooth_feat_[i] = alpha_ * smooth_feat_[i] + (1.f - alpha_) * curr_feat_[i];
            }
            normalizeL2(smooth_feat_);
        }
    }

    /**
     * @brief Computes maximum cosine similarity against the feature history.
     * @param detection_feat Input feature vector for comparison.
     * @return Maximum similarity score in the range [0, 1].
     */
    float Tracklet::getMaxFeatureSimilarity(const std::vector<float>& detection_feat) const
    {
        if (feature_history_.empty() && smooth_feat_.empty()) return 1.0f;

        float max_similarity = 0.0f;

        for (const auto& hist_feat : feature_history_) 
        {
            if (hist_feat.size() != detection_feat.size()) continue;

            float dot = 0.0f;
            for (size_t i = 0; i < hist_feat.size(); ++i)
            {
                dot += hist_feat[i] * detection_feat[i];
            }

            max_similarity = std::max(max_similarity, dot);
        }

        if (!smooth_feat_.empty() && smooth_feat_.size() == detection_feat.size()) 
        {
            float dot = 0.0f;
            for (size_t i = 0; i < smooth_feat_.size(); ++i)
            {
                dot += smooth_feat_[i] * detection_feat[i];
            }
            max_similarity = std::max(max_similarity, dot);
        }

        return max_similarity;
    }

    /**
     * @brief Records a raw observation and refreshes the observation-centric direction.
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
        {
            obs_history_.pop_front();
        }

        if (obs_history_.size() < 2)
        {
            obs_velocity_valid_ = false;
            obs_speed_ = 0.0f;
            return;
        }

        // Direction is measured from the oldest sample in the window.
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
     * @brief Captures the estimator state at an observation for ORU rollback.
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
     * @param new_meas Recovery detection in measurement space.
     * @param gap Unobserved frames bridged.
     * @param score Recovery detection confidence.
     * @return True when ORU ran; false to signal the caller to do a plain update.
     */
    bool Tracklet::recoveryReupdate(const KalmanTypes::DetectBox& new_meas, int gap, float score)
    {
        if (!enable_oru_ || !has_checkpoint_ || !kf_ || gap < 2) return false;

        mean_       = last_obs_mean_;
        covariance_ = last_obs_cov_;
        kf_->restoreCheckpoint();

        const KalmanTypes::DetectBox z1 = toMeasurement(last_observation_);
        const KalmanTypes::DetectBox z2 = new_meas;

        // Spread the accumulated lost time evenly across the unobserved gap.
        const float per_frame_dt = accumulated_lost_time_ / static_cast<float>(gap);
        const float step_kf_dt   = timing_policy_ ? timing_policy_->toKfDt(per_frame_dt) : 1.0f;

        // Replay predict and update steps along the virtual linear path.
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
     * @brief Converts a box to the predictor's measurement vector.
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
     * @return Measurement vector in the configured coordinate format.
     */
    KalmanTypes::DetectBox Tracklet::getMeasurement() const
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        return toMeasurement(rect_);
    }

} // namespace SushiTrack
