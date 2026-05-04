/**
 * @file      TrackFSM.cpp
 * @brief     Implementation of the kinematic state machine for adaptive motion modeling.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      28.04.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */

#include "SushiTrack/TrackFSM.h"
#include <cmath>
#include <algorithm>

namespace SushiTrack
{
    /**
     * @brief Initializes the Kinematic Finite State Machine.
     * 
     * @param config Configuration parameters containing kinematic thresholds.
     */
    TrackFSM::TrackFSM(const TrackerConfig& config)
        : config_(config),
        current_state_(KinematicState::Stationary),
        last_vx_per_sec_(0.0f),
        last_vy_per_sec_(0.0f)
    { }

    /**
     * @brief Updates the kinematic state based on estimated velocity and acceleration.
     * 
     * Analyzes the state vector to determine the object's movement category.
     * Velocity is normalized to units-per-second using the reference frame rate,
     * and acceleration is computed as the derivative of velocity over the 
     * instantaneous time interval. Transitions use a hysteresis margin to 
     * prevent state flickering under detection noise.
     * 
     * @param mean Current state mean vector containing position and velocity.
     * @param is_lost Flag indicating if the tracklet is in the Lost state.
     * @param tracklet_len Number of consecutive frames the tracklet has been active.
     * @param dt Time interval elapsed since the previous update in seconds.
     */
    void TrackFSM::updateState(const KalmanTypes::StateMean& mean, bool is_lost,
                            size_t tracklet_len, float dt)
    {
        if (is_lost)
        {
            current_state_ = KinematicState::Coasting;
            return;
        }

        // 1. ZAMAN BİRİMLERİ (Normalizasyon Hatası Çözümü)
        const float actual_dt = config_.time_aware ? dt : (1.0f / static_cast<float>(std::max(1, config_.frame_rate)));
        const float safe_dt = std::max(actual_dt, 0.001f);
        const float reference_fps = static_cast<float>(std::max(1, config_.frame_rate));

        // Kalman durumu frame bazlıdır, saniyeye çevirmek için referans FPS ile çarpılır.
        const float vx_per_sec = mean(4) * reference_fps;
        const float vy_per_sec = mean(5) * reference_fps;
        const float vel_residual_per_sec = std::sqrt(vx_per_sec * vx_per_sec + vy_per_sec * vy_per_sec);

        // 2. GHOST ACCELERATION (Hayalet İvme) Çözümü
        // Eğer nesne az önce kayıptıysa (Coasting) ve geri döndüyse, geçmiş hız hafızasını 
        // yeni hıza eşitleyerek sahte bir ivme patlamasını engelliyoruz.
        if (current_state_ == KinematicState::Coasting)
        {
            last_vx_per_sec_ = vx_per_sec;
            last_vy_per_sec_ = vy_per_sec;
            current_state_ = KinematicState::Cruising; // Nötr durumdan başlat
            return; 
        }

        // 3. DOĞRU İVME HESABI
        // Saniye cinsinden hız farkı, doğrudan geçen anlık zamana (actual_dt) bölünür.
        const float dvx = vx_per_sec - last_vx_per_sec_;
        const float dvy = vy_per_sec - last_vy_per_sec_;
        const float accel_per_sec2 = std::sqrt(dvx * dvx + dvy * dvy) / safe_dt;

        // Sonraki frame için hızları kaydet
        last_vx_per_sec_ = vx_per_sec;
        last_vy_per_sec_ = vy_per_sec;

        // Isınma (Warmup) süreci
        if (tracklet_len < kWarmupFrames)
        {
            current_state_ = KinematicState::Cruising;
            return;
        }

        // 4. HYSTERESIS (Titreme Önleyici Ölü Bant) Mantığı
        const float margin_down = 0.8f; // Eşiğin %20 altı
        const float margin_up   = 1.2f; // Eşiğin %20 üstü

        if (current_state_ == KinematicState::Stationary)
        {
            if (vel_residual_per_sec > config_.fsm_stationary_vel_thresh * margin_up)
                current_state_ = KinematicState::Cruising;
        }
        else if (current_state_ == KinematicState::Maneuvering)
        {
            if (accel_per_sec2 < config_.fsm_maneuver_accel_thresh * margin_down)
                current_state_ = KinematicState::Cruising;
        }
        else // Mevcut durum: Cruising
        {
            if (vel_residual_per_sec < config_.fsm_stationary_vel_thresh * margin_down)
                current_state_ = KinematicState::Stationary;
            else if (accel_per_sec2 > config_.fsm_maneuver_accel_thresh * margin_up)
                current_state_ = KinematicState::Maneuvering;
        }
    }
    
    /**
     * @brief Adjusts state predictor noise parameters based on the current 
     * kinematic state.
     * 
     * @param kf Pointer to the state predictor instance to be configured.
     * @param dt Time interval for prediction (unused).
     */
    void TrackFSM::applyToPredictor(IStatePredictor* kf, float dt) const
    {
        if (!kf) return;

        (void)dt;

        switch (current_state_)
        {
            case KinematicState::Stationary:
                kf->setNoiseMultipliers(0.8f, 0.6f, 0.5f);
                break;

            case KinematicState::Cruising:
                kf->setNoiseMultipliers(1.0f, 1.0f, 1.0f);
                break;

            case KinematicState::Maneuvering:
                kf->setNoiseMultipliers(1.5f, 1.5f, 2.0f);
                break;

            case KinematicState::Coasting:
                kf->setNoiseMultipliers(1.5f, 1.5f, 1.0f);
                break;
        }
    }

} // namespace SushiTrack
