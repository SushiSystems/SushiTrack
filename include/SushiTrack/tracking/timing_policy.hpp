/**************************************************************************/
/* timing_policy.hpp                                                      */
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
 * @file timing_policy.hpp
 * @brief Interface definition for time-interval normalization and track expiration.
 * @author Mustafa Garip
 */

#pragma once

namespace SushiTrack
{
    /**
     * @brief Strategy interface for time-interval normalization and track expiration.
     */
    class TimingPolicy
    {
        public:
            /**
             * @brief Virtual destructor for the interface.
             */
            virtual ~TimingPolicy() = default;

            /**
             * @brief Normalizes a raw time interval for use in state prediction.
             * @param raw_dt Input time interval provided by the caller.
             * @return Normalized interval compatible with state predictor calibration.
             */
            [[nodiscard]] virtual float to_kf_dt(float raw_dt) const = 0;

            /**
             * @brief Determines if a track has exceeded its maximum persistence limit.
             * @param accumulated Time interval or frame count elapsed since last update.
             * @param limit_units Configured maximum persistence threshold in frames.
             * @return True if the track is qualified for removal.
             */
            [[nodiscard]] virtual bool is_expired(float accumulated, int limit_units) const = 0;
    };

} // namespace SushiTrack
