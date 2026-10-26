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
    bool grouped = false;
    for (const SushiProfiler::SiteRecord& site : record.sites)
    {
        if (site.name == "test_stage")
            grouped = site.group == ProfilerKeys::K_GROUP_TRACK;
    }
    EXPECT_TRUE(grouped);
}

/**
 * @brief A counter site takes the value of its argument and an Add counter sums its samples.
 */
TEST(Medium_ProfilingCompiledInTest, CounterHoldsTheValueAndAddSums)
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
 * @brief With no session the macros run and record nothing.
 */
TEST(Medium_ProfilingCompiledInTest, RunsWithoutASession)
{
    EXPECT_NO_THROW(recorded_work(1));
}

#endif
