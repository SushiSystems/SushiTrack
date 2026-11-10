/**************************************************************************/
/* profile_loss.hpp                                                       */
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
 * @file profile_loss.hpp
 * @brief Declares account_for_lost_events, which adds up what a profile record lost and warns.
 * @author Mustafa Garip
 * @see docs/design/SUSHITRACK_PROFILING/README.md
 */

#pragma once

#include <SushiTrack/profiling/profile_config.hpp>

#if SUSHITRACK_ENABLE_PROFILER

#include <cstdint>
#include <filesystem>

#include <SushiProfiler/record/run_record.hpp>

namespace MOTEngine
{
    /**
     * @brief Adds up the events @p record lost and logs one WARN naming @p path when any were.
     *
     * The sum holds the events that began outside every period, the zones still open at stop, and
     * for each thread the events its full ring dropped and the late ones.
     *
     * @return The number of events lost.
     */
    std::uint64_t account_for_lost_events(const SushiProfiler::RunRecord& record,
                                          const std::filesystem::path& path);
} // namespace MOTEngine

#endif
