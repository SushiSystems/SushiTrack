/**************************************************************************/
/* test_error_handling.cpp                                                */
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
 * @file test_error_handling.cpp
 * @brief Tests how the C API handles invalid arguments, small buffers and failures.
 * @author Mustafa Garip
 */

#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include <SushiLog/log/log_calls.hpp>
#include <SushiLog/log/log_record.hpp>
#include <gtest/gtest.h>

#include <SushiTrack/c_api/sushitrack_c.h>
#include <SushiTrack/c_api/c_api_log_category.hpp>
#include <SushiTrack/tracking/tracking_log_category.hpp>

#include <c_detection_builders.hpp>

namespace
{

    /**
     * @brief Constructs a benign detection with default geometry and the given score.
     * @param x     Top-left horizontal coordinate.
     * @param y     Top-left vertical coordinate.
     * @param score Detection confidence score in [0, 1].
     * @return Populated sushitrack_object_t.
     */
    sushitrack_object_t make_det(float x, float y, float score = 0.9f)
    {
        sushitrack_object_t o{};
        o.x = x;
        o.y = y;
        o.width = 50.0f;
        o.height = 100.0f;
        o.score = score;
        o.label = 1;
        return o;
    }

} // namespace

// Small_SushiTrackCInvalidArgumentTest

/**
 * @brief Fixture providing default params for invalid-argument tests.
 */
class Small_SushiTrackCInvalidArgumentTest : public ::testing::Test
{
    protected:
        sushitrack_params_t params_{};
        sushitrack_tracker_t hdl_ = nullptr;

        void SetUp() override
        {
            sushitrack_get_default_params(&params_);
            hdl_ = sushitrack_create(&params_);
            ASSERT_NE(hdl_, nullptr);
        }

        void TearDown() override
        {
            if (hdl_ != nullptr)
                sushitrack_destroy(hdl_);
        }
};

/**
 * @brief GetDefaultParams tolerates a null pointer and changes nothing: the defaults it fills
 *        afterwards still build a tracker.
 *
 * The C contract documents the parameter only as the output destination, so
 * a null input is considered an unintentional misuse.
 */
TEST(Small_SushiTrackCInvalidArgumentStandaloneTest, GetDefaultParamsNullPointerChangesNothing)
{
    sushitrack_get_default_params(nullptr);
    sushitrack_params_t params{};
    sushitrack_get_default_params(&params);

    sushitrack_tracker_t hdl = sushitrack_create(&params);

    EXPECT_NE(hdl, nullptr);
    sushitrack_destroy(hdl);
}

/**
 * @brief sushitrack_create with a null params pointer overlays the runtime config on the
 *        defaults (sushitrack_create_from_config): the first detection starts a Tentative track,
 *        where the defaults, which turn tentative tracking off, start a Tracked one.
 *
 * The overlay is the file SUSHITRACK_CONFIG names, assets/sushitrack.json under the test runner,
 * which sets enable_tentative to 1.
 */
TEST(Medium_SushiTrackCInvalidArgumentStandaloneTest, CreateWithNullParamsOverlaysTheConfig)
{
    sushitrack_tracker_t hdl = sushitrack_create(nullptr);
    ASSERT_NE(hdl, nullptr);
    std::vector<sushitrack_track_t> out(4);
    sushitrack_object_t d = make_det(10.0f, 20.0f);

    EXPECT_EQ(CDetections::update_frame(hdl, &d, 1, out.data(), 4, 0.04f), 1);

    EXPECT_EQ(out[0].state, CDetections::K_TENTATIVE);
    EXPECT_EQ(out[0].is_confirmed, 0);
    sushitrack_destroy(hdl);
}

/**
 * @brief CreateEx with a null params pointer reports INVALID_ARGUMENT and sets the out handle to
 *        null.
 */
TEST(Small_SushiTrackCInvalidArgumentStandaloneTest, CreateExNullParamsSetsTheOutHandleToNull)
{
    sushitrack_tracker_t hdl =
        reinterpret_cast<sushitrack_tracker_t>(static_cast<uintptr_t>(0xDEADBEEFul));
    sushitrack_status_t status = sushitrack_create_ex(nullptr, &hdl);
    EXPECT_EQ(status, SUSHITRACK_INVALID_ARGUMENT);
    EXPECT_EQ(hdl, nullptr);
}

