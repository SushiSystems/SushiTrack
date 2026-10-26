/**************************************************************************/
/* tracking_log_category.hpp                                              */
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
 * @file tracking_log_category.hpp
 * @brief Declares K_TRACKING_LOG_CATEGORY, under which the tracking area logs.
 * @author Mustafa Garip
 */

#pragma once

#include <SushiLog/log/log_category.hpp>

#include <SushiTrack/logging/sushitrack_log_project.hpp>

namespace SushiTrack
{
    /** @brief Names the records of the tracking area. */
    inline constexpr SushiLog::LogCategory K_TRACKING_LOG_CATEGORY =
        SushiLog::make_log_category(K_SUSHITRACK_LOG_PROJECT, "tracking");
} // namespace SushiTrack
