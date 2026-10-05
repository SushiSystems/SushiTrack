/****************************************************************/
/* i_timing_policy.hpp                                          */
/* SushiTrack - https://github.com/SushiSystems/SushiTrack      */
/* Copyright (c) 2026 Sushi Systems                             */
/* Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.    */
/* Commercial use requires a licence from Sushi Systems.        */
/****************************************************************/

/**
 * @file i_timing_policy.hpp
 * @brief Interface definition for time-interval normalization and track expiration.
 * @author Mustafa Garip
 */

#pragma once

namespace SushiTrack
{
    /**
     * @brief Strategy interface for time-interval normalization and track expiration.
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
         * @param raw_dt Input time interval provided by the caller.
         * @return Normalized interval compatible with state predictor calibration.
         */
        virtual float toKfDt(float raw_dt) const = 0;

        /**
         * @brief Determines if a track has exceeded its maximum persistence limit.
         * @param accumulated Time interval or frame count elapsed since last update.
         * @param limit_units Configured maximum persistence threshold in frames.
         * @return True if the track is qualified for removal.
         */
        virtual bool isExpired(float accumulated, int limit_units) const = 0;
    };

} // namespace SushiTrack