/**
 * @brief Update with a null tracker handle reports INVALID_ARGUMENT and writes no track.
 */
TEST_F(Small_SushiTrackCInvalidArgumentTest, UpdateWithNullHandleReturnsInvalidArgument)
{
    std::vector<sushitrack_track_t> out(4);
    sushitrack_object_t d = make_det(10.0f, 20.0f);
    int count = -1;
    EXPECT_EQ(
        sushitrack_update(nullptr, &d, 1, out.data(), static_cast<int>(out.size()), 0.04f, &count),
        SUSHITRACK_INVALID_ARGUMENT);
    EXPECT_EQ(count, 0);
}

/**
 * @brief Update with a null detection array and a positive n_objects reports INVALID_ARGUMENT.
 */
TEST_F(Small_SushiTrackCInvalidArgumentTest,
       UpdateWithNullDetectionsAndPositiveCountReturnsInvalidArgument)
{
    std::vector<sushitrack_track_t> out(4);
    int count = -1;
    EXPECT_EQ(sushitrack_update(hdl_, nullptr, 1, out.data(), static_cast<int>(out.size()), 0.04f,
                                &count),
              SUSHITRACK_INVALID_ARGUMENT);
    EXPECT_EQ(count, 0);
}

/**
 * @brief Update with a null output buffer and a positive max_tracks reports INVALID_ARGUMENT.
 */
TEST_F(Small_SushiTrackCInvalidArgumentTest,
       UpdateWithNullOutputBufferAndPositiveMaxReturnsInvalidArgument)
{
    sushitrack_object_t d = make_det(10.0f, 20.0f);
    int count = -1;
    EXPECT_EQ(sushitrack_update(hdl_, &d, 1, nullptr, 4, 0.04f, &count),
              SUSHITRACK_INVALID_ARGUMENT);
    EXPECT_EQ(count, 0);
}

/**
 * @brief Update with a negative n_objects reports INVALID_ARGUMENT.
 */
TEST_F(Small_SushiTrackCInvalidArgumentTest, UpdateWithNegativeObjectCountReturnsInvalidArgument)
{
    std::vector<sushitrack_track_t> out(4);
    sushitrack_object_t d = make_det(10.0f, 20.0f);
    int count = -1;
    EXPECT_EQ(
        sushitrack_update(hdl_, &d, -1, out.data(), static_cast<int>(out.size()), 0.04f, &count),
        SUSHITRACK_INVALID_ARGUMENT);
    EXPECT_EQ(count, 0);
}

/**
 * @brief Update with a negative max_tracks reports INVALID_ARGUMENT.
 */
TEST_F(Small_SushiTrackCInvalidArgumentTest, UpdateWithNegativeMaxTracksReturnsInvalidArgument)
{
    std::vector<sushitrack_track_t> out(4);
    sushitrack_object_t d = make_det(10.0f, 20.0f);
    int count = -1;
    EXPECT_EQ(sushitrack_update(hdl_, &d, 1, out.data(), -1, 0.04f, &count),
              SUSHITRACK_INVALID_ARGUMENT);
    EXPECT_EQ(count, 0);
}

/**
 * @brief Update with a null out_count reports INVALID_ARGUMENT.
 */
TEST_F(Small_SushiTrackCInvalidArgumentTest, UpdateWithNullOutCountReturnsInvalidArgument)
{
    std::vector<sushitrack_track_t> out(4);
    sushitrack_object_t d = make_det(10.0f, 20.0f);
    EXPECT_EQ(
        sushitrack_update(hdl_, &d, 1, out.data(), static_cast<int>(out.size()), 0.04f, nullptr),
        SUSHITRACK_INVALID_ARGUMENT);
}

/**
 * @brief GetTracklets with a null tracker handle reports INVALID_ARGUMENT and writes no track.
 */
