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
 * @file test_compiled_out.cpp
 * @brief Tests that every profiling macro expands to nothing when the switch is 0.
 * @author Mustafa Garip
 */

// The generated config comes first, and this file then sees a build with the profiler off.
#include <SushiTrack/profiling/profile_config.hpp>

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
