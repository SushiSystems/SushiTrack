/**************************************************************************/
/* wall_clock_timing_policy.hpp                                           */
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
/**************************************************************************/

/**
 * @file wall_clock_timing_policy.hpp
 * @brief Declares the timing policy that measures elapsed seconds.
 * @author Mustafa Garip
 */

#pragma once

#include <algorithm>
#include <cmath>

#include <SushiTrack/tracking/timing_policy.hpp>

namespace SushiTrack
{
    /**
     * @brief Upper bound for state prediction time steps to limit covariance growth.
     */
    constexpr float K_MAX_KF_DT_STEPS = 5.0f;

    /**
     * @brief Implements a real-time aware timing policy.
     */
    class WallClockTimingPolicy final : public TimingPolicy
    {
        public:
            /**
             * @brief Initializes policy with target frequency.
             * @param fps Targeted frames-per-second for normalization.
             */
            explicit WallClockTimingPolicy(int fps)
                : fps_(std::max(1, fps)), fps_f_(static_cast<float>(std::max(1, fps)))
            {
            }

            /**
             * @brief Converts real-time intervals to normalized time steps.
             * @param raw_dt Elapsed time interval in seconds.
             * @return Normalized time step capped at K_MAX_KF_DT_STEPS.
             */
            [[nodiscard]] float to_kf_dt(float raw_dt) const override
            {
                float kf_dt = raw_dt * fps_f_;

                if (std::abs(kf_dt - 1.0f) < 1e-4f)
                {
                    kf_dt = 1.0f;
                }
                return std::min(kf_dt, K_MAX_KF_DT_STEPS);
            }

            /**
             * @brief Evaluates track expiration based on accumulated real-time.
             * @param accumulated_seconds Elapsed real-time since last update.
             * @param buffer_frames Configured persistence threshold in frames.
             * @return True if real-time interval exceeds timeout.
             */
            [[nodiscard]] bool is_expired(float accumulated_seconds,
                                          int buffer_frames) const override
            {
                const float timeout_seconds = static_cast<float>(buffer_frames) / fps_f_;
                return accumulated_seconds > (timeout_seconds + 1e-4f);
            }

        private:
            int fps_;
            float fps_f_;
    };
} // namespace SushiTrack
