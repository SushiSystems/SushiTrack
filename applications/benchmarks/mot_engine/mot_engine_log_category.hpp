/**************************************************************************/
/* mot_engine_log_category.hpp                                            */
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
 * @file mot_engine_log_category.hpp
 * @brief Declares K_MOT_ENGINE_LOG_CATEGORY, under which the benchmark runner logs.
 * @author Mustafa Garip
 */

#pragma once

#include <SushiLog/log/log_category.hpp>

#include <SushiTrack/logging/sushitrack_log_project.hpp>

namespace MOTEngine
{
    /** @brief Names the records of the benchmark runner. */
    inline constexpr SushiLog::LogCategory K_MOT_ENGINE_LOG_CATEGORY =
        SushiLog::make_log_category(SushiTrack::K_SUSHITRACK_LOG_PROJECT, "mot_engine");
} // namespace MOTEngine
