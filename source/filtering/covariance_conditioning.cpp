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
