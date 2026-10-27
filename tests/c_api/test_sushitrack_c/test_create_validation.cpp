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
 * @file test_create_validation.cpp
 * @brief Tests what the C API refuses or adjusts at create and update, and the records it writes.
 * @author Mustafa Garip
 */

#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/c_api/sushitrack_c.h>

#include <captured_log.hpp>

using SushiTrack::CapturedLog;

namespace
{
    /// @brief Returns the default parameters.
    sushitrack_params_t defaults()
    {
        sushitrack_params_t params{};
        sushitrack_get_default_params(&params);
        return params;
    }

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
} // namespace

/**
 * @brief An unregistered kalman_type is refused at create with Mahalanobis gating off, where it
 *        once crashed at the first track, and the registry's one ERROR names the type.
 */
TEST(Small_SushiTrackCCreateValidationTest, UnregisteredKalmanTypeIsRefusedAtCreate)
{
    CapturedLog log;
    sushitrack_params_t params = defaults();
    params.kalman_type = 987;
    params.enable_mahalanobis = 0;
    sushitrack_tracker_t tracker = reinterpret_cast<sushitrack_tracker_t>(0x1);

    EXPECT_EQ(sushitrack_create_ex(&params, &tracker), SUSHITRACK_INVALID_ARGUMENT);
    EXPECT_EQ(tracker, nullptr);
    EXPECT_EQ(log.count("Unregistered Kalman type requested: 987"), 1);
}

/**
 * @brief The same refusal holds with Mahalanobis gating on.
 */
TEST(Small_SushiTrackCCreateValidationTest, UnregisteredKalmanTypeIsRefusedWithGatingOn)
{
    CapturedLog log;
    sushitrack_params_t params = defaults();
    params.kalman_type = 987;
    params.enable_mahalanobis = 1;
    sushitrack_tracker_t tracker = nullptr;

    EXPECT_EQ(sushitrack_create_ex(&params, &tracker), SUSHITRACK_INVALID_ARGUMENT);
    EXPECT_EQ(tracker, nullptr);
}

/**
 * @brief Each entry point that refuses an argument logs one ERROR naming the entry and the
 *        argument.
 */
TEST(Small_SushiTrackCCreateValidationTest, RefusedArgumentsAreLoggedByName)
{
    CapturedLog log;
    sushitrack_params_t params = defaults();
    sushitrack_tracker_t tracker = sushitrack_create(&params);
    ASSERT_NE(tracker, nullptr);
    std::vector<sushitrack_track_t> out(4);
    const sushitrack_object_t object = detection(10.f, 10.f);
    int count = 0;

    EXPECT_EQ(sushitrack_create_ex(&params, nullptr), SUSHITRACK_INVALID_ARGUMENT);
    sushitrack_tracker_t refused = nullptr;
    EXPECT_EQ(sushitrack_create_ex(nullptr, &refused), SUSHITRACK_INVALID_ARGUMENT);
    EXPECT_EQ(refused, nullptr);
    EXPECT_EQ(sushitrack_update(nullptr, &object, 1, out.data(), 4, 0.04f, &count),
              SUSHITRACK_INVALID_ARGUMENT);
    EXPECT_EQ(sushitrack_update(tracker, nullptr, 1, out.data(), 4, 0.04f, &count),
              SUSHITRACK_INVALID_ARGUMENT);
    EXPECT_EQ(sushitrack_update(tracker, &object, 1, nullptr, 4, 0.04f, &count),
              SUSHITRACK_INVALID_ARGUMENT);
    EXPECT_EQ(sushitrack_update(tracker, &object, 1, out.data(), 4, 0.04f, nullptr),
              SUSHITRACK_INVALID_ARGUMENT);
    EXPECT_EQ(sushitrack_get_tracklets(nullptr, out.data(), 4, &count),
              SUSHITRACK_INVALID_ARGUMENT);
    EXPECT_EQ(sushitrack_get_tracklets(tracker, nullptr, 4, &count), SUSHITRACK_INVALID_ARGUMENT);
    sushitrack_destroy(tracker);

    EXPECT_EQ(log.count("sushitrack_create_ex refused: out_tracker is null"), 1);
    EXPECT_EQ(log.count("sushitrack_create_ex refused: params is null"), 1);
    EXPECT_EQ(log.count("sushitrack_update refused: tracker is null"), 1);
    EXPECT_EQ(log.count("sushitrack_update refused: n_objects is 1 and objects is null"), 1);
    EXPECT_EQ(log.count("sushitrack_update refused: out_tracks is null or max_tracks is 4"), 1);
    EXPECT_EQ(log.count("sushitrack_update refused: out_count is null"), 1);
    EXPECT_EQ(log.count("sushitrack_get_tracklets refused: tracker is null"), 1);
    EXPECT_EQ(log.count("sushitrack_get_tracklets refused: out_tracks is null"), 1);
    EXPECT_EQ(log.level_of("refused"), SushiLog::LogLevel::Error);
}

