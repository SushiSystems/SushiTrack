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
 * @file profiling.hpp
 * @brief Declares the SUSHITRACK_ZONE, SUSHITRACK_COUNTER and SUSHITRACK_INSTANT macros.
 * @author Mustafa Garip
 * @see docs/design/SUSHITRACK_PROFILING/README.md
 */

#pragma once

/// @brief Whether this translation unit records through SushiProfiler; 0 compiles every macro out.
#ifndef SUSHITRACK_ENABLE_PROFILER
#define SUSHITRACK_ENABLE_PROFILER 1
#endif

/// @brief The most detailed level compiled in: 1 stages and counters, 2 adds fine zones.
#ifndef SUSHITRACK_PROFILE_LEVEL
#ifdef NDEBUG
#define SUSHITRACK_PROFILE_LEVEL 1
#else
#define SUSHITRACK_PROFILE_LEVEL 2
#endif
#endif

#if SUSHITRACK_ENABLE_PROFILER

#include <SushiProfiler/zones/macros.hpp>

/// @brief Records the rest of the scope as a stage zone of @p group, level 1.
#define SUSHITRACK_ZONE(name, group) SP_ZONE_AT(SUSHITRACK_PROFILE_LEVEL, 1, name, group)

/// @brief Records the rest of the scope as a fine zone of @p group, level 2.
#define SUSHITRACK_ZONE_FINE(name, group) SP_ZONE_AT(SUSHITRACK_PROFILE_LEVEL, 2, name, group)

/// @brief Records @p value at a counter folded by CounterOp @p op (Add, Set, Max, Min), level 1.
#define SUSHITRACK_COUNTER(name, unit, op, value)                                                  \
    SP_COUNTER_AT(SUSHITRACK_PROFILE_LEVEL, 1, name, unit, op, static_cast<double>(value))

/// @brief Records @p value at a counter folded by CounterOp @p op, level 2.
#define SUSHITRACK_COUNTER_FINE(name, unit, op, value)                                             \
    SP_COUNTER_AT(SUSHITRACK_PROFILE_LEVEL, 2, name, unit, op, static_cast<double>(value))

/// @brief Records an occurrence now, level 2.
#define SUSHITRACK_INSTANT(name) SP_INSTANT_AT(SUSHITRACK_PROFILE_LEVEL, 2, name)

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
