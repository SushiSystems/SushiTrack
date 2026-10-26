/**************************************************************************/
/* test_mot_runner.cpp                                                    */
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
 * @file test_mot_runner.cpp
 * @brief Tests run_mot_sequence on a three-frame sequence written to a temporary folder.
 * @author Mustafa Garip
 */

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

#include <gtest/gtest.h>

#include <SushiProfiler/record_json/record_json.hpp>

#include <SushiTrack/profiling/profiling.hpp>

#include <captured_log.hpp>
#include <mot_engine_log_category.hpp>
#include <mot_runner.hpp>
#include <sushitrack_adapter.hpp>

using namespace MOTEngine;

namespace
{
    /** @brief Lays out `<root>/seq/det/det.txt` and `seqinfo.ini` and removes them afterwards. */
    class Medium_MOTRunnerTest : public ::testing::Test
    {
        protected:
            /// @brief Writes a three-frame sequence at 25 fps.
            void SetUp() override
            {
                std::filesystem::create_directories(root_ / "seq" / "det");
                std::ofstream(root_ / "seq" / "det" / "det.txt")
                    << "1,-1,100,100,50,80,0.9\n2,-1,102,100,50,80,0.9\n3,-1,104,100,50,80,0.9\n";
                std::ofstream(root_ / "seq" / "seqinfo.ini") << "frameRate=25\n";
            }

            /// @brief Removes the folder.
            void TearDown() override { std::filesystem::remove_all(root_); }

            /// @brief The temporary data root.
            const std::filesystem::path root_ =
                std::filesystem::temp_directory_path() / "sushitrack_mot_runner_test";
    };
} // namespace

/**
 * @brief Every frame is processed and the predictions land in `pred/<tracker>/<sequence>.txt`.
 */
TEST_F(Medium_MOTRunnerTest, ProcessesEveryFrameAndWritesPredictions)
{
    TrackerBridge::SushiTrackAdapter adapter;
    const RunResult result = run_mot_sequence(root_, "seq", adapter);

    EXPECT_EQ(result.frames_processed, 3);
    EXPECT_GE(result.mean_us(), 0.0);
    EXPECT_TRUE(std::filesystem::exists(root_ / "seq" / "pred" / "sushitrack" / "seq.txt"));
}

/**
 * @brief A sequence with no detections file is refused.
 */
TEST_F(Medium_MOTRunnerTest, MissingDetectionsAreRefused)
{
    TrackerBridge::SushiTrackAdapter adapter;
    EXPECT_THROW(static_cast<void>(run_mot_sequence(root_, "absent", adapter)), std::runtime_error);
}

/**
 * @brief A sequence whose detections file holds no rows logs one WARN, reports no frame and gives
 *        no `[ PERF ]` line, where it once printed `0 us/frame (0 frames)` as a success.
 */
TEST_F(Medium_MOTRunnerTest, EmptyDetectionsAreSkippedWithAWarnAndNoPerfLine)
{
    SushiTrack::CapturedLog log(SushiLog::LogLevel::Trace, {K_MOT_ENGINE_LOG_CATEGORY});
    std::ofstream(root_ / "seq" / "det" / "det.txt", std::ios::trunc) << "";
    TrackerBridge::SushiTrackAdapter adapter;

    const RunResult result = run_mot_sequence(root_, "seq", adapter);

    EXPECT_EQ(result.frames_processed, 0);
    EXPECT_EQ(perf_line("sushitrack", "seq", result), "");
    EXPECT_EQ(log.count("holds no detections; the sequence is skipped"), 1);
}

/**
 * @brief The `[ PERF ]` line of a run with frames names the tracker, the sequence and the count.
 */
TEST(Small_MOTRunnerPerfLineTest, NamesTheTrackerTheSequenceAndTheFrameCount)
{
    RunResult result;
    result.frames_processed = 4;
    result.total_tracking_us = 80.0;

    EXPECT_EQ(perf_line("sushitrack", "MOT17-02", result),
              "[ PERF sushitrack ] MOT17-02: 20 us/frame  (4 frames)");
}

#if SUSHITRACK_ENABLE_PROFILER
/**
 * @brief A run with a profiles folder writes a record `read_record` accepts, one period per
 *        frame and no dropped event, and the predictions are the same as without it.
 */
TEST_F(Medium_MOTRunnerTest, ProfileRecordHoldsOnePeriodPerFrameAndLeavesPredictionsAlone)
{
    TrackerBridge::SushiTrackAdapter plain_adapter;
    const RunResult plain = run_mot_sequence(root_, "seq", plain_adapter);
    ASSERT_TRUE(plain.profile.empty());
    std::ifstream plain_file(root_ / "seq" / "pred" / "sushitrack" / "seq.txt");
    std::stringstream plain_text;
    plain_text << plain_file.rdbuf();

    TrackerBridge::SushiTrackAdapter adapter;
    const RunResult profiled =
        run_mot_sequence(root_, "seq", adapter, root_ / "predictions", root_ / "profiles");

    ASSERT_FALSE(profiled.profile.empty());
    EXPECT_EQ(profiled.frames_processed, 3);
    EXPECT_EQ(profiled.profile_dropped, 0U);
    std::ifstream record_file(profiled.profile, std::ios::binary);
    std::stringstream record_text;
    record_text << record_file.rdbuf();
    const SushiProfiler::RecordReadResult read = SushiProfiler::read_record(record_text.str());
    ASSERT_TRUE(read.ok()) << read.error;
    EXPECT_EQ(read.record->periods.size(), 3U);

    std::ifstream profiled_file(root_ / "predictions" / "sushitrack" / "seq.txt");
    std::stringstream profiled_text;
    profiled_text << profiled_file.rdbuf();
    EXPECT_EQ(profiled_text.str(), plain_text.str());
}
#endif