TEST_F(Small_SushiTrackCInvalidArgumentTest, GetTrackletsWithNullHandleReturnsInvalidArgument)
{
    std::vector<sushitrack_track_t> out(4);
    int count = -1;
    EXPECT_EQ(sushitrack_get_tracklets(nullptr, out.data(), static_cast<int>(out.size()), &count),
              SUSHITRACK_INVALID_ARGUMENT);
    EXPECT_EQ(count, 0);
}

/**
 * @brief GetTracklets with a null out_count reports INVALID_ARGUMENT.
 */
TEST_F(Small_SushiTrackCInvalidArgumentTest, GetTrackletsWithNullOutCountReturnsInvalidArgument)
{
    std::vector<sushitrack_track_t> out(4);
    EXPECT_EQ(sushitrack_get_tracklets(hdl_, out.data(), static_cast<int>(out.size()), nullptr),
              SUSHITRACK_INVALID_ARGUMENT);
}

// Small_SushiTrackCParameterRangeTest

/**
 * @brief Fixture for parameter-range validation tests.
 *
 * Each test mutates a single field of an otherwise default params struct and
 * checks that the tracker either refuses creation or behaves defensively (no
 * crash, no spurious tracks).
 */
class Small_SushiTrackCParameterRangeTest : public ::testing::Test
{
    protected:
        sushitrack_params_t params_{};

        void SetUp() override { sushitrack_get_default_params(&params_); }
};

/**
 * @brief A frame_rate of 0 is clamped to 1: with time_aware on and track_buffer 1, a lost track
 *        expires once its lost time exceeds 1 s.
 *
 * Lost time is 0.5 s after the first missed frame, 0.9 s after the second (0.5 + 0.4) and 1.1 s
 * after the third (0.9 + 0.2). The limit is track_buffer / frame_rate + 0.0001 = 1.0001 s, so the
 * track is Lost at 0.9 s and Removed at 1.1 s. A frame_rate left at 0 never expires it.
 */
TEST_F(Small_SushiTrackCParameterRangeTest, ZeroFrameRateIsClampedToOne)
{
    params_.frame_rate = 0;
    params_.time_aware = 1;
    params_.track_buffer = 1;
    sushitrack_tracker_t hdl = nullptr;
    ASSERT_EQ(sushitrack_create_ex(&params_, &hdl), SUSHITRACK_SUCCESS);
    std::vector<sushitrack_track_t> out(4);
    sushitrack_object_t d = make_det(10.0f, 20.0f);

    EXPECT_EQ(CDetections::update_frame(hdl, &d, 1, out.data(), 4, 0.04f), 1);
    EXPECT_EQ(out[0].state, CDetections::K_TRACKED);
    EXPECT_EQ(CDetections::update_frame(hdl, nullptr, 0, out.data(), 4, 0.5f), 1);
    EXPECT_EQ(out[0].state, CDetections::K_LOST);
    EXPECT_EQ(CDetections::update_frame(hdl, nullptr, 0, out.data(), 4, 0.4f), 1);
    EXPECT_EQ(out[0].state, CDetections::K_LOST);
    EXPECT_EQ(CDetections::update_frame(hdl, nullptr, 0, out.data(), 4, 0.2f), 1);
    EXPECT_EQ(out[0].state, CDetections::K_REMOVED);

    sushitrack_destroy(hdl);
}

/**
 * @brief A negative frame_rate is refused with INVALID_ARGUMENT and no handle, not clamped to 1.
 */
TEST_F(Small_SushiTrackCParameterRangeTest, NegativeFrameRateIsRefused)
{
    params_.frame_rate = -25;
    sushitrack_tracker_t hdl = reinterpret_cast<sushitrack_tracker_t>(0x1);

    EXPECT_EQ(sushitrack_create_ex(&params_, &hdl), SUSHITRACK_INVALID_ARGUMENT);
    EXPECT_EQ(hdl, nullptr);
}

/**
 * @brief A negative track_buffer is clamped to 1: with time_aware off, a lost track is removed on
 *        the second frame after its last match, not kept for the default 30.
 *
 * The track matches in frame 1 and turns Lost in frame 2. In frame 3 its age is 3 - 1 = 2 frames
 * and the frame policy expires an age above the buffer: 2 > 1. With track_buffer 2 it would stay.
 */
