/**************************************************************************/
/* i_timing_policy.hpp                                                    */
/**************************************************************************/
/*                          This file is part of:                         */
/*                               SushiTrack                               */
/*              https://github.com/SushiSystems/SushiTrack                */
/*                          https://sushisystems.io                       */
/**************************************************************************/
/* Copyright (c) 2026-present Mustafa Garip & Sushi Systems               */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License");        */
/* you may not use this file except in compliance with the License.       */
/* You may obtain a copy of the License at                                */
/*                                                                        */
/*     http://www.apache.org/licenses/LICENSE-2.0                         */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                          */
/**************************************************************************/

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
