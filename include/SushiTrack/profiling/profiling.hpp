/**************************************************************************/
/* profiling.hpp                                                          */
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
 * @file profiling.hpp
 * @brief Declares the SUSHITRACK_ZONE, SUSHITRACK_COUNTER and SUSHITRACK_INSTANT macros.
 * @author Mustafa Garip
 * @see docs/design/SUSHITRACK_PROFILING/README.md
 */

#pragma once

// The generated config, which sushitrack_link_profiling puts on the path, defines the switch.
#include <SushiTrack/profiling/profile_config.hpp>

#if SUSHITRACK_ENABLE_PROFILER

#include <SushiProfiler/zones/macros.hpp>

#include <SushiTrack/profiling/profile_sink.hpp>

/// @brief Tells the sink installed on this thread, if any, that a zone was entered.
#define SUSHITRACK_DETAIL_SINK_ZONE(level, name, group)                                            \
    do                                                                                             \
    {                                                                                              \
        if constexpr ((level) <= SUSHITRACK_PROFILE_LEVEL)                                         \
        {                                                                                          \
            if (::SushiTrack::ProfileSink* sushitrack_sink = ::SushiTrack::active_profile_sink())  \
                sushitrack_sink->zone(name, group);                                                \
        }                                                                                          \
    } while (false)

/// @brief Records one counter sample at SushiProfiler and at the sink installed on this thread.
#define SUSHITRACK_DETAIL_COUNTER(level, name, unit, op, value)                                    \
    do                                                                                             \
    {                                                                                              \
        if constexpr ((level) <= SUSHITRACK_PROFILE_LEVEL)                                         \
        {                                                                                          \
            const double sushitrack_sample = static_cast<double>(value);                           \
            if (::SushiTrack::ProfileSink* sushitrack_sink = ::SushiTrack::active_profile_sink())  \
                sushitrack_sink->counter(name, ::SushiTrack::ProfileCounterOp::op,                 \
                                         sushitrack_sample);                                       \
            SP_COUNTER_AT(SUSHITRACK_PROFILE_LEVEL, level, name, unit, op, sushitrack_sample);     \
        }                                                                                          \
    } while (false)

/// @brief Records one instant at SushiProfiler and at the sink installed on this thread.
#define SUSHITRACK_DETAIL_INSTANT(level, name)                                                     \
    do                                                                                             \
    {                                                                                              \
        if constexpr ((level) <= SUSHITRACK_PROFILE_LEVEL)                                         \
        {                                                                                          \
            if (::SushiTrack::ProfileSink* sushitrack_sink = ::SushiTrack::active_profile_sink())  \
                sushitrack_sink->instant(name);                                                    \
            SP_INSTANT_AT(SUSHITRACK_PROFILE_LEVEL, level, name);                                  \
        }                                                                                          \
    } while (false)

/// @brief Records the rest of the scope as a stage zone of @p group, level 1.
#define SUSHITRACK_ZONE(name, group)                                                               \
    SUSHITRACK_DETAIL_SINK_ZONE(1, name, group);                                                   \
    SP_ZONE_AT(SUSHITRACK_PROFILE_LEVEL, 1, name, group)

/// @brief Records the rest of the scope as a fine zone of @p group, level 2.
#define SUSHITRACK_ZONE_FINE(name, group)                                                          \
    SUSHITRACK_DETAIL_SINK_ZONE(2, name, group);                                                   \
    SP_ZONE_AT(SUSHITRACK_PROFILE_LEVEL, 2, name, group)

/// @brief Records @p value at a counter folded by CounterOp @p op (Add, Set, Max, Min), level 1.
#define SUSHITRACK_COUNTER(name, unit, op, value)                                                  \
    SUSHITRACK_DETAIL_COUNTER(1, name, unit, op, value)

/// @brief Records @p value at a counter folded by CounterOp @p op, level 2.
#define SUSHITRACK_COUNTER_FINE(name, unit, op, value)                                             \
    SUSHITRACK_DETAIL_COUNTER(2, name, unit, op, value)

/// @brief Records an occurrence now, level 2.
#define SUSHITRACK_INSTANT(name) SUSHITRACK_DETAIL_INSTANT(2, name)

#else

/// @brief Expands to nothing: the profiler is compiled out of this translation unit.
#define SUSHITRACK_ZONE(name, group) static_assert(true, "profiler compiled out")

/// @brief Expands to nothing: the profiler is compiled out of this translation unit.
#define SUSHITRACK_ZONE_FINE(name, group) static_assert(true, "profiler compiled out")

/// @brief Expands to nothing and leaves @p value unevaluated.
#define SUSHITRACK_COUNTER(name, unit, op, value) static_assert(true, "profiler compiled out")

/// @brief Expands to nothing and leaves @p value unevaluated.
#define SUSHITRACK_COUNTER_FINE(name, unit, op, value)                                             \
    static_assert(true, "profiler compiled out")

/// @brief Expands to nothing: the profiler is compiled out of this translation unit.
#define SUSHITRACK_INSTANT(name) static_assert(true, "profiler compiled out")

#endif