/**
 * @brief An unknown iou_type and a frame_rate or track_buffer the tracker clamps each log one WARN
 *        and still build a tracker.
 */
TEST(Small_SushiTrackCCreateValidationTest, AdjustedValuesWarnAndStillBuild)
{
    CapturedLog log;
    sushitrack_params_t params = defaults();
    params.iou_type = 9;
    params.frame_rate = 0;
    params.track_buffer = -3;
    sushitrack_tracker_t tracker = sushitrack_create(&params);

    EXPECT_NE(tracker, nullptr);
    sushitrack_destroy(tracker);
    EXPECT_EQ(log.count("iou_type 9 is unknown; using plain IoU"), 1);
    EXPECT_EQ(log.count("frame_rate 0 is below 1"), 1);
    EXPECT_EQ(log.count("track_buffer -3 is below 1"), 1);
    EXPECT_EQ(log.level_of("iou_type 9"), SushiLog::LogLevel::Warn);
}

/**
 * @brief With enable_reid on, detections without a feature log one WARN however many frames
 *        carry them, and the throttle's count is named.
 */
TEST(Small_SushiTrackCCreateValidationTest, FeaturelessDetectionsUnderReidWarnOnce)
{
    CapturedLog log;
    sushitrack_params_t params = defaults();
    params.enable_reid = 1;
    sushitrack_tracker_t tracker = sushitrack_create(&params);
    ASSERT_NE(tracker, nullptr);
    std::vector<sushitrack_track_t> out(4);
    const sushitrack_object_t object = detection(10.f, 10.f);
    int count = 0;

    for (int frame = 0; frame < 5; ++frame)
    {
        ASSERT_EQ(sushitrack_update(tracker, &object, 1, out.data(), 4, 0.04f, &count),
                  SUSHITRACK_SUCCESS);
    }
    sushitrack_destroy(tracker);

    EXPECT_EQ(log.count("enable_reid is on but 1 of 1 detections carry no feature"), 1);
}

/**
 * @brief A buffer too small for the tracks logs one WARN across many frames.
 */
TEST(Small_SushiTrackCCreateValidationTest, RepeatedOverflowWarnsOnce)
{
    CapturedLog log;
    sushitrack_params_t params = defaults();
    sushitrack_tracker_t tracker = sushitrack_create(&params);
    ASSERT_NE(tracker, nullptr);
    std::vector<sushitrack_track_t> out(1);
    const std::vector<sushitrack_object_t> objects = {detection(10.f, 10.f),
                                                      detection(300.f, 10.f)};
    int count = 0;

    for (int frame = 0; frame < 5; ++frame)
    {
        ASSERT_EQ(sushitrack_update(tracker, objects.data(), 2, out.data(), 1, 0.04f, &count),
                  SUSHITRACK_BUFFER_OVERFLOW);
    }
    sushitrack_destroy(tracker);

    EXPECT_EQ(log.count("exceed the output buffer of 1"), 1);
}
