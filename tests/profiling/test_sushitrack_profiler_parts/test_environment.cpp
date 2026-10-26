/**************************************************************************/
/* test_environment.cpp                                                   */
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
 * @file test_environment.cpp
 * @brief Tests the facts SushiTrack adds to a record and that two such records compare.
 * @author Mustafa Garip
 */

#include <SushiTrack/profiling/profiling.hpp>

#if SUSHITRACK_ENABLE_PROFILER

#include <regex>
#include <string>

#include <gtest/gtest.h>

#include <SushiProfiler/compare/compare_records.hpp>

#include <profile_session.hpp>

using namespace SushiTrack;

namespace
{
    /// @brief Returns the value of environment fact @p name of @p record, or `<absent>`.
    std::string fact(const SushiProfiler::RunRecord& record, const std::string& name)
    {
        for (const SushiProfiler::Fact& entry : record.environment)
        {
            if (entry.name == name)
                return entry.value;
        }
        return "<absent>";
    }

    /// @brief Runs five empty frames under a session and returns its record.
    SushiProfiler::RunRecord short_record()
    {
        ProfiledFrames session(5U);
        EXPECT_TRUE(session.ok()) << session.error();
        for (int frame = 0; frame < 5; ++frame)
        {
            session.frame(
                []
                {
                    SUSHITRACK_ZONE("parts_test", ProfilerKeys::K_GROUP_TRACK);
                });
        }
        return session.finish();
    }
} // namespace

/**
 * @brief A record made through the contributor holds the library name, version, commit, profile
 *        level and a UTC date.
 */
TEST(Medium_SushiTrackProfilerPartsTest, RecordHoldsTheLibraryFacts)
{
    const SushiProfiler::RunRecord record = short_record();

    EXPECT_EQ(fact(record, ProfilerKeys::K_FACT_LIBRARY), "SushiTrack");
    EXPECT_NE(fact(record, ProfilerKeys::K_FACT_VERSION), "<absent>");
    EXPECT_NE(fact(record, ProfilerKeys::K_FACT_COMMIT), "<absent>");
    EXPECT_EQ(fact(record, ProfilerKeys::K_FACT_PROFILE_LEVEL),
              std::to_string(SUSHITRACK_PROFILE_LEVEL));
    const std::regex date(R"(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z)");
    EXPECT_TRUE(std::regex_match(fact(record, ProfilerKeys::K_FACT_DATE), date))
        << fact(record, ProfilerKeys::K_FACT_DATE);
}

/**
 * @brief Two records made one after the other in one process compare without a refusal, which
 *        also needs a second session to start after the first stopped.
 */
TEST(Medium_SushiTrackProfilerPartsTest, TwoRecordsFromOneProcessCompare)
{
    const SushiProfiler::RunRecord baseline = short_record();
    const SushiProfiler::RunRecord candidate = short_record();

    const SushiProfiler::Comparison comparison =
        SushiProfiler::compare_records(baseline, candidate);

    EXPECT_TRUE(comparison.ok()) << comparison.error;
}

#endif
