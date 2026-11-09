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
 * @file test_mot_runner.cpp
 * @brief Tests run_mot_sequence on a three-frame sequence written to a temporary folder, and the
 *        rows and the PERF line it prints.
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

#include <byte_track_adapter.hpp>
#include <captured_log.hpp>
#include <mot_engine_log_category.hpp>
#include <mot_runner.hpp>
#include <oc_sort_adapter.hpp>
#include <support/default_params.hpp>
#include <sushitrack_adapter.hpp>

using namespace MOTEngine;

namespace
{
    /** @brief Owns a data root named after the running test and removes it afterwards. */
    class Medium_MOTRunnerTest : public ::testing::Test
    {
        protected:
            /// @brief Starts from a data root that does not exist.
            void SetUp() override { std::filesystem::remove_all(root_); }

            /// @brief Removes the folder.
            void TearDown() override { std::filesystem::remove_all(root_); }

            /// @brief Writes `seq/det/det.txt` with three frames and `seq/seqinfo.ini` at 25 fps.
            void write_sequence() const
            {
                std::filesystem::create_directories(root_ / "seq" / "det");
                std::ofstream(root_ / "seq" / "det" / "det.txt")
                    << "1,-1,100,100,50,80,0.9\n2,-1,102,100,50,80,0.9\n3,-1,104,100,50,80,0.9\n";
                std::ofstream(root_ / "seq" / "seqinfo.ini") << "frameRate=25\n";
            }

            /// @brief Runs @p adapter over the three-frame sequence and returns whether it wrote
            ///        `pred/<folder>/seq.txt`.
            [[nodiscard]] bool wrote_under(TrackerBridge::TrackerAdapter& adapter,
                                           const std::string& folder) const
            {
                write_sequence();
                static_cast<void>(run_mot_sequence(root_, "seq", adapter));
                return std::filesystem::exists(root_ / "seq" / "pred" / folder / "seq.txt");
            }

            /// @brief Returns the text of the file at @p path.
            [[nodiscard]] static std::string read_text(const std::filesystem::path& path)
            {
                std::ifstream file(path, std::ios::binary);
                std::stringstream text;
                text << file.rdbuf();
                return text.str();
            }

            /// @brief The data root, named after the running test so two tests never share one.
            const std::filesystem::path root_ =
                std::filesystem::temp_directory_path() /
                (std::string("sushitrack_mot_runner_") +
                 ::testing::UnitTest::GetInstance()->current_test_info()->name());
    };
} // namespace

/**
 * @brief Every frame of the detections file is handed to the tracker once.
 */
TEST_F(Medium_MOTRunnerTest, ProcessesEveryFrameOnce)
{
    write_sequence();
    TrackerBridge::SushiTrackAdapter adapter(TestSupport::default_params());

    const RunResult result = run_mot_sequence(root_, "seq", adapter);

    EXPECT_EQ(result.frames_processed, 3);
}

/**
 * @brief The predictions land in `pred/<tracker>/<sequence>.txt` of the sequence folder.
 */
TEST_F(Medium_MOTRunnerTest, WritesPredictionsToTheAdapterFolder)
{
    write_sequence();
    TrackerBridge::SushiTrackAdapter adapter(TestSupport::default_params());

    static_cast<void>(run_mot_sequence(root_, "seq", adapter));

    EXPECT_TRUE(std::filesystem::exists(root_ / "seq" / "pred" / "sushitrack" / "seq.txt"));
}

/**
 * @brief ByteTrack's predictions land in the `bytetrack` folder `st eval` reads.
 */
TEST_F(Medium_MOTRunnerTest, ByteTrackWritesToTheBytetrackFolder)
{
    TrackerBridge::ByteTrackAdapter adapter;

    EXPECT_TRUE(wrote_under(adapter, "bytetrack"));
}

/**
 * @brief OC-SORT's predictions land in the `ocsort` folder `st eval` reads.
 */
TEST_F(Medium_MOTRunnerTest, OcSortWritesToTheOcsortFolder)
{
    TrackerBridge::OCSORTAdapter adapter;

    EXPECT_TRUE(wrote_under(adapter, "ocsort"));
}

/**
 * @brief A sequence with no detections file throws a runtime_error that names det.txt.
 */
TEST_F(Medium_MOTRunnerTest, MissingDetectionsAreRefused)
{
    TrackerBridge::SushiTrackAdapter adapter(TestSupport::default_params());

    try
    {
        static_cast<void>(run_mot_sequence(root_, "absent", adapter));
        FAIL() << "run_mot_sequence did not throw";
    }
    catch (const std::runtime_error& refused)
    {
        EXPECT_NE(std::string(refused.what()).find("det.txt"), std::string::npos);
    }
}

/**
 * @brief A predictions path that cannot be created throws a runtime_error.
 */
