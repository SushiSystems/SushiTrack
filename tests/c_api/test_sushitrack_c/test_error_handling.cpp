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
 * @file test_error_handling.cpp
 * @brief Tests how the C API handles invalid arguments, small buffers and failures.
 * @author Mustafa Garip
 */

#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

#include <SushiLog/log/log_calls.hpp>
#include <SushiLog/log/log_record.hpp>
#include <gtest/gtest.h>

#include <SushiTrack/c_api/sushitrack_c.h>
#include <SushiTrack/c_api/c_api_log_category.hpp>
#include <SushiTrack/filtering/kalman_filter_registry.hpp>
#include <SushiTrack/tracking/tracking_log_category.hpp>

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
 * @brief GetDefaultParams must tolerate a null pointer without crashing.
 *
 * The C contract documents the parameter only as the output destination, so
 * a null input is considered an unintentional misuse. The implementation must
 * not segfault; passing the call is the success criterion.
 */
TEST(Small_SushiTrackCInvalidArgumentStandaloneTest, GetDefaultParamsNullPointerDoesNotCrash)
{
    sushitrack_get_default_params(nullptr);
    SUCCEED();
}

/**
 * @brief sushitrack_create with a null params pointer auto-loads the runtime
 *        config (sushitrack_create_from_config) and returns a valid handle.
 *
 * A null params pointer is no longer a misuse: it is the documented opt-in to
 * the library-owned config path (defaults overlaid with sushitrack.json when one
 */
