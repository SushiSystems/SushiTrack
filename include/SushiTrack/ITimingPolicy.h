/**
 * @file      ITimingPolicy.h
 * @brief     Interface definition for time-interval normalization and track expiration.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      28.04.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */

#pragma once

namespace SushiTrack
{
    /**
     * @brief Strategy interface for time-interval normalization and track expiration.
     *
     * Provides an abstraction for handling different timing semantics, allowing 
     * the tracker to operate in frame-based or real-time modes.
     */
    class ITimingPolicy
    {
        public:
    /**
     * @brief Virtual destructor for the interface.
     */
    virtual ~ITimingPolicy() = default;

            /**
             * @brief Normalizes a raw time interval for use in state prediction.
             *
             * @param raw_dt Input time interval provided by the caller.
             * @return Normalized interval compatible with the state predictor calibration.
             */
            virtual float toKfDt(float raw_dt) const = 0;

            /**
             * @brief Determines if a track has exceeded its maximum persistence limit.
             *
             * @param accumulated Time interval or frame count elapsed since the last update.
             * @param limit_units Configured maximum persistence threshold in frames.
             * @return True if the track is qualified for removal.
             */
            virtual bool isExpired(float accumulated, int limit_units) const = 0;
    };

} // namespace SushiTrack
