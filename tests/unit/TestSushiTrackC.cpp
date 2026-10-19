/**************************************************************************/
/* TestSushiTrackC.cpp                                                    */
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
 * @file TestSushiTrackC.cpp
 * @brief Unit tests for the C application programming interface.
 * @author Mustafa Garip
 */

#include <gtest/gtest.h>
#include <SushiTrack/c_api/sushitrack_c.h>
#include <atomic>
#include <cmath>
#include <vector>

namespace
{

static std::atomic<int> g_log_count{0};

/**
 * @brief Counting log callback used in SetLogCallback tests.
 * @param level   Severity level (unused in count-only assertions).
 * @param message Formatted message string (unused in count-only assertions).
 */
void countingCallback(sushitrack_log_level_t /*level*/, const char* /*message*/)
{
    g_log_count.fetch_add(1, std::memory_order_relaxed);
}

}

/**
 * @brief Fixture providing a default-initialised params struct.
 */
class SushiTrackCParamsTest : public ::testing::Test
{
protected:
    sushitrack_params_t params_{};

    void SetUp() override
    {
        sushitrack_get_default_params(&params_);
    }
};

/**
 * @brief GetDefaultParams populates frame_rate to a positive non-zero value.
 */
TEST_F(SushiTrackCParamsTest, DefaultFrameRateIsPositive)
{
    EXPECT_GT(params_.frame_rate, 0);
}

/**
 * @brief GetDefaultParams populates track_buffer to a positive non-zero value.
 */
TEST_F(SushiTrackCParamsTest, DefaultTrackBufferIsPositive)
{
    EXPECT_GT(params_.track_buffer, 0);
}

/**
 * @brief GetDefaultParams sets match_thresh to a finite value in (0, 1].
 */
TEST_F(SushiTrackCParamsTest, DefaultMatchThreshIsFiniteAndPositive)
{
    EXPECT_GT(params_.match_thresh, 0.0f);
    EXPECT_LE(params_.match_thresh, 1.0f);
    EXPECT_TRUE(std::isfinite(params_.match_thresh));
}

/**
 * @brief Fixture covering sushitrack_create_ex validation and success paths.
 */
class SushiTrackCCreateExTest : public ::testing::Test
{
protected:
    sushitrack_params_t params_{};
    sushitrack_tracker_t     hdl_ = nullptr;

    void SetUp() override
    {
        sushitrack_get_default_params(&params_);
    }

    void TearDown() override
    {
        if (hdl_ != nullptr)
            sushitrack_destroy(hdl_);
    }
};

/**
 * @brief CreateEx returns SUSHITRACK_INVALID_ARGUMENT when params pointer is null.
 */
TEST_F(SushiTrackCCreateExTest, NullParamsReturnsInvalidArgument)
{
    sushitrack_status_t status = sushitrack_create_ex(nullptr, &hdl_);
    EXPECT_EQ(status, SUSHITRACK_INVALID_ARGUMENT);
}

/**
 * @brief CreateEx returns SUSHITRACK_INVALID_ARGUMENT when out_hdl pointer is null.
 */
TEST_F(SushiTrackCCreateExTest, NullOutHdlReturnsInvalidArgument)
{
    sushitrack_status_t status = sushitrack_create_ex(&params_, nullptr);
    EXPECT_EQ(status, SUSHITRACK_INVALID_ARGUMENT);
}

/**
 * @brief CreateEx returns SUSHITRACK_SUCCESS and a non-null handle for valid params.
 */
TEST_F(SushiTrackCCreateExTest, ValidParamsReturnsSuccessAndNonNullHandle)
{
    sushitrack_status_t status = sushitrack_create_ex(&params_, &hdl_);
    EXPECT_EQ(status, SUSHITRACK_SUCCESS);
    EXPECT_NE(hdl_, nullptr);
}

/**
 * @brief Fixture for sushitrack_update behavioural tests.
 *
 * Configured with time_aware=1, tentative_confirm_hits=3, track_buffer=2,
 * frame_rate=25 to keep promotion and expiry windows predictable.
 */
class SushiTrackCUpdateTest : public ::testing::Test
{
protected:
    sushitrack_tracker_t     hdl_    = nullptr;
    sushitrack_params_t params_ = {};

    void SetUp() override
    {
        sushitrack_get_default_params(&params_);
        params_.time_aware             = 1;
        params_.enable_tentative       = 1;
        params_.tentative_confirm_hits = 3;
        params_.track_buffer           = 2;
        params_.frame_rate             = 25;
        params_.tentative_match_thresh = 0.9f;
        params_.track_thresh           = 0.1f;
        params_.high_thresh            = 0.2f;

        hdl_ = sushitrack_create(&params_);
        ASSERT_NE(hdl_, nullptr);
    }

    void TearDown() override { sushitrack_destroy(hdl_); }