TEST_F(Medium_MOTRunnerTest, APredictionsPathThatCannotBeWrittenIsRefused)
{
    write_sequence();
    { std::ofstream(root_ / "blocker").put('x'); }
    TrackerBridge::SushiTrackAdapter adapter(TestSupport::default_params());

    EXPECT_THROW(static_cast<void>(run_mot_sequence(root_, "seq", adapter, root_ / "blocker")),
                 std::runtime_error);
}

/**
 * @brief A sequence whose detections file holds no rows logs one WARN naming the skip.
 */
TEST_F(Medium_MOTRunnerTest, EmptyDetectionsLogOneWarn)
{
    write_sequence();
    std::ofstream(root_ / "seq" / "det" / "det.txt", std::ios::trunc) << "";
    const SushiTrack::CapturedLog log(SushiLog::LogLevel::Trace, {K_MOT_ENGINE_LOG_CATEGORY});
    TrackerBridge::SushiTrackAdapter adapter(TestSupport::default_params());

    static_cast<void>(run_mot_sequence(root_, "seq", adapter));

    EXPECT_EQ(log.count("holds no detections; the sequence is skipped"), 1);
    EXPECT_EQ(log.level_of("holds no detections; the sequence is skipped"),
              SushiLog::LogLevel::Warn);
}

/**
 * @brief A sequence whose detections file holds no rows reports no frame.
 */
TEST_F(Medium_MOTRunnerTest, EmptyDetectionsReportNoFrame)
{
    write_sequence();
    std::ofstream(root_ / "seq" / "det" / "det.txt", std::ios::trunc) << "";
    TrackerBridge::SushiTrackAdapter adapter(TestSupport::default_params());

    const RunResult result = run_mot_sequence(root_, "seq", adapter);

    EXPECT_EQ(result.frames_processed, 0);
}

/**
 * @brief The `[ PERF ]` line of a run with frames names the tracker, the sequence and the count.
 *        80 us over 4 frames is 80 / 4 = 20 us per frame.
 */
TEST(Small_MOTRunnerPerfLineTest, NamesTheTrackerTheSequenceAndTheFrameCount)
{
    RunResult result;
    result.frames_processed = 4;
    result.total_tracking_us = 80.0;

    EXPECT_EQ(perf_line("sushitrack", "MOT17-02", result),
              "[ PERF sushitrack ] MOT17-02: 20 us/frame  (4 frames)");
}

/**
 * @brief A run that processed no frame has no line, where it once printed `0 us/frame` as a
 *        measurement.
 */
TEST(Small_MOTRunnerPerfLineTest, AnEmptyRunHasNoLine)
{
    EXPECT_EQ(perf_line("sushitrack", "seq", RunResult{}), "");
}

/**
 * @brief Each track is one row `frame,id,x,y,w,h,score,-1,-1,-1`.
 */
TEST(Small_MOTRunnerRowsTest, WriteMotChallengeRows)
{
    std::ostringstream out;
    const std::vector<TrackerBridge::TrackResult> tracks = {{7, 1.5f, 2.5f, 30.f, 40.f, 0.5f}};

    write_mot_tracks(out, 3, tracks);

    EXPECT_EQ(out.str(), "3,7,1.5,2.5,30,40,0.5,-1,-1,-1\n");
}

#if SUSHITRACK_ENABLE_PROFILER
/**
 * @brief A run with a profiles folder writes a record `read_record` accepts, with one period per
 *        frame and no dropped event.
 */
TEST_F(Medium_MOTRunnerTest, ProfileRecordHoldsOnePeriodPerFrame)
{
    write_sequence();
    TrackerBridge::SushiTrackAdapter adapter(TestSupport::default_params());

    const RunResult profiled =
        run_mot_sequence(root_, "seq", adapter, root_ / "predictions", root_ / "profiles");

    ASSERT_FALSE(profiled.profile.empty());
    EXPECT_EQ(profiled.profile_dropped, 0U);
    const SushiProfiler::RecordReadResult read =
        SushiProfiler::read_record(read_text(profiled.profile));
    ASSERT_TRUE(read.ok()) << read.error;
    EXPECT_EQ(read.record->periods.size(), 3U);
}

/**
 * @brief The predictions of a profiled run are the predictions of a plain run, which is the
 *        oracle.
 */
TEST_F(Medium_MOTRunnerTest, ProfilingLeavesThePredictionsAlone)
{
    write_sequence();
    TrackerBridge::SushiTrackAdapter plain_adapter(TestSupport::default_params());
    static_cast<void>(run_mot_sequence(root_, "seq", plain_adapter, root_ / "plain"));
    TrackerBridge::SushiTrackAdapter adapter(TestSupport::default_params());

    static_cast<void>(
        run_mot_sequence(root_, "seq", adapter, root_ / "predictions", root_ / "profiles"));

    EXPECT_EQ(read_text(root_ / "predictions" / "sushitrack" / "seq.txt"),
              read_text(root_ / "plain" / "sushitrack" / "seq.txt"));
}
#endif