TEST_F(Small_SushiTrackCParameterRangeTest, NegativeTrackBufferIsClampedToOne)
{
    params_.track_buffer = -5;
    sushitrack_tracker_t hdl = nullptr;
    ASSERT_EQ(sushitrack_create_ex(&params_, &hdl), SUSHITRACK_SUCCESS);
    std::vector<sushitrack_track_t> out(4);
    sushitrack_object_t d = make_det(10.0f, 20.0f);

    EXPECT_EQ(CDetections::update_frame(hdl, &d, 1, out.data(), 4, 0.04f), 1);
    EXPECT_EQ(CDetections::update_frame(hdl, nullptr, 0, out.data(), 4, 0.04f), 1);
    EXPECT_EQ(out[0].state, CDetections::K_LOST);
    EXPECT_EQ(CDetections::update_frame(hdl, nullptr, 0, out.data(), 4, 0.04f), 1);
    EXPECT_EQ(out[0].state, CDetections::K_REMOVED);

    sushitrack_destroy(hdl);
}

/**
 * @brief match_thresh greater than 1.0 (impossible cost) is tolerated but must
 *        not match anything — the tracker should still create cleanly.
 */
TEST_F(Small_SushiTrackCParameterRangeTest, MatchThreshGreaterThanOneIsTolerated)
{
    params_.match_thresh = 5.0f;
    sushitrack_tracker_t hdl = sushitrack_create(&params_);
    ASSERT_NE(hdl, nullptr);
    sushitrack_destroy(hdl);
}

/**
 * @brief With track_thresh above high_thresh, a detection scoring between them is a low-score
 *        detection and starts no track.
 *
 * A detection joins the high pool when its score reaches track_thresh. The score 0.5 is below
 * 0.9, so it goes to the low pool, and new tracks start only from the high pool: the first frame
 * writes 0 tracks.
 */
TEST_F(Small_SushiTrackCParameterRangeTest, TrackThreshAboveHighThreshStartsNoTrackFromALowScore)
{
    params_.track_thresh = 0.9f;
    params_.high_thresh = 0.1f;
    sushitrack_tracker_t hdl = sushitrack_create(&params_);
    ASSERT_NE(hdl, nullptr);
    std::vector<sushitrack_track_t> out(4);
    sushitrack_object_t d = make_det(10.0f, 20.0f, 0.5f);
    int count = -1;

    EXPECT_EQ(
        sushitrack_update(hdl, &d, 1, out.data(), static_cast<int>(out.size()), 0.04f, &count),
        SUSHITRACK_SUCCESS);

    EXPECT_EQ(count, 0);
    sushitrack_destroy(hdl);
}

// Small_SushiTrackCBufferCapacityTest

/**
 * @brief Fixture for output-buffer capacity behaviour.
 */
class Small_SushiTrackCBufferCapacityTest : public ::testing::Test
{
    protected:
        sushitrack_tracker_t hdl_ = nullptr;
        sushitrack_params_t params_ = {};

        void SetUp() override
        {
            sushitrack_get_default_params(&params_);
            params_.enable_tentative = 0;
            params_.time_aware = 1;
            params_.track_thresh = 0.1f;
            params_.high_thresh = 0.2f;
            params_.match_thresh = 0.9f;
            hdl_ = sushitrack_create(&params_);
            ASSERT_NE(hdl_, nullptr);
        }

        void TearDown() override { sushitrack_destroy(hdl_); }
};

/**
 * @brief A null buffer of capacity zero is accepted: the frame is applied and the track it makes
 *        is reported through BUFFER_OVERFLOW with nothing written.
 */
TEST_F(Small_SushiTrackCBufferCapacityTest, ZeroMaxTracksAppliesTheFrameAndReportsOverflow)
{
    sushitrack_object_t d = make_det(10.0f, 20.0f);
    int count = -1;
    EXPECT_EQ(sushitrack_update(hdl_, &d, 1, nullptr, 0, 0.04f, &count),
              SUSHITRACK_BUFFER_OVERFLOW);
    EXPECT_EQ(count, 0);

    std::vector<sushitrack_track_t> out(4);
    EXPECT_EQ(sushitrack_get_tracklets(hdl_, out.data(), static_cast<int>(out.size()), &count),
              SUSHITRACK_SUCCESS);
    EXPECT_EQ(count, 1);
}

