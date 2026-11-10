/**************************************************************************/
/* test_create_validation.cpp                                             */
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
 * @file test_create_validation.cpp
 * @brief Tests what the C API refuses or adjusts at create and update, and the records it writes.
 * @author Mustafa Garip
 */

#include <cstddef>
#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/c_api/create_tracker.hpp>
#include <SushiTrack/c_api/sushitrack_c.h>

#include <captured_log.hpp>
#include <support/default_params.hpp>
#include <support/kalman_registry_builders.hpp>

using SushiTrack::CapturedLog;
using TestSupport::default_params;

namespace
{
    /// @brief A kalman_type no registry holds.
    constexpr int K_UNREGISTERED_KALMAN_TYPE = 987;

    /// @brief Frames the repeated-warning tests run.
    constexpr int K_REPEATED_FRAMES = 5;

    /// @brief Returns a 50 x 80 detection at (x, y) with score 0.9.
    sushitrack_object_t detection(float x, float y)
    {
        sushitrack_object_t object{};
        object.x = x;
        object.y = y;
        object.width = 50.0f;
        object.height = 80.0f;
        object.score = 0.9f;
        object.label = 1;
        return object;
    }

    /// @brief Returns the defaults with an unregistered kalman_type and Mahalanobis gating set to
    ///        @p gating.
    sushitrack_params_t unregistered_type_params(int gating)
    {
        sushitrack_params_t params = default_params();
        params.kalman_type = K_UNREGISTERED_KALMAN_TYPE;
        params.enable_mahalanobis = gating;
        return params;
    }

    /**
     * @brief Applies @p frames updates of @p objects, each into a buffer of @p capacity, and
     *        returns the status of every update in order.
     */
    std::vector<sushitrack_status_t>
    update_repeatedly(sushitrack_tracker_t tracker, const std::vector<sushitrack_object_t>& objects,
                      int capacity, int frames)
    {
        std::vector<sushitrack_track_t> out(static_cast<std::size_t>(capacity));
        std::vector<sushitrack_status_t> statuses;
        int count = 0;
        for (int frame = 0; frame < frames; ++frame)
        {
            statuses.push_back(sushitrack_update(tracker, objects.data(),
                                                 static_cast<int>(objects.size()), out.data(),
                                                 capacity, 0.04f, &count));
        }
        return statuses;
    }
} // namespace

/**
 * @brief An unregistered kalman_type is refused at create with INVALID_ARGUMENT and no handle,
 *        with Mahalanobis gating off, where it once crashed at the first track, and with it on.
 */
TEST(Small_SushiTrackCCreateValidationTest, UnregisteredKalmanTypeIsRefusedAtCreate)
{
    const auto registry = TestSupport::library_kalman_registry();
    const sushitrack_params_t gating_off = unregistered_type_params(0);
    const sushitrack_params_t gating_on = unregistered_type_params(1);
    sushitrack_tracker_t off_tracker = reinterpret_cast<sushitrack_tracker_t>(0x1);
    sushitrack_tracker_t on_tracker = reinterpret_cast<sushitrack_tracker_t>(0x1);

    EXPECT_EQ(SushiTrack::create_tracker(&gating_off, *registry, &off_tracker),
              SUSHITRACK_INVALID_ARGUMENT);
    EXPECT_EQ(SushiTrack::create_tracker(&gating_on, *registry, &on_tracker),
              SUSHITRACK_INVALID_ARGUMENT);

    EXPECT_EQ(off_tracker, nullptr);
    EXPECT_EQ(on_tracker, nullptr);
}

/**
 * @brief The refusal of an unregistered kalman_type logs one ERROR that names the type.
 */
TEST(Small_SushiTrackCCreateValidationTest, UnregisteredKalmanTypeLogsOneErrorNamingTheType)
{
    CapturedLog log;
    const auto registry = TestSupport::library_kalman_registry();
    const sushitrack_params_t params = unregistered_type_params(0);
    sushitrack_tracker_t tracker = nullptr;

    EXPECT_EQ(SushiTrack::create_tracker(&params, *registry, &tracker),
              SUSHITRACK_INVALID_ARGUMENT);

    EXPECT_EQ(log.count("Unregistered Kalman type requested: 987"), 1);
    EXPECT_EQ(log.level_of("Unregistered Kalman type requested"), SushiLog::LogLevel::Error);
}

/**
 * @brief sushitrack_create_ex logs one ERROR for each argument it refuses, naming the entry and
 *        the argument.
 */
TEST(Small_SushiTrackCCreateValidationTest, CreateExRefusalsAreLoggedByArgument)
{
    CapturedLog log;
    const sushitrack_params_t params = default_params();
    sushitrack_tracker_t refused = nullptr;

    sushitrack_create_ex(&params, nullptr);
    sushitrack_create_ex(nullptr, &refused);

    EXPECT_EQ(log.count("sushitrack_create_ex refused: out_tracker is null"), 1);
    EXPECT_EQ(log.count("sushitrack_create_ex refused: params is null"), 1);
    EXPECT_EQ(log.level_of("sushitrack_create_ex refused"), SushiLog::LogLevel::Error);
}

/**
 * @brief sushitrack_update logs one ERROR for each argument it refuses, naming the entry and the
 *        argument.
 */
