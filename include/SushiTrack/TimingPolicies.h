/**
 * @file      TimingPolicies.h
 * @brief     Concrete implementations of timing policies for tracking.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      28.04.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */

#pragma once

#include "SushiTrack/ITimingPolicy.h"
#include <algorithm>

namespace SushiTrack
{
    /**
     * @brief Implements a discrete-step timing policy.
     *
     * Returns a constant delta-time value of 1.0 for all updates. 
     * Track expiration is evaluated using a discrete frame counter 
     * against the configured buffer limit.
     */
    class FrameTimingPolicy final : public ITimingPolicy
    {
        public:
            /**
             * @brief Returns a constant unit step regardless of input.
             * 
             * @param raw_dt Input time interval (unused).
             * @return Constant value of 1.0.
             */
            float toKfDt(float /*raw_dt*/) const override
            {
                return 1.0f;
            }

            /**
             * @brief Evaluates expiration using frame count comparisons.
             * 
             * @param frame_count Number of frames elapsed.
             * @param buffer_frames Configured persistence limit in frames.
             * @return True if the frame count exceeds the limit.
             */
            bool isExpired(float frame_count, int buffer_frames) const override
            {
                return frame_count > static_cast<float>(buffer_frames);
            }
    };

    /**
     * @brief Upper bound for state prediction time steps to limit covariance growth.
     */
    #define MAX_KF_DT_STEPS 5.0f

    /**
     * @brief Implements a real-time aware timing policy.
     *
     * Normalizes real-seconds intervals into frame-equivalent units based on a 
     * configured processing frequency.
     */
    class WallClockTimingPolicy final : public ITimingPolicy
    {
        public:
            /**
             * @brief Initializes the policy with a target frequency.
             * 
             * @param fps Targeted frames-per-second for normalization.
             */
            explicit WallClockTimingPolicy(int fps)
                : fps_(std::max(1, fps)),
                  fps_f_(static_cast<float>(std::max(1, fps)))
            {}

            /**
             * @brief Converts real-time intervals to normalized time steps.
             * 
             * @param raw_dt The elapsed time interval in seconds.
             * @return The normalized time step, capped at MAX_KF_DT_STEPS.
             */
            float toKfDt(float raw_dt) const override
            {
                float kf_dt = raw_dt * fps_f_;

                if (std::abs(kf_dt - 1.0f) < 1e-4f) {
                    kf_dt = 1.0f;
                }
                return std::min(kf_dt, MAX_KF_DT_STEPS);
            }

            /**
             * @brief Evaluates track expiration based on accumulated real-time.
             *
             * The threshold is the buffer expressed in seconds. The comparison uses
             * a strict greater-than to remain symmetric with FrameTimingPolicy
             * (which removes a track at the first frame where the count strictly
             * exceeds the buffer). No half-frame slack is added because the slack
             * shifted removal by one frame relative to the frame-counted path and
             * was a measurable source of divergence between the two timing modes.
             *
             * @param accumulated_seconds Elapsed real-time since the last update.
             * @param buffer_frames Configured persistence threshold in frames.
             * @return True if the real-time interval exceeds the converted threshold.
             */
            bool isExpired(float accumulated_seconds, int buffer_frames) const override
            {
                const float timeout_seconds = static_cast<float>(buffer_frames) / fps_f_;
                return accumulated_seconds > timeout_seconds;
            }

        private:
            int   fps_;
            float fps_f_;
    };

} // namespace SushiTrack
