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
 * @file test_tracker_bridge.cpp
 * @brief Tests the C entry points of tracker_bridge the inference pipeline calls.
 * @author Mustafa Garip
 */

#include <vector>

#include <gtest/gtest.h>

#include <captured_log.hpp>
#include <oc_sort_adapter.hpp>
#include <tracker_adapter.hpp>
#include <tracker_bridge.h>
#include <tracker_bridge_log_category.hpp>

namespace
{
    /// @brief Frames the steady object moves for.
    constexpr int K_STEADY_FRAMES = 10;

    /// @brief Returns @p adapter as the opaque handle tracker_process_c takes.
    void* handle_of(TrackerBridge::TrackerAdapter& adapter)
    {
        return &adapter;
    }

    /// @brief Feeds @p tracker one box that moves 1 px a frame for K_STEADY_FRAMES frames, writing
    ///        into @p results, and returns the count of the last frame.
    int count_after_steady_frames(void* tracker, std::vector<CTrackResult>& results)
    {
        int count = -1;
        for (int frame = 0; frame < K_STEADY_FRAMES; ++frame)
        {
            const CDetection det{100.f + static_cast<float>(frame), 100.f, 50.f, 80.f, 0.9f, 1,
                                 nullptr, 0};
            tracker_process_c(tracker, &det, 1, results.data(), &count,
                              static_cast<int>(results.size()));
        }
        return count;
    }
} // namespace

/**
 * @brief One steady detection over ten frames comes back as one track, within the capacity.
 */
TEST(Small_TrackerBridgeTest, ProcessWritesTheTracksWithinTheCapacity)
{
    TrackerBridge::OCSORTAdapter adapter;
    std::vector<CTrackResult> results(4);

    EXPECT_EQ(count_after_steady_frames(handle_of(adapter), results), 1);
}

/**
 * @brief A null result buffer is refused without a write.
 */
TEST(Small_TrackerBridgeTest, NullResultsAreRefused)
{
    SushiTrack::CapturedLog log(SushiLog::LogLevel::Trace,
                                {TrackerBridge::K_TRACKER_BRIDGE_LOG_CATEGORY});
    TrackerBridge::OCSORTAdapter adapter;
    int count = -1;

    EXPECT_EQ(tracker_process_c(handle_of(adapter), nullptr, 0, nullptr, &count, 0),
              TRACKER_BRIDGE_INVALID_ARGUMENT);

    EXPECT_EQ(count, -1);
    EXPECT_EQ(log.count("tracker_process_c refused: tracker, results or num_results is null"), 1);
}

/**
 * @brief A null tracker is refused without a write and logs one ERROR.
 */
TEST(Small_TrackerBridgeTest, NullTrackerIsRefused)
{
    SushiTrack::CapturedLog log(SushiLog::LogLevel::Trace,
                                {TrackerBridge::K_TRACKER_BRIDGE_LOG_CATEGORY});
    std::vector<CTrackResult> results(4);
    int count = -1;

    EXPECT_EQ(tracker_process_c(nullptr, nullptr, 0, results.data(), &count, 4),
              TRACKER_BRIDGE_INVALID_ARGUMENT);

    EXPECT_EQ(count, -1);
    EXPECT_EQ(log.count("tracker_process_c refused: tracker, results or num_results is null"), 1);
    EXPECT_EQ(log.level_of("tracker_process_c refused"), SushiLog::LogLevel::Error);
}

/**
 * @brief Tracks past max_results are left out and the status says so.
 */
TEST(Small_TrackerBridgeTest, TruncationIsReportedByStatus)
{
    TrackerBridge::OCSORTAdapter adapter;
    const CDetection dets[2] = {{100.f, 100.f, 50.f, 80.f, 0.9f, 1, nullptr, 0},
                                {400.f, 100.f, 50.f, 80.f, 0.9f, 1, nullptr, 0}};
    std::vector<CTrackResult> results(1);
    int count = -1;

    const tracker_bridge_status_t status =
        tracker_process_c(handle_of(adapter), dets, 2, results.data(), &count, 1);

    EXPECT_EQ(status, TRACKER_BRIDGE_TRUNCATED);
    EXPECT_EQ(count, 1);
}

/**
 * @brief A truncated frame logs one WARN that names the written count, the track count and the
 *        capacity.
 */
TEST(Small_TrackerBridgeTest, TruncationIsLoggedOnce)
{
    SushiTrack::CapturedLog log(SushiLog::LogLevel::Trace,
                                {TrackerBridge::K_TRACKER_BRIDGE_LOG_CATEGORY});
    TrackerBridge::OCSORTAdapter adapter;
    const CDetection dets[2] = {{100.f, 100.f, 50.f, 80.f, 0.9f, 1, nullptr, 0},
                                {400.f, 100.f, 50.f, 80.f, 0.9f, 1, nullptr, 0}};
    std::vector<CTrackResult> results(1);
    int count = -1;

    tracker_process_c(handle_of(adapter), dets, 2, results.data(), &count, 1);

    EXPECT_EQ(log.count("tracker_process_c wrote 1 of 2 tracks; max_results is 1"), 1);
    EXPECT_EQ(log.level_of("tracker_process_c wrote"), SushiLog::LogLevel::Warn);
}

/**
 * @brief An empty frame still runs the tracker and returns OK with no track.
 */
TEST(Small_TrackerBridgeTest, EmptyFrameRunsAndReturnsOk)
{
    TrackerBridge::OCSORTAdapter adapter;
    std::vector<CTrackResult> results(4);
    int count = -1;

    EXPECT_EQ(tracker_process_c(handle_of(adapter), nullptr, 0, results.data(), &count, 4),
              TRACKER_BRIDGE_OK);

    EXPECT_EQ(count, 0);
}

/**
 * @brief A negative count is refused with a status and writes zero results.
 */
TEST(Small_TrackerBridgeTest, NegativeCountsAreRefused)
{
    TrackerBridge::OCSORTAdapter adapter;
    std::vector<CTrackResult> results(4);
    int count = -1;

    EXPECT_EQ(tracker_process_c(handle_of(adapter), nullptr, -1, results.data(), &count, 4),
              TRACKER_BRIDGE_INVALID_ARGUMENT);

    EXPECT_EQ(count, 0);
}

/**
 * @brief A negative count logs one ERROR that names it.
 */
TEST(Small_TrackerBridgeTest, NegativeCountsLogOneError)
{
    SushiTrack::CapturedLog log(SushiLog::LogLevel::Trace,
                                {TrackerBridge::K_TRACKER_BRIDGE_LOG_CATEGORY});
    TrackerBridge::OCSORTAdapter adapter;
    std::vector<CTrackResult> results(4);
    int count = -1;

    tracker_process_c(handle_of(adapter), nullptr, -1, results.data(), &count, 4);

    EXPECT_EQ(log.count("tracker_process_c refused: num_detections is -1"), 1);
    EXPECT_EQ(log.level_of("tracker_process_c refused"), SushiLog::LogLevel::Error);
}

/**
 * @brief Releasing a null tracker is ignored.
 */
TEST(Small_TrackerBridgeTest, DeletingANullTrackerIsIgnored)
{
    EXPECT_NO_THROW(delete_tracker(nullptr));
}