/**
 * @brief More tracks than the buffer holds fill it and return BUFFER_OVERFLOW.
 */
TEST_F(Small_SushiTrackCBufferCapacityTest, TracksPastTheCapacityReturnBufferOverflow)
{
    constexpr int K_DETECTIONS = 12;
    constexpr int K_CAPACITY = 3;

    std::vector<sushitrack_object_t> dets;
    dets.reserve(K_DETECTIONS);
    for (int i = 0; i < K_DETECTIONS; ++i)
        dets.push_back(make_det(50.0f + 200.0f * i, 50.0f + 200.0f * i));

    std::vector<sushitrack_track_t> out(K_CAPACITY);
    int count = -1;
    EXPECT_EQ(
        sushitrack_update(hdl_, dets.data(), K_DETECTIONS, out.data(), K_CAPACITY, 0.04f, &count),
        SUSHITRACK_BUFFER_OVERFLOW);
    EXPECT_EQ(count, K_CAPACITY);
}

/**
 * @brief Tracks that fit the buffer return SUCCESS with every track written.
 */
TEST_F(Small_SushiTrackCBufferCapacityTest, TracksWithinTheCapacityReturnSuccess)
{
    constexpr int K_DETECTIONS = 3;

    std::vector<sushitrack_object_t> dets;
    for (int i = 0; i < K_DETECTIONS; ++i)
        dets.push_back(make_det(50.0f + 200.0f * i, 50.0f + 200.0f * i));

    std::vector<sushitrack_track_t> out(K_DETECTIONS);
    int count = -1;
    EXPECT_EQ(
        sushitrack_update(hdl_, dets.data(), K_DETECTIONS, out.data(), K_DETECTIONS, 0.04f, &count),
        SUSHITRACK_SUCCESS);
    EXPECT_EQ(count, K_DETECTIONS);
}

// Small_SushiTrackCLifecycleSafetyTest

/**
 * @brief Destroying a null handle is a safe no-op: a live tracker still updates afterwards, so
 *        consumers can call Destroy unconditionally from cleanup paths.
 */
TEST(Small_SushiTrackCLifecycleSafetyTest, DestroyNullHandleLeavesALiveTrackerWorking)
{
    sushitrack_params_t params{};
    sushitrack_get_default_params(&params);
    sushitrack_tracker_t hdl = sushitrack_create(&params);
    ASSERT_NE(hdl, nullptr);
    std::vector<sushitrack_track_t> out(4);
    sushitrack_object_t d = make_det(10.0f, 20.0f);

    sushitrack_destroy(nullptr);

    EXPECT_EQ(CDetections::update_frame(hdl, &d, 1, out.data(), 4, 0.04f), 1);
    sushitrack_destroy(hdl);
}

/**
 * @brief A tracker can be created again after another was destroyed. Leaks are the sanitizers'
 *        promise, not this test's.
 */
TEST(Small_SushiTrackCLifecycleSafetyTest, CreateAfterDestroySucceeds)
{
    sushitrack_params_t params{};
    sushitrack_get_default_params(&params);
    sushitrack_tracker_t first = sushitrack_create(&params);
    ASSERT_NE(first, nullptr);
    sushitrack_destroy(first);

    sushitrack_tracker_t second = sushitrack_create(&params);

    EXPECT_NE(second, nullptr);
    sushitrack_destroy(second);
}

// Medium_SushiTrackCBoundaryExceptionTest

/**
 * @brief Fixture that owns one tracker and installs a SushiLog sink that throws on every record.
 *
 * SushiLog contains an exception from its sink, so no C entry point sees one: each test checks
 * that a call keeps its normal result while every record it logs throws in the sink.
 */
class Medium_SushiTrackCBoundaryExceptionTest : public ::testing::Test
{
    protected:
        sushitrack_tracker_t hdl_ = nullptr;
        sushitrack_params_t params_ = {};

