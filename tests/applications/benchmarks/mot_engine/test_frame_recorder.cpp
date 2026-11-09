/**************************************************************************/
/* test_frame_recorder.cpp                                                */
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
 * @file test_frame_recorder.cpp
 * @brief Tests FrameRecorder on a few marked frames, on an empty folder and on a folder it cannot write.
 * @author Mustafa Garip
 */

#include <SushiTrack/profiling/profiling.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include <gtest/gtest.h>

#include <captured_log.hpp>
#include <frame_recorder.hpp>
#include <mot_engine_log_category.hpp>

#if SUSHITRACK_ENABLE_PROFILER
#include <SushiProfiler/record_json/record_json.hpp>
#endif

using namespace MOTEngine;

/**
 * @brief A recorder given no folder does nothing and finishes with no file and no error.
 */
TEST(Small_FrameRecorderTest, EmptyFolderRecordsNothing)
{
    FrameRecorder recorder({}, "sushitrack", "seq", 3U);
    recorder.begin_frame();

    const FrameRecording recording = recorder.finish();

    EXPECT_TRUE(recording.path.empty());
    EXPECT_TRUE(recording.error.empty());
    EXPECT_EQ(recording.dropped, 0U);
}

#if SUSHITRACK_ENABLE_PROFILER
namespace
{
    /// @brief The frames each run below marks.
    constexpr std::size_t K_FRAMES = 4U;

    /**
     * @brief Returns an empty folder named after the running test, so two tests never share one.
     */
    std::filesystem::path fresh_folder()
    {
        const std::string name =
            std::string("sushitrack_frame_recorder_") +
            ::testing::UnitTest::GetInstance()->current_test_info()->name();
        const std::filesystem::path folder = std::filesystem::temp_directory_path() / name;
        std::filesystem::remove_all(folder);
        return folder;
    }

    /**
     * @brief Records K_FRAMES marked frames of tracker `sushitrack` on sequence `seq` into @p folder.
     */
    FrameRecording record_frames(const std::filesystem::path& folder)
    {
        FrameRecorder recorder(folder, "sushitrack", "seq", K_FRAMES);
        for (std::size_t frame = 0U; frame < K_FRAMES; ++frame)
            recorder.begin_frame();
        return recorder.finish();
    }

    /**
     * @brief Reads the record file @p recording points to, failing the test when it does not parse.
     */
    SushiProfiler::RecordReadResult read_back(const FrameRecording& recording)
    {
        std::stringstream text;
        std::ifstream file(recording.path, std::ios::binary);
        text << file.rdbuf();
        return SushiProfiler::read_record(text.str());
    }

    /**
     * @brief Returns whether the record's environment holds the fact @p name with @p value.
     */
    template <typename Record>
    bool has_fact(const Record& record, const std::string& name,
                  const std::string& value)
    {
        for (const SushiProfiler::Fact& fact : record.environment)
        {
            if (fact.name == name && fact.value == value)
                return true;
        }
        return false;
    }
} // namespace

/**
 * @brief The record is `<tracker>_<sequence>.sprec.json` and no event was lost.
 */
TEST(Medium_FrameRecorderTest, NamesTheRecordAfterTheTrackerAndTheSequence)
{
    const std::filesystem::path folder = fresh_folder();

    const FrameRecording recording = record_frames(folder);

    ASSERT_TRUE(recording.error.empty()) << recording.error;
    EXPECT_EQ(recording.path.filename().string(), "sushitrack_seq.sprec.json");
    EXPECT_EQ(recording.dropped, 0U);
    std::filesystem::remove_all(folder);
}

/**
 * @brief The record holds one period for each marked frame.
 */
TEST(Medium_FrameRecorderTest, WritesOnePeriodPerFrame)
{
    const std::filesystem::path folder = fresh_folder();

    const FrameRecording recording = record_frames(folder);

    ASSERT_TRUE(recording.error.empty()) << recording.error;
    const SushiProfiler::RecordReadResult read = read_back(recording);
    ASSERT_TRUE(read.ok()) << read.error;
    EXPECT_EQ(read.record->periods.size(), K_FRAMES);
    std::filesystem::remove_all(folder);
}

/**
 * @brief The record's environment names the tracker and the sequence of the run.
 */
TEST(Medium_FrameRecorderTest, RecordsTheTrackerAndTheSequenceInItsEnvironment)
{
    const std::filesystem::path folder = fresh_folder();

    const FrameRecording recording = record_frames(folder);

    ASSERT_TRUE(recording.error.empty()) << recording.error;
    const SushiProfiler::RecordReadResult read = read_back(recording);
    ASSERT_TRUE(read.ok()) << read.error;
    EXPECT_TRUE(has_fact(*read.record, "mot.tracker", "sushitrack"));
    EXPECT_TRUE(has_fact(*read.record, "mot.sequence", "seq"));
    std::filesystem::remove_all(folder);
}

/**
 * @brief A folder that is a regular file cannot hold the record: finish sets the error, returns
 *        no path and logs one ERROR.
 */
TEST(Medium_FrameRecorderTest, ARecordThatCannotBeWrittenSetsTheErrorAndLogsError)
{
    const std::filesystem::path folder = fresh_folder();
    { std::ofstream(folder).put('x'); }
    const SushiTrack::CapturedLog log(SushiLog::LogLevel::Trace, {K_MOT_ENGINE_LOG_CATEGORY});

    const FrameRecording recording = record_frames(folder);

    EXPECT_EQ(recording.error, "cannot write " + (folder / "sushitrack_seq.sprec.json").string());
    EXPECT_TRUE(recording.path.empty());
    EXPECT_EQ(log.count("No profile written"), 1);
    EXPECT_EQ(log.level_of("No profile written"), SushiLog::LogLevel::Error);
    std::filesystem::remove_all(folder);
}
#endif