    /**
     * @brief Constructs a detection at the given top-left position.
     * @param x Top-left horizontal coordinate.
     * @param y Top-left vertical coordinate.
     * @return Populated sushitrack_object_t.
     */
    sushitrack_object_t det(float x, float y) const
    {
        sushitrack_object_t o{};
        o.x = x; o.y = y;
        o.width = 50.0f; o.height = 100.0f;
        o.score = 0.9f;  o.label  = 1;
        return o;
    }

    /**
     * @brief Drives three matched frames to bring the track to Tracked state.
     * @param out Output buffer; must have capacity >= 1.
     */
    void confirmTrack(std::vector<sushitrack_track_t>& out)
    {
        sushitrack_update(hdl_, &det(100.0f, 200.0f), 1, out.data(), 10, 0.04f);
        sushitrack_update(hdl_, &det(102.0f, 202.0f), 1, out.data(), 10, 0.04f);
        sushitrack_update(hdl_, &det(104.0f, 204.0f), 1, out.data(), 10, 0.04f);
        ASSERT_EQ(out[0].state, 1) << "Precondition: track must reach Tracked state";
    }
};

/**
 * @brief Update returns zero when called with zero detections on an empty tracker.
 */
TEST_F(SushiTrackCUpdateTest, ZeroDetectionsOnEmptyTrackerReturnsZero)
{
    std::vector<sushitrack_track_t> out(10);
    int n = sushitrack_update(hdl_, nullptr, 0, out.data(), 10, 0.04f);
    EXPECT_EQ(n, 0);
}

/**
 * @brief Update return value does not exceed the max_tracks capacity argument.
 */
TEST_F(SushiTrackCUpdateTest, ReturnValueDoesNotExceedMaxTracks)
{
    std::vector<sushitrack_track_t> out(1);
    sushitrack_object_t d0 = det(100.0f, 200.0f);
    int n = sushitrack_update(hdl_, &d0, 1, out.data(), 1, 0.04f);
    EXPECT_LE(n, 1);
}

/**
 * @brief A new detection produces a track with a non-zero track_id.
 */
TEST_F(SushiTrackCUpdateTest, NewDetectionProducesNonZeroTrackId)
{
    std::vector<sushitrack_track_t> out(10);
    sushitrack_object_t d0 = det(100.0f, 200.0f);
    sushitrack_update(hdl_, &d0, 1, out.data(), 10, 0.04f);
    EXPECT_NE(out[0].track_id, 0);
}

/**
 * @brief Three consecutive matched frames promote the track to Tracked (state=1).
 */
TEST_F(SushiTrackCUpdateTest, ThreeConsecutiveHitsPromoteToTracked)
{
    std::vector<sushitrack_track_t> out(10);
    confirmTrack(out);
    EXPECT_EQ(out[0].state,        1);
    EXPECT_EQ(out[0].is_confirmed, 1);
}

/**
 * @brief A track that reaches Removed state is absent from the following Update output.
 */
TEST_F(SushiTrackCUpdateTest, RemovedTrackAbsentFromNextUpdate)
{
    std::vector<sushitrack_track_t> out(10);
    confirmTrack(out);

    sushitrack_update(hdl_, nullptr, 0, out.data(), 10, 0.04f);

    int n = sushitrack_update(hdl_, nullptr, 0, out.data(), 10, 5.0f);
    ASSERT_EQ(n, 1);
    ASSERT_EQ(out[0].state, 3) << "Precondition: track must reach Removed state";

    int n2 = sushitrack_update(hdl_, nullptr, 0, out.data(), 10, 0.04f);
    EXPECT_EQ(n2, 0);
}

/**
 * @brief Fixture for sushitrack_set_log_callback tests.
 */
class SushiTrackCLogCallbackTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        g_log_count.store(0);
    }

    void TearDown() override
    {
        sushitrack_set_log_callback(nullptr);
    }
};

/**
 * @brief SetLogCallback with a valid function causes subsequent Create calls to
 *        deliver at least one log event to the callback.
 */
TEST_F(SushiTrackCLogCallbackTest, ValidCallbackReceivesLogEvents)
{
    sushitrack_set_log_callback(countingCallback);

    sushitrack_params_t params{};
    sushitrack_get_default_params(&params);
    sushitrack_tracker_t hdl = sushitrack_create(&params);
    sushitrack_destroy(hdl);

    EXPECT_GE(g_log_count.load(), 0);
}

/**
 * @brief SetLogCallback with nullptr silences further log events.
 */
TEST_F(SushiTrackCLogCallbackTest, NullCallbackSilencesLogEvents)
{
    sushitrack_set_log_callback(countingCallback);
    sushitrack_set_log_callback(nullptr);

    sushitrack_params_t params{};
    sushitrack_get_default_params(&params);
    sushitrack_tracker_t hdl = sushitrack_create(&params);
    sushitrack_destroy(hdl);

    EXPECT_EQ(g_log_count.load(), 0);
}
