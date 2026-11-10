/**************************************************************************/
/* test_profile_loss.cpp                                                  */
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
 * @file test_profile_loss.cpp
 * @brief Tests account_for_lost_events on records built by hand.
 * @author Mustafa Garip
 */

#include <profile_loss.hpp>

#if SUSHITRACK_ENABLE_PROFILER

#include <filesystem>

#include <gtest/gtest.h>

#include <SushiProfiler/record/run_record.hpp>

#include <captured_log.hpp>
#include <mot_engine_log_category.hpp>

using namespace MOTEngine;

namespace
{
    /// @brief The record every test below starts from: two threads and no loss.
    SushiProfiler::RunRecord clean_record()
    {
        SushiProfiler::RunRecord record;
        record.threads.resize(2U);
        return record;
    }
} // namespace

/**
 * @brief Each of the six loss fields adds to the sum once. The fields hold distinct powers of two,
 *        so a field that is missed or counted twice changes the sum to a value no other
 *        combination gives: 1 + 2 + 4 + 8 + 16 + 32 = 63.
 */
TEST(Small_ProfileLossTest, SumsEveryLossFieldOnce)
{
    SushiProfiler::RunRecord record = clean_record();
    record.drops.outside_periods = 1U;
    record.drops.open_at_stop = 2U;
    record.threads[0].dropped = 4U;
    record.threads[0].late = 8U;
    record.threads[1].dropped = 16U;
    record.threads[1].late = 32U;

    EXPECT_EQ(account_for_lost_events(record, "run.sprec.json"), 63U);
}

/**
 * @brief A loss logs one WARN that names the record file and the count.
 */
TEST(Small_ProfileLossTest, ALossLogsOneWarnNamingTheFileAndTheCount)
{
    SushiProfiler::RunRecord record = clean_record();
    record.threads[1].dropped = 7U;
    const SushiTrack::CapturedLog log(SushiLog::LogLevel::Trace, {K_MOT_ENGINE_LOG_CATEGORY});

    static_cast<void>(account_for_lost_events(record, "run.sprec.json"));

    EXPECT_EQ(log.count("The profile of run.sprec.json lost 7 events"), 1);
    EXPECT_EQ(log.level_of("The profile of run.sprec.json lost 7 events"),
              SushiLog::LogLevel::Warn);
}

/**
 * @brief A record that lost nothing sums to zero and logs nothing.
 */
TEST(Small_ProfileLossTest, ARecordThatLostNothingLogsNothing)
{
    const SushiProfiler::RunRecord record = clean_record();
    const SushiTrack::CapturedLog log(SushiLog::LogLevel::Trace, {K_MOT_ENGINE_LOG_CATEGORY});

    EXPECT_EQ(account_for_lost_events(record, "run.sprec.json"), 0U);
    EXPECT_EQ(log.count("lost"), 0);
}

#endif
