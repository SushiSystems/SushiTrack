/**************************************************************************/
/* test_tracker_bridge.cpp                                                */
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
 * @file test_tracker_bridge.cpp
 * @brief Tests the C entry points of tracker_bridge the inference pipeline calls.
 * @author Mustafa Garip
 */

#include <vector>

#include <gtest/gtest.h>

#include <captured_log.hpp>
#include <tracker_bridge.h>
#include <tracker_bridge_log_category.hpp>

/**
 * @brief Each registered name, in the case the pipeline passes, builds a tracker.
 */
TEST(Small_TrackerBridgeTest, BuildsEveryRegisteredTracker)
{
    for (const char* name : {"SushiTrack", "ByteTrack", "OCSORT"})
    {
        void* tracker = create_tracker(name);
        EXPECT_NE(tracker, nullptr) << name;
        delete_tracker(tracker);
    }
    EXPECT_EQ(create_tracker("no-such-tracker"), nullptr);
    EXPECT_EQ(create_tracker(nullptr), nullptr);
}

/**
 * @brief One steady detection over ten frames comes back as one track, within the capacity.
 */
TEST(Small_TrackerBridgeTest, ProcessWritesTheTracksWithinTheCapacity)
{
    void* tracker = create_tracker("sushitrack");
    ASSERT_NE(tracker, nullptr);

    std::vector<CTrackResult> results(4);
    int count = -1;
    for (int frame = 0; frame < 10; ++frame)
    {
        const CDetection det{100.f + frame, 100.f, 50.f, 80.f, 0.9f, 1, nullptr, 0};
        tracker_process_c(tracker, &det, 1, results.data(), &count,
                          static_cast<int>(results.size()));
    }
    delete_tracker(tracker);

    EXPECT_EQ(count, 1);
}

/**
 * @brief A null result buffer is refused without a write.
 */
TEST(Small_TrackerBridgeTest, NullResultsAreRefused)
{
    SushiTrack::CapturedLog log(SushiLog::LogLevel::Trace,
                                {TrackerBridge::K_TRACKER_BRIDGE_LOG_CATEGORY});
    void* tracker = create_tracker("sushitrack");
    int count = -1;
    EXPECT_EQ(tracker_process_c(tracker, nullptr, 0, nullptr, &count, 0),
              TRACKER_BRIDGE_INVALID_ARGUMENT);
    delete_tracker(tracker);
    EXPECT_EQ(count, -1);
    EXPECT_EQ(log.count("tracker_process_c refused: tracker, results or num_results is null"), 1);
}

/**
 * @brief An unknown or null tracker name logs one ERROR that lists the registered names.
 */
TEST(Small_TrackerBridgeTest, RefusedNamesAreLogged)
{
    SushiTrack::CapturedLog log(SushiLog::LogLevel::Trace,
                                {TrackerBridge::K_TRACKER_BRIDGE_LOG_CATEGORY});

    EXPECT_EQ(create_tracker("no-such-tracker"), nullptr);
    EXPECT_EQ(create_tracker(nullptr), nullptr);

    EXPECT_EQ(log.count("no-such-tracker is not registered (known: "), 1);
    EXPECT_EQ(log.count("create_tracker refused: the name is null"), 1);
    EXPECT_EQ(log.level_of("is not registered"), SushiLog::LogLevel::Error);
}

/**
 * @brief Tracks past max_results are left out, the status says so and one WARN names the counts.
 */
TEST(Small_TrackerBridgeTest, TruncationIsReportedByStatusAndLog)
{
    SushiTrack::CapturedLog log(SushiLog::LogLevel::Trace,
                                {TrackerBridge::K_TRACKER_BRIDGE_LOG_CATEGORY});
    void* tracker = create_tracker("sushitrack");
    ASSERT_NE(tracker, nullptr);
    const CDetection dets[2] = {{100.f, 100.f, 50.f, 80.f, 0.9f, 1, nullptr, 0},
                                {400.f, 100.f, 50.f, 80.f, 0.9f, 1, nullptr, 0}};
    std::vector<CTrackResult> results(1);
    int count = -1;
    tracker_bridge_status_t status = TRACKER_BRIDGE_OK;

    for (int frame = 0; frame < 3; ++frame)
        status = tracker_process_c(tracker, dets, 2, results.data(), &count, 1);
    delete_tracker(tracker);

    EXPECT_EQ(status, TRACKER_BRIDGE_TRUNCATED);
    EXPECT_EQ(count, 1);
    EXPECT_GE(log.count("tracker_process_c wrote 1 of 2 tracks; max_results is 1"), 1);
}

/**
 * @brief An empty frame still runs the tracker and returns OK with no track.
 */
TEST(Small_TrackerBridgeTest, EmptyFrameRunsAndReturnsOk)
{
    void* tracker = create_tracker("sushitrack");
    ASSERT_NE(tracker, nullptr);
    std::vector<CTrackResult> results(4);
    int count = -1;

    EXPECT_EQ(tracker_process_c(tracker, nullptr, 0, results.data(), &count, 4),
              TRACKER_BRIDGE_OK);
    delete_tracker(tracker);

    EXPECT_EQ(count, 0);
}

/**
 * @brief A negative count is refused with a status and an ERROR, and writes zero results.
 */
TEST(Small_TrackerBridgeTest, NegativeCountsAreRefused)
{
    SushiTrack::CapturedLog log(SushiLog::LogLevel::Trace,
                                {TrackerBridge::K_TRACKER_BRIDGE_LOG_CATEGORY});
    void* tracker = create_tracker("sushitrack");
    ASSERT_NE(tracker, nullptr);
    std::vector<CTrackResult> results(4);
    int count = -1;

    EXPECT_EQ(tracker_process_c(tracker, nullptr, -1, results.data(), &count, 4),
              TRACKER_BRIDGE_INVALID_ARGUMENT);
    delete_tracker(tracker);

    EXPECT_EQ(count, 0);
    EXPECT_EQ(log.count("tracker_process_c refused: num_detections is -1"), 1);
}