        void SetUp() override
        {
            sushitrack_get_default_params(&params_);
            params_.enable_tentative = 0;
            params_.time_aware = 1;
            params_.track_thresh = 0.1f;
            params_.high_thresh = 0.2f;
            params_.match_thresh = 0.9f;
            hdl_ = sushitrack_create(&params_);
            ASSERT_NE(hdl_, nullptr);

            SushiLog::set_log_synchronous(true);
            SushiLog::set_log_level(SushiTrack::K_C_API_LOG_CATEGORY, SushiLog::LogLevel::Trace);
            SushiLog::set_log_level(SushiTrack::K_TRACKING_LOG_CATEGORY,
                                    SushiLog::LogLevel::Trace);
            SushiLog::set_log_sink(
                [](const SushiLog::LogRecord&)
                {
                    throw 42;
                });
        }

        void TearDown() override
        {
            SushiLog::set_log_sink(nullptr);
            SushiLog::set_log_level(SushiTrack::K_C_API_LOG_CATEGORY, SushiLog::LogLevel::Warn);
            SushiLog::set_log_level(SushiTrack::K_TRACKING_LOG_CATEGORY,
                                    SushiLog::LogLevel::Warn);
            SushiLog::set_log_synchronous(false);
            if (hdl_ != nullptr)
                sushitrack_destroy(hdl_);
        }
};

/**
 * @brief sushitrack_update returns its track while the sink throws.
 */
TEST_F(Medium_SushiTrackCBoundaryExceptionTest, UpdateSurvivesAThrowingSink)
{
    std::vector<sushitrack_track_t> out(4);
    sushitrack_object_t d = make_det(10.0f, 20.0f);

    int count = -1;
    sushitrack_status_t status = SUSHITRACK_RUNTIME_ERROR;
    EXPECT_NO_THROW(status = sushitrack_update(hdl_, &d, 1, out.data(),
                                               static_cast<int>(out.size()), 0.04f, &count));
    EXPECT_EQ(status, SUSHITRACK_SUCCESS);
    EXPECT_EQ(count, 1);
}

/**
 * @brief sushitrack_get_tracklets truncates to its capacity, logging the warning, while the sink
 *        throws.
 */
TEST_F(Medium_SushiTrackCBoundaryExceptionTest, GetTrackletsSurvivesAThrowingSink)
{
    std::vector<sushitrack_object_t> dets;
    for (int i = 0; i < 3; ++i)
        dets.push_back(make_det(50.0f + 400.0f * i, 50.0f + 400.0f * i));

    std::vector<sushitrack_track_t> out(8);
    int count = -1;
    EXPECT_EQ(sushitrack_update(hdl_, dets.data(), 3, out.data(), static_cast<int>(out.size()),
                                0.04f, &count),
              SUSHITRACK_SUCCESS);

    sushitrack_status_t status = SUSHITRACK_RUNTIME_ERROR;
    EXPECT_NO_THROW(status = sushitrack_get_tracklets(hdl_, out.data(), 1, &count));
    EXPECT_EQ(status, SUSHITRACK_BUFFER_OVERFLOW);
    EXPECT_EQ(count, 1);
}

/**
 * @brief sushitrack_create_ex succeeds while the sink throws.
 */
TEST_F(Medium_SushiTrackCBoundaryExceptionTest, CreateExSurvivesAThrowingSink)
{
    sushitrack_tracker_t created = nullptr;
    sushitrack_status_t status = SUSHITRACK_RUNTIME_ERROR;

    EXPECT_NO_THROW(status = sushitrack_create_ex(&params_, &created));
    EXPECT_EQ(status, SUSHITRACK_SUCCESS);
    EXPECT_NE(created, nullptr);

    if (created != nullptr)
        sushitrack_destroy(created);
}

/**
 * @brief sushitrack_destroy releases the tracker while the sink throws.
 */
TEST_F(Medium_SushiTrackCBoundaryExceptionTest, DestroySurvivesAThrowingSink)
{
    EXPECT_NO_THROW(sushitrack_destroy(hdl_));
    hdl_ = nullptr;
}
