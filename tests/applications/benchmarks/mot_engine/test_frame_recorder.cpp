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
 * @file test_frame_recorder.cpp
 * @brief Tests FrameRecorder on a few marked frames and on an empty folder.
 * @author Mustafa Garip
 */

#include <SushiTrack/profiling/profiling.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include <gtest/gtest.h>

#include <frame_recorder.hpp>

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
/**
 * @brief A recorder writes `<tracker>_<sequence>.sprec.json` with one period per marked frame, the
 *        tracker and the sequence in its environment, and no lost event.
 */
TEST(Medium_FrameRecorderTest, WritesOnePeriodPerFrameWithTheRunFacts)
{
    const std::filesystem::path folder =
        std::filesystem::temp_directory_path() / "sushitrack_frame_recorder_test";
    std::filesystem::remove_all(folder);
    FrameRecorder recorder(folder, "sushitrack", "seq", 4U);
    for (int frame = 0; frame < 4; ++frame)
        recorder.begin_frame();

    const FrameRecording recording = recorder.finish();

    ASSERT_TRUE(recording.error.empty()) << recording.error;
    EXPECT_EQ(recording.path.filename().string(), "sushitrack_seq.sprec.json");
    EXPECT_EQ(recording.dropped, 0U);
    std::ifstream file(recording.path, std::ios::binary);
    std::stringstream text;
    text << file.rdbuf();
    const SushiProfiler::RecordReadResult read = SushiProfiler::read_record(text.str());
    ASSERT_TRUE(read.ok()) << read.error;
    EXPECT_EQ(read.record->periods.size(), 4U);
    bool tracker_fact = false;
    bool sequence_fact = false;
    for (const SushiProfiler::Fact& fact : read.record->environment)
    {
        tracker_fact = tracker_fact || (fact.name == "mot.tracker" && fact.value == "sushitrack");
        sequence_fact = sequence_fact || (fact.name == "mot.sequence" && fact.value == "seq");
    }
    EXPECT_TRUE(tracker_fact);
    EXPECT_TRUE(sequence_fact);
    std::filesystem::remove_all(folder);
}
#endif
