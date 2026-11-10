/**************************************************************************/
/* profile_loss.cpp                                                       */
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
 * @file profile_loss.cpp
 * @brief Defines account_for_lost_events.
 * @author Mustafa Garip
 */

#include <profile_loss.hpp>

#if SUSHITRACK_ENABLE_PROFILER

#include <SushiTrack/logging/logger.hpp>

#include <mot_engine_log_category.hpp>

namespace MOTEngine
{
    std::uint64_t account_for_lost_events(const SushiProfiler::RunRecord& record,
                                          const std::filesystem::path& path)
    {
        std::uint64_t lost = record.drops.outside_periods + record.drops.open_at_stop;
        for (const SushiProfiler::ThreadRecord& thread : record.threads)
            lost += thread.dropped + thread.late;

        if (lost != 0U)
        {
            SUSHITRACK_LOG_WARN(K_MOT_ENGINE_LOG_CATEGORY,
                                "The profile of {} lost {} events; its timings are incomplete.",
                                path.string(), lost);
        }
        return lost;
    }
} // namespace MOTEngine

#endif
