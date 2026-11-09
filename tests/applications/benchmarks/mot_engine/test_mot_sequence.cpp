/**************************************************************************/
/* test_mot_sequence.cpp                                                  */
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
 * @file test_mot_sequence.cpp
 * @brief Tests parse_mot_detections and parse_frame_rate on streams, and read_mot_detections and
 *        read_frame_rate on the files they open.
 * @author Mustafa Garip
 */

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include <gtest/gtest.h>

#include <captured_log.hpp>
#include <mot_engine_log_category.hpp>
#include <mot_sequence.hpp>

using namespace MOTEngine;

namespace
{
    /// @brief The detections path no test creates.
    constexpr const char* K_MISSING_DETECTIONS = "this/path/does/not/exist/det.txt";
    /// @brief The seqinfo path no test creates.
    constexpr const char* K_MISSING_SEQINFO = "this/path/does/not/exist/seqinfo.ini";

    /** @brief Writes @p text to a file named after the running test and returns its path. */
    std::filesystem::path write_temp(const std::string& text)
    {
        const std::filesystem::path path =
            std::filesystem::temp_directory_path() /
            (std::string("sushitrack_mot_sequence_") +
             ::testing::UnitTest::GetInstance()->current_test_info()->name());
        std::ofstream(path) << text;
        return path;
    }

    /** @brief Parses @p text as a detections file. */
    MOTFrames parse(const std::string& text)
    {
        std::istringstream rows(text);
        return parse_mot_detections(rows, "det.txt");
    }

    /** @brief Parses @p text as a seqinfo file, with 30 as the fallback. */
    int parse_rate(const std::string& text)
    {
        std::istringstream lines(text);
        return parse_frame_rate(lines, "seqinfo.ini", 30);
    }
} // namespace

/**
 * @brief Rows group by the frame number in column 1.
 */
TEST(Small_MOTSequenceTest, GroupsRowsByFrame)
{
    const MOTFrames frames =
        parse("1,-1,10,20,30,40,0.9\n1,-1,50,60,70,80,0.8\n3,-1,5,6,7,8,0.7\n");

    ASSERT_EQ(frames.size(), 2U);
    EXPECT_EQ(frames.at(1).size(), 2U);
    EXPECT_EQ(frames.at(3).size(), 1U);
}

/**
 * @brief The box comes from columns 3 to 6 and the score from column 7.
 */
TEST(Small_MOTSequenceTest, ReadsTheBoxAndTheScoreFromTheirColumns)
{
    const MOTFrames frames = parse("1,-1,10,20,30,40,0.5\n");

    const TrackerBridge::MOTDetection& det = frames.at(1).at(0);
    EXPECT_EQ(det.x, 10.f);
    EXPECT_EQ(det.y, 20.f);
    EXPECT_EQ(det.w, 30.f);
    EXPECT_EQ(det.h, 40.f);
    EXPECT_EQ(det.score, 0.5f);
}

/**
 * @brief The label comes from column 8 and is 1 when the row has no such column.
 */
TEST(Small_MOTSequenceTest, TakesTheLabelFromColumnEightOrOne)
{
    const MOTFrames frames = parse("1,-1,10,20,30,40,0.9\n1,-1,50,60,70,80,0.8,2\n");

    EXPECT_EQ(frames.at(1).at(0).label, 1);
    EXPECT_EQ(frames.at(1).at(1).label, 2);
}

/**
 * @brief A comment and a blank line are skipped without a word.
 */
TEST(Small_MOTSequenceTest, SkipsCommentsAndBlankLinesSilently)
{
    const SushiTrack::CapturedLog log(SushiLog::LogLevel::Trace, {K_MOT_ENGINE_LOG_CATEGORY});

    const MOTFrames frames = parse("# comment\n\n1,-1,10,20,30,40,0.9\n");

    EXPECT_EQ(frames.size(), 1U);
    EXPECT_EQ(log.count("skipped"), 0);
}

/**
 * @brief Rows with fewer than seven numbers are left out and counted in one WARN with the origin
 *        and the number of rows read: 2 of the 3 rows below.
 */
