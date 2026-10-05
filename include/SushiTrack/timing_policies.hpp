/****************************************************************/
/* timing_policies.hpp                                          */
/* SushiTrack - https://github.com/SushiSystems/SushiTrack      */
/* Copyright (c) 2026 Sushi Systems                             */
/* Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.    */
/* Commercial use requires a licence from Sushi Systems.        */
/****************************************************************/

/**
 * @file timing_policies.hpp
 * @brief Concrete implementations of timing policies for tracking.
 * @author Mustafa Garip
 */

#pragma once

#include <SushiTrack/i_timing_policy.hpp>

#include <algorithm>
#include <cmath>

namespace SushiTrack
{
    /**
     * @brief Implements a discrete-step timing policy.
     */
    class FrameTimingPolicy final : public ITimingPolicy
    {
    public:
        /**
         * @brief Returns a constant unit step regardless of input.
         * @param raw_dt Input time interval (unused).
         * @return Constant value of 1.0.
         */
        float toKfDt(float /*raw_dt*/) const override
        {
            return 1.0f;
        }

        /**
         * @brief Evaluates expiration using frame count comparisons.
         * @param frame_count Number of frames elapsed.
         * @param buffer_frames Configured persistence limit in frames.
         * @return True if frame count exceeds limit.
         */
        bool isExpired(float frame_count, int buffer_frames) const override
        {
            return frame_count > static_cast<float>(buffer_frames);
        }
    };

    /**
     * @brief Upper bound for state prediction time steps to limit covariance growth.
     */
    constexpr float kMaxKfDtSteps = 5.0f;

    /**
     * @brief Implements a real-time aware timing policy.
     */
    class WallClockTimingPolicy final : public ITimingPolicy
    {
    public:
        /**
         * @brief Initializes policy with target frequency.
         * @param fps Targeted frames-per-second for normalization.
         */
        explicit WallClockTimingPolicy(int fps)
            : fps_(std::max(1, fps)),
              fps_f_(static_cast<float>(std::max(1, fps)))
        {}

        /**
         * @brief Converts real-time intervals to normalized time steps.
         * @param raw_dt Elapsed time interval in seconds.
         * @return Normalized time step capped at kMaxKfDtSteps.
         */
        float toKfDt(float raw_dt) const override
        {
            float kf_dt = raw_dt * fps_f_;

            if (std::abs(kf_dt - 1.0f) < 1e-4f)
            {
                kf_dt = 1.0f;
            }
            return std::min(kf_dt, kMaxKfDtSteps);
        }

        /**
         * @brief Evaluates track expiration based on accumulated real-time.
         * @param accumulated_seconds Elapsed real-time since last update.
         * @param buffer_frames Configured persistence threshold in frames.
         * @return True if real-time interval exceeds timeout.
         */
        bool isExpired(float accumulated_seconds, int buffer_frames) const override
        {
            const float timeout_seconds = static_cast<float>(buffer_frames) / fps_f_;
            return accumulated_seconds > (timeout_seconds + 1e-4f);
        }

    private:
        int   fps_;
        float fps_f_;
    };

} // namespace SushiTrack
