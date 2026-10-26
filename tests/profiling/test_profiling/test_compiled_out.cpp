/**************************************************************************/
/* test_compiled_out.cpp                                                  */
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
 * @file test_compiled_out.cpp
 * @brief Tests that every profiling macro expands to nothing when the switch is 0.
 * @author Mustafa Garip
 */

#undef SUSHITRACK_ENABLE_PROFILER
#define SUSHITRACK_ENABLE_PROFILER 0

#include <SushiTrack/profiling/profiling.hpp>

#include <gtest/gtest.h>

namespace
{
    /// @brief Counts how often its argument is evaluated.
    int evaluations = 0;

    /// @brief Returns 1 and counts the call.
    int counted()
    {
        ++evaluations;
        return 1;
    }

    /// @brief Writes every macro once, the way library code does.
    void every_macro()
    {
        SUSHITRACK_ZONE("zone", "Group");
        SUSHITRACK_ZONE_FINE("fine", "Group");
        SUSHITRACK_COUNTER("counter", "units", Set, counted());
        SUSHITRACK_COUNTER_FINE("fine_counter", "units", Add, counted());
        SUSHITRACK_INSTANT("instant");
    }
} // namespace

/**
 * @brief With the switch at 0 the macros compile and leave their value arguments unevaluated.
 */
TEST(Small_ProfilingCompiledOutTest, MacrosExpandToNothingAndEvaluateNoArgument)
{
    evaluations = 0;
    every_macro();
    EXPECT_EQ(evaluations, 0);
}