TEST(Small_MOTSequenceTest, SkippedRowsAreCountedInOneWarn)
{
    const SushiTrack::CapturedLog log(SushiLog::LogLevel::Trace, {K_MOT_ENGINE_LOG_CATEGORY});

    const MOTFrames frames = parse("1,-1,10,20,30,40,0.9\n2,-1,1,2\n3,-1,x,2,3,4,5\n");

    EXPECT_EQ(frames.size(), 1U);
    EXPECT_EQ(log.count("det.txt: skipped 2 of 3 rows"), 1);
}

/**
 * @brief The frame rate is the number after `frameRate=`.
 */
TEST(Small_MOTSequenceTest, ReadsTheFrameRate)
{
    EXPECT_EQ(parse_rate("[Sequence]\nframeRate=25\n"), 25);
}

/**
 * @brief A seqinfo without `frameRate=` gives the fallback and one WARN.
 */
TEST(Small_MOTSequenceTest, AMissingFrameRateGivesTheFallbackAndOneWarn)
{
    const SushiTrack::CapturedLog log(SushiLog::LogLevel::Trace, {K_MOT_ENGINE_LOG_CATEGORY});

    EXPECT_EQ(parse_rate("[Sequence]\nname=x\n"), 30);
    EXPECT_EQ(log.count("holds no positive frameRate; using 30 frames per second"), 1);
}

/**
 * @brief A frame rate that is not positive gives the fallback and one WARN.
 */
TEST(Small_MOTSequenceTest, AFrameRateThatIsNotPositiveGivesTheFallbackAndOneWarn)
{
    const SushiTrack::CapturedLog log(SushiLog::LogLevel::Trace, {K_MOT_ENGINE_LOG_CATEGORY});

    EXPECT_EQ(parse_rate("frameRate=0\n"), 30);
    EXPECT_EQ(log.count("holds no positive frameRate; using 30 frames per second"), 1);
}

/**
 * @brief read_mot_detections opens the file it is given and reads its rows.
 */
TEST(Medium_MOTSequenceTest, ReadsTheDetectionsOfAFile)
{
    const std::filesystem::path path = write_temp("1,-1,10,20,30,40,0.9\n");

    const MOTFrames frames = read_mot_detections(path.string());
    std::filesystem::remove(path);

    ASSERT_EQ(frames.size(), 1U);
    EXPECT_EQ(frames.at(1).at(0).x, 10.f);
}

/**
 * @brief A detections file that cannot be opened reads as empty.
 */
TEST(Medium_MOTSequenceTest, UnreadableDetectionsReadNothing)
{
    EXPECT_TRUE(read_mot_detections(K_MISSING_DETECTIONS).empty());
}

/**
 * @brief A detections file that cannot be opened logs one WARN naming it, where it once returned
 *        an empty map without a word.
 */
TEST(Medium_MOTSequenceTest, UnreadableDetectionsLogAWarnNamingTheFile)
{
    const SushiTrack::CapturedLog log(SushiLog::LogLevel::Trace, {K_MOT_ENGINE_LOG_CATEGORY});

    static_cast<void>(read_mot_detections(K_MISSING_DETECTIONS));

    EXPECT_EQ(log.count("this/path/does/not/exist/det.txt cannot be opened"), 1);
    EXPECT_EQ(log.level_of("cannot be opened"), SushiLog::LogLevel::Warn);
}

/**
 * @brief read_frame_rate opens the file it is given and returns its frame rate.
 */
TEST(Medium_MOTSequenceTest, ReadsTheFrameRateOfAFile)
{
    const std::filesystem::path path = write_temp("[Sequence]\nframeRate=25\n");

    const int fps = read_frame_rate(path.string(), 30);
    std::filesystem::remove(path);

    EXPECT_EQ(fps, 25);
}

/**
 * @brief A seqinfo.ini that cannot be opened gives the fallback and one WARN.
 */
TEST(Medium_MOTSequenceTest, AMissingSeqinfoGivesTheFallbackAndOneWarn)
{
    const SushiTrack::CapturedLog log(SushiLog::LogLevel::Trace, {K_MOT_ENGINE_LOG_CATEGORY});

    EXPECT_EQ(read_frame_rate(K_MISSING_SEQINFO, 30), 30);
    EXPECT_EQ(log.count("seqinfo.ini cannot be opened; using 30 frames per second"), 1);
}