TEST(Small_SushiTrackCInvalidArgumentStandaloneTest, CreateWithNullParamsAutoLoadsConfig)
{
    sushitrack_tracker_t hdl = sushitrack_create(nullptr);
    EXPECT_NE(hdl, nullptr);
    if (hdl != nullptr)
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
 * @brief Zero frame_rate must either be rejected at construction or, if the
 *        tracker is created, must not produce promoted tracks (the timing
 *        denominator is undefined).
 */
TEST_F(Small_SushiTrackCParameterRangeTest, ZeroFrameRateIsRejectedOrInert)
{
    params_.frame_rate = 0;
    sushitrack_tracker_t hdl = nullptr;
    sushitrack_status_t s = sushitrack_create_ex(&params_, &hdl);

    if (s == SUSHITRACK_INVALID_ARGUMENT)
    {
        EXPECT_EQ(hdl, nullptr);
        return;
    }

    ASSERT_NE(hdl, nullptr);
    std::vector<sushitrack_track_t> out(4);
    sushitrack_object_t d = make_det(10.0f, 20.0f, 0.9f);
    int count = -1;
    EXPECT_EQ(
        sushitrack_update(hdl, &d, 1, out.data(), static_cast<int>(out.size()), 0.04f, &count),
        SUSHITRACK_SUCCESS);
    EXPECT_GE(count, 0);
    sushitrack_destroy(hdl);
}

/**
 * @brief Negative frame_rate is nonsensical and must be rejected.
 */
TEST_F(Small_SushiTrackCParameterRangeTest, NegativeFrameRateIsRejectedOrInert)
{
    params_.frame_rate = -25;
    sushitrack_tracker_t hdl = nullptr;
    sushitrack_status_t s = sushitrack_create_ex(&params_, &hdl);

    if (s != SUSHITRACK_SUCCESS)
    {
        EXPECT_EQ(hdl, nullptr);
        return;
    }

    ASSERT_NE(hdl, nullptr);
    sushitrack_destroy(hdl);
}

/**
 * @brief Negative track_buffer must be rejected or normalised; a negative
 *        lost-track lifetime would otherwise cause underflow on buffer arithmetic.
 */
TEST_F(Small_SushiTrackCParameterRangeTest, NegativeTrackBufferIsRejectedOrNormalised)
{
    params_.track_buffer = -5;
    sushitrack_tracker_t hdl = nullptr;
    sushitrack_status_t s = sushitrack_create_ex(&params_, &hdl);

    if (s != SUSHITRACK_SUCCESS)
    {
        EXPECT_EQ(hdl, nullptr);
        return;
    }

    ASSERT_NE(hdl, nullptr);
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
 * @brief track_thresh greater than high_thresh inverts the score-pool split
 *        rule. The tracker must not crash and should not silently fabricate
 *        a track from a low-score detection.
 */
TEST_F(Small_SushiTrackCParameterRangeTest, TrackThreshGreaterThanHighThreshDoesNotCrash)
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
    EXPECT_GE(count, 0);

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
 * @brief Destroying a null handle must be a safe no-op so consumers can call
 *        Destroy unconditionally from cleanup paths.
 */
TEST(Small_SushiTrackCLifecycleSafetyTest, DestroyNullHandleIsNoOp)
{
    sushitrack_destroy(nullptr);
    SUCCEED();
}

/**
 * @brief Create / Destroy cycles must reclaim resources cleanly; running many
 *        rapid cycles must not leak or fail.
 */
TEST(Small_SushiTrackCLifecycleSafetyTest, RepeatedCreateDestroyCycleSucceeds)
{
    sushitrack_params_t params{};
    sushitrack_get_default_params(&params);

    for (int i = 0; i < 50; ++i)
    {
        sushitrack_tracker_t hdl = sushitrack_create(&params);
        ASSERT_NE(hdl, nullptr);
        sushitrack_destroy(hdl);
    }
}

// Small_SushiTrackCStatusCodeCoverageTest

namespace
{
    /// @brief Kalman type whose creator throws std::runtime_error.
    constexpr int K_THROWING_KALMAN_TYPE = 9101;
    /// @brief Kalman type whose creator throws std::bad_alloc.
    constexpr int K_OUT_OF_MEMORY_KALMAN_TYPE = 9102;
    /// @brief Kalman type whose creator builds type 0 until told to throw.
    constexpr int K_FAILING_LATER_KALMAN_TYPE = 9103;

    /// @brief Whether the creator of K_FAILING_LATER_KALMAN_TYPE throws.
    bool g_fail_later_throws = false;

    /// @brief Returns default parameters naming Kalman type @p type.
    sushitrack_params_t params_with_kalman_type(int type)
    {
        sushitrack_params_t params{};
        sushitrack_get_default_params(&params);
        params.kalman_type = type;
        return params;
    }
} // namespace

/**
 * @brief A component that throws while the tracker is built returns
 *        SUSHITRACK_INITIALIZATION_FAILURE and no handle.
 */
TEST(Small_SushiTrackCStatusCodeCoverageTest, InitFailureReturnsInitializationFailureCode)
{
    SushiTrack::KalmanFilterRegistry::get_instance().register_creator(
        K_THROWING_KALMAN_TYPE,
        [](bool) -> std::unique_ptr<SushiTrack::GatedStatePredictor>
        {
            throw std::runtime_error("injected initialisation failure");
        });
    const sushitrack_params_t params = params_with_kalman_type(K_THROWING_KALMAN_TYPE);
    sushitrack_tracker_t tracker = reinterpret_cast<sushitrack_tracker_t>(0x1);

    EXPECT_EQ(sushitrack_create_ex(&params, &tracker), SUSHITRACK_INITIALIZATION_FAILURE);
    EXPECT_EQ(tracker, nullptr);
}

/**
 * @brief An exception inside sushitrack_update returns SUSHITRACK_RUNTIME_ERROR with a count
 *        of 0.
 */
TEST(Small_SushiTrackCStatusCodeCoverageTest, RuntimeErrorReturnsRuntimeErrorCode)
{
    g_fail_later_throws = false;
    SushiTrack::KalmanFilterRegistry::get_instance().register_creator(
        K_FAILING_LATER_KALMAN_TYPE,
        [](bool enable_xyah) -> std::unique_ptr<SushiTrack::GatedStatePredictor>
        {
            if (g_fail_later_throws)
                throw std::runtime_error("injected update failure");
            return SushiTrack::KalmanFilterRegistry::get_instance().create(0, enable_xyah);
        });
    const sushitrack_params_t params = params_with_kalman_type(K_FAILING_LATER_KALMAN_TYPE);
    sushitrack_tracker_t tracker = nullptr;
    ASSERT_EQ(sushitrack_create_ex(&params, &tracker), SUSHITRACK_SUCCESS);
    g_fail_later_throws = true;
    const sushitrack_object_t object = make_det(10.0f, 20.0f);
    std::vector<sushitrack_track_t> out(4);
    int count = -1;

    EXPECT_EQ(sushitrack_update(tracker, &object, 1, out.data(), 4, 0.04f, &count),
              SUSHITRACK_RUNTIME_ERROR);
    EXPECT_EQ(count, 0);
    g_fail_later_throws = false;
    sushitrack_destroy(tracker);
}

/**
 * @brief An allocation that fails while the tracker is built returns SUSHITRACK_OUT_OF_MEMORY
 *        and no handle.
 */
TEST(Small_SushiTrackCStatusCodeCoverageTest, OutOfMemoryReturnsOutOfMemoryCode)
{
    SushiTrack::KalmanFilterRegistry::get_instance().register_creator(
        K_OUT_OF_MEMORY_KALMAN_TYPE,
        [](bool) -> std::unique_ptr<SushiTrack::GatedStatePredictor>
        {
            throw std::bad_alloc();
        });
    const sushitrack_params_t params = params_with_kalman_type(K_OUT_OF_MEMORY_KALMAN_TYPE);
    sushitrack_tracker_t tracker = reinterpret_cast<sushitrack_tracker_t>(0x1);

    EXPECT_EQ(sushitrack_create_ex(&params, &tracker), SUSHITRACK_OUT_OF_MEMORY);
    EXPECT_EQ(tracker, nullptr);
}

// Small_SushiTrackCBoundaryExceptionTest

/**
 * @brief Fixture that owns one tracker and installs a SushiLog sink that throws on every record.
 *
 * SushiLog contains an exception from its sink, so no C entry point sees one: each test checks
 * that a call keeps its normal result while every record it logs throws in the sink.
 */
class Small_SushiTrackCBoundaryExceptionTest : public ::testing::Test
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
TEST_F(Small_SushiTrackCBoundaryExceptionTest, UpdateSurvivesAThrowingSink)
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
TEST_F(Small_SushiTrackCBoundaryExceptionTest, GetTrackletsSurvivesAThrowingSink)
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
TEST_F(Small_SushiTrackCBoundaryExceptionTest, CreateExSurvivesAThrowingSink)
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
TEST_F(Small_SushiTrackCBoundaryExceptionTest, DestroySurvivesAThrowingSink)
{
    EXPECT_NO_THROW(sushitrack_destroy(hdl_));
    hdl_ = nullptr;
}