TEST(Small_SushiTrackCCreateValidationTest, UpdateRefusalsAreLoggedByArgument)
{
    CapturedLog log;
    const sushitrack_params_t params = default_params();
    sushitrack_tracker_t tracker = sushitrack_create(&params);
    ASSERT_NE(tracker, nullptr);
    std::vector<sushitrack_track_t> out(4);
    const sushitrack_object_t object = detection(10.f, 10.f);
    int count = 0;

    sushitrack_update(nullptr, &object, 1, out.data(), 4, 0.04f, &count);
    sushitrack_update(tracker, nullptr, 1, out.data(), 4, 0.04f, &count);
    sushitrack_update(tracker, &object, 1, nullptr, 4, 0.04f, &count);
    sushitrack_update(tracker, &object, 1, out.data(), 4, 0.04f, nullptr);
    sushitrack_destroy(tracker);

    EXPECT_EQ(log.count("sushitrack_update refused: tracker is null"), 1);
    EXPECT_EQ(log.count("sushitrack_update refused: n_objects is 1 and objects is null"), 1);
    EXPECT_EQ(log.count("sushitrack_update refused: out_tracks is null or max_tracks is 4"), 1);
    EXPECT_EQ(log.count("sushitrack_update refused: out_count is null"), 1);
    EXPECT_EQ(log.level_of("sushitrack_update refused"), SushiLog::LogLevel::Error);
}

/**
 * @brief sushitrack_get_tracklets logs one ERROR for each argument it refuses, naming the entry
 *        and the argument.
 */
TEST(Small_SushiTrackCCreateValidationTest, GetTrackletsRefusalsAreLoggedByArgument)
{
    CapturedLog log;
    const sushitrack_params_t params = default_params();
    sushitrack_tracker_t tracker = sushitrack_create(&params);
    ASSERT_NE(tracker, nullptr);
    std::vector<sushitrack_track_t> out(4);
    int count = 0;

    sushitrack_get_tracklets(nullptr, out.data(), 4, &count);
    sushitrack_get_tracklets(tracker, nullptr, 4, &count);
    sushitrack_destroy(tracker);

    EXPECT_EQ(log.count("sushitrack_get_tracklets refused: tracker is null"), 1);
    EXPECT_EQ(log.count("sushitrack_get_tracklets refused: out_tracks is null"), 1);
    EXPECT_EQ(log.level_of("sushitrack_get_tracklets refused"), SushiLog::LogLevel::Error);
}

/**
 * @brief An unknown iou_type logs one WARN and still builds a tracker.
 */
TEST(Small_SushiTrackCCreateValidationTest, UnknownIouTypeWarnsAndStillBuilds)
{
    CapturedLog log;
    sushitrack_params_t params = default_params();
    params.iou_type = 9;

    sushitrack_tracker_t tracker = sushitrack_create(&params);

    EXPECT_NE(tracker, nullptr);
    sushitrack_destroy(tracker);
    EXPECT_EQ(log.count("iou_type 9 is unknown; using plain IoU"), 1);
    EXPECT_EQ(log.level_of("iou_type 9"), SushiLog::LogLevel::Warn);
}

/**
 * @brief A frame_rate of 0 logs one WARN and still builds a tracker.
 */
TEST(Small_SushiTrackCCreateValidationTest, ZeroFrameRateWarnsAndStillBuilds)
{
    CapturedLog log;
    sushitrack_params_t params = default_params();
    params.frame_rate = 0;

    sushitrack_tracker_t tracker = sushitrack_create(&params);

    EXPECT_NE(tracker, nullptr);
    sushitrack_destroy(tracker);
    EXPECT_EQ(log.count("frame_rate 0 is below 1"), 1);
    EXPECT_EQ(log.level_of("frame_rate 0"), SushiLog::LogLevel::Warn);
}

/**
 * @brief A track_buffer below 1 logs one WARN and still builds a tracker.
 */
TEST(Small_SushiTrackCCreateValidationTest, NegativeTrackBufferWarnsAndStillBuilds)
{
    CapturedLog log;
    sushitrack_params_t params = default_params();
    params.track_buffer = -3;

    sushitrack_tracker_t tracker = sushitrack_create(&params);

    EXPECT_NE(tracker, nullptr);
    sushitrack_destroy(tracker);
    EXPECT_EQ(log.count("track_buffer -3 is below 1"), 1);
    EXPECT_EQ(log.level_of("track_buffer -3"), SushiLog::LogLevel::Warn);
}

/**
 * @brief With enable_reid on, detections without a feature log one WARN however many frames
 *        carry them, and the throttle's count is named.
 */
TEST(Small_SushiTrackCCreateValidationTest, FeaturelessDetectionsUnderReidWarnOnce)
{
    CapturedLog log;
    sushitrack_params_t params = default_params();
    params.enable_reid = 1;
    sushitrack_tracker_t tracker = sushitrack_create(&params);
    ASSERT_NE(tracker, nullptr);

    const std::vector<sushitrack_status_t> statuses =
        update_repeatedly(tracker, {detection(10.f, 10.f)}, 4, K_REPEATED_FRAMES);
    sushitrack_destroy(tracker);

    EXPECT_EQ(statuses, std::vector<sushitrack_status_t>(K_REPEATED_FRAMES, SUSHITRACK_SUCCESS));
    EXPECT_EQ(log.count("enable_reid is on but 1 of 1 detections carry no feature"), 1);
}

/**
 * @brief A buffer too small for the tracks logs one WARN across many frames.
 */
TEST(Small_SushiTrackCCreateValidationTest, RepeatedOverflowWarnsOnce)
{
    CapturedLog log;
    const sushitrack_params_t params = default_params();
    sushitrack_tracker_t tracker = sushitrack_create(&params);
    ASSERT_NE(tracker, nullptr);

    const std::vector<sushitrack_status_t> statuses = update_repeatedly(
        tracker, {detection(10.f, 10.f), detection(300.f, 10.f)}, 1, K_REPEATED_FRAMES);
    sushitrack_destroy(tracker);

    EXPECT_EQ(statuses,
              std::vector<sushitrack_status_t>(K_REPEATED_FRAMES, SUSHITRACK_BUFFER_OVERFLOW));
    EXPECT_EQ(log.count("exceed the output buffer of 1"), 1);
}
