/**************************************************************************/
/* sushitrack_profiler_parts.cpp                                          */
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
 * @file sushitrack_profiler_parts.cpp
 * @brief Implements the environment contributor and the registration of SushiTrack's parts.
 * @author Mustafa Garip
 */

#include <SushiTrack/profiling/sushitrack_profiler_parts.hpp>

#include <SushiTrack/profiling/profile_config.hpp>

#if SUSHITRACK_ENABLE_PROFILER

#include <SushiProfiler/environment/environment_contributor.hpp>
#include <SushiProfiler/registry/registry.hpp>

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>

#include <SushiTrack/profiling/profiler_keys.hpp>

/// @brief The version the build declared, or `unknown` when the build declared none.
#ifndef SUSHITRACK_VERSION_STRING
#define SUSHITRACK_VERSION_STRING "unknown"
#endif

/// @brief The commit the build declared, or `unknown` when the build declared none.
#ifndef SUSHITRACK_COMMIT
#define SUSHITRACK_COMMIT "unknown"
#endif

namespace SushiTrack
{
    namespace
    {
        /// @brief Returns @p time as `YYYY-MM-DDTHH:MM:SSZ` in UTC.
        std::string utc_text(std::chrono::system_clock::time_point time)
        {
            const std::int64_t seconds =
                std::chrono::duration_cast<std::chrono::seconds>(time.time_since_epoch()).count();
            std::int64_t days = seconds / 86400;
            std::int64_t second_of_day = seconds % 86400;
            if (second_of_day < 0)
            {
                second_of_day += 86400;
                --days;
            }

            days += 719468;
            const std::int64_t era = (days >= 0 ? days : days - 146096) / 146097;
            const std::int64_t day_of_era = days - era * 146097;
            const std::int64_t year_of_era =
                (day_of_era - day_of_era / 1460 + day_of_era / 36524 - day_of_era / 146096) / 365;
            const std::int64_t day_of_year =
                day_of_era - (365 * year_of_era + year_of_era / 4 - year_of_era / 100);
            const std::int64_t shifted_month = (5 * day_of_year + 2) / 153;
            const std::int64_t day = day_of_year - (153 * shifted_month + 2) / 5 + 1;
            const std::int64_t month = shifted_month < 10 ? shifted_month + 3 : shifted_month - 9;
            const std::int64_t year = year_of_era + era * 400 + (month <= 2 ? 1 : 0);

            char text[32];
            std::snprintf(text, sizeof(text), "%04lld-%02lld-%02lldT%02lld:%02lld:%02lldZ",
                          static_cast<long long>(year), static_cast<long long>(month),
                          static_cast<long long>(day),
                          static_cast<long long>(second_of_day / 3600),
                          static_cast<long long>(second_of_day % 3600 / 60),
                          static_cast<long long>(second_of_day % 60));
            return text;
        }

        /// @brief Adds the library's name, version, commit, profile level and the run's UTC date
        ///        to a record's environment.
        class SushiTrackEnvironment final : public SushiProfiler::EnvironmentContributor
        {
            public:
                /// @brief Sets the `sushitrack.*` facts of ProfilerKeys in @p facts.
                void contribute(SushiProfiler::RecordFacts& facts) const override;
        };

        void SushiTrackEnvironment::contribute(SushiProfiler::RecordFacts& facts) const
        {
            SushiProfiler::set_fact(facts.environment, ProfilerKeys::K_FACT_LIBRARY, "SushiTrack");
            SushiProfiler::set_fact(facts.environment, ProfilerKeys::K_FACT_VERSION,
                                    SUSHITRACK_VERSION_STRING);
            SushiProfiler::set_fact(facts.environment, ProfilerKeys::K_FACT_COMMIT,
                                    SUSHITRACK_COMMIT);
            SushiProfiler::set_fact(facts.environment, ProfilerKeys::K_FACT_DATE,
                                    utc_text(std::chrono::system_clock::now()));
            SushiProfiler::set_fact(facts.environment, ProfilerKeys::K_FACT_PROFILE_LEVEL,
                                    std::to_string(SUSHITRACK_PROFILE_LEVEL));
        }
    } // namespace

    struct SushiTrackProfilerParts::State
    {
        SushiTrackEnvironment environment; /**< The contributor the registry refers to. */
    };

    SushiTrackProfilerParts::SushiTrackProfilerParts(SushiProfiler::Registry& registry)
        : state_(std::make_unique<State>())
    {
        registry.add_contributor(state_->environment);
    }

    SushiTrackProfilerParts::~SushiTrackProfilerParts() = default;

    SushiTrackProfilerParts register_profiler_parts(SushiProfiler::Registry& registry)
    {
        return SushiTrackProfilerParts(registry);
    }
} // namespace SushiTrack

#else

namespace SushiTrack
{
    struct SushiTrackProfilerParts::State
    {
    };

    SushiTrackProfilerParts::SushiTrackProfilerParts(SushiProfiler::Registry&)
    {
    }

    SushiTrackProfilerParts::~SushiTrackProfilerParts() = default;

    SushiTrackProfilerParts register_profiler_parts(SushiProfiler::Registry& registry)
    {
        return SushiTrackProfilerParts(registry);
    }
} // namespace SushiTrack

#endif
