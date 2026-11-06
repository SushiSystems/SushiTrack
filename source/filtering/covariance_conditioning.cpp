/**************************************************************************/
/* covariance_conditioning.cpp                                            */
/**************************************************************************/
/*                          This file is part of:                         */
/*                               SushiTrack                               */
/*               https://github.com/SushiSystems/SushiTrack               */
/*                         https://sushisystems.io                        */
/**************************************************************************/
/* Copyright (c) 2026-present Mustafa Garip & Sushi Systems               */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License"); you    */
/* may not use this file except in compliance with the License. You may   */
/* obtain a copy of the License at                                        */
/*                                                                        */
/*     https://www.apache.org/licenses/LICENSE-2.0                        */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                         */
/**************************************************************************/

/**
 * @file covariance_conditioning.cpp
 * @brief Implements report_regularized_covariance with one throttle per filter.
 * @author Mustafa Garip
 */

#include <filtering/covariance_conditioning.hpp>

#include <SushiTrack/filtering/filtering_log_category.hpp>
#include <SushiTrack/logging/log_throttle.hpp>
#include <SushiTrack/logging/logger.hpp>

namespace SushiTrack::Detail
{
    void report_regularized_covariance(ConditionedFilter filter, const char* step)
    {
        static LogThrottle kalman_throttle;
        static LogThrottle imm_throttle;
        if (filter == ConditionedFilter::Kalman)
        {
            SUSHITRACK_LOG_WARN_LIMITED(K_FILTERING_LOG_CATEGORY, kalman_throttle,
                                        "Kalman filter {}: covariance failed to factor; "
                                        "regularising it with 1e-6 on the diagonal.",
                                        step);
        }
        else
        {
            SUSHITRACK_LOG_WARN_LIMITED(K_FILTERING_LOG_CATEGORY, imm_throttle,
                                        "IMM filter {}: covariance failed to factor; "
                                        "regularising it with 1e-6 on the diagonal.",
                                        step);
        }
    }
} // namespace SushiTrack::Detail
