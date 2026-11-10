/**************************************************************************/
/* test_compiled_in.cpp                                                   */
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
 * @file test_compiled_in.cpp
 * @brief Tests that the SUSHITRACK_ZONE and SUSHITRACK_COUNTER macros record under a session.
 * @author Mustafa Garip
 */

#include <SushiTrack/profiling/profiling.hpp>

#if SUSHITRACK_ENABLE_PROFILER

#include <gtest/gtest.h>

#include <profile_session.hpp>

using namespace SushiTrack;

namespace
{
    /// @brief Runs one zone and one counter the way library code writes them.
    void recorded_work(int items)
    {
        SUSHITRACK_ZONE("test_stage", ProfilerKeys::K_GROUP_TRACK);
        SUSHITRACK_COUNTER("test_items", ProfilerKeys::K_UNIT_TRACKS, Add, items);
        SUSHITRACK_COUNTER("test_items", ProfilerKeys::K_UNIT_TRACKS, Add, items);
    }

    /// @brief Records @p value at one Set counter site.
    void set_level(int value)
    {
        SUSHITRACK_COUNTER("test_level", ProfilerKeys::K_UNIT_TRACKS, Set, value);
    }
} // namespace

/**
 * @brief A zone recorded under an open session appears in the record under its group.
 */
TEST(Medium_ProfilingCompiledInTest, ZoneAppearsInTheRecordUnderItsGroup)
{
    ProfiledFrames session(1U);
    ASSERT_TRUE(session.ok()) << session.error();
    session.frame(
        []
        {
            recorded_work(3);
        });
    const SushiProfiler::RunRecord record = session.finish();

    ASSERT_EQ(record.periods.size(), 1U);
    EXPECT_EQ(zone_names(record, record.periods[0]).count("test_stage"), 1U);
    EXPECT_EQ(site_group(record, "test_stage"), ProfilerKeys::K_GROUP_TRACK);
}

/**
 * @brief An Add counter sums its samples: two samples of 3 give 6.
 */
TEST(Medium_ProfilingCompiledInTest, AddCounterSumsItsSamples)
{
    ProfiledFrames session(1U);
    ASSERT_TRUE(session.ok()) << session.error();
    session.frame(
        []
        {
            recorded_work(3);
        });
    const SushiProfiler::RunRecord record = session.finish();

    ASSERT_EQ(record.periods.size(), 1U);
    EXPECT_EQ(counter_value(record, record.periods[0], "test_items"), 6.0);
}

/**
 * @brief A Set counter holds its last sample: samples 4 then 9 give 9.
 */
TEST(Medium_ProfilingCompiledInTest, SetCounterHoldsItsLastSample)
{
    ProfiledFrames session(1U);
    ASSERT_TRUE(session.ok()) << session.error();
    session.frame(
        []
        {
            set_level(4);
            set_level(9);
        });
    const SushiProfiler::RunRecord record = session.finish();

    ASSERT_EQ(record.periods.size(), 1U);
    EXPECT_EQ(counter_value(record, record.periods[0], "test_level"), 9.0);
}

/**
 * @brief With no session the macros run and record nothing: a session started afterwards holds
 *        neither the zone nor the counter of the earlier call.
 */
TEST(Medium_ProfilingCompiledInTest, RunsWithoutASession)
{
    recorded_work(1);
    ProfiledFrames session(1U);
    ASSERT_TRUE(session.ok()) << session.error();
    session.frame(
        []
        {
        });
    const SushiProfiler::RunRecord record = session.finish();

    ASSERT_EQ(record.periods.size(), 1U);
    EXPECT_EQ(zone_names(record, record.periods[0]).count("test_stage"), 0U);
    EXPECT_EQ(counter_value(record, record.periods[0], "test_items"), -1.0);
}

#endif
