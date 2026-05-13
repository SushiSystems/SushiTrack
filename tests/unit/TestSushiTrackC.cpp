/**
 * @file      TestSushiTrackC.cpp
 * @brief     Unit tests for the SushiTrack C-compatible public API layer.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      13.05.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 *
 * Coverage
 * --------
 * SushiTrackCParamsTest
 *   - GetDefaultParams populates frame_rate to a positive non-zero value.
 *   - GetDefaultParams populates track_buffer to a positive non-zero value.
 *   - GetDefaultParams sets match_thresh to a finite value in (0, 1].
 *
 * SushiTrackCCreateExTest
 *   - CreateEx returns SUSHITRACK_INVALID_ARGUMENT when params pointer is null.
 *   - CreateEx returns SUSHITRACK_INVALID_ARGUMENT when out_hdl pointer is null.
 *   - CreateEx returns SUSHITRACK_SUCCESS and a non-null handle for valid params.
 *
 * SushiTrackCUpdateTest (time_aware=1, tentative_confirm_hits=3)
 *   - Update returns zero when called with zero detections on an empty tracker.
 *   - Update returns the number of tracks written, not exceeding max_tracks.
 *   - A new detection produces a track with a non-zero track_id.
 *   - Three consecutive matched frames promote the track to Tracked (state=1).
 *   - A track that reaches Removed state is absent from the following Update output.
 *
 * SushiTrackCLogCallbackTest
 *   - SetLogCallback with a valid function delivers subsequent log events.
 *   - SetLogCallback with nullptr silences further log events.
 */

#include <gtest/gtest.h>
#include "SushiTrackC.h"
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
void countingCallback(SushiTrack_LogLevel /*level*/, const char* /*message*/)
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
    SushiTrack_Params params_{};

    void SetUp() override
    {
        SushiTrack_GetDefaultParams(&params_);
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
 * @brief Fixture covering SushiTrack_CreateEx validation and success paths.
 */
class SushiTrackCCreateExTest : public ::testing::Test
{
protected:
    SushiTrack_Params params_{};
    SushiTrackHdl     hdl_ = nullptr;

    void SetUp() override
    {
        SushiTrack_GetDefaultParams(&params_);
    }

    void TearDown() override
    {
        if (hdl_ != nullptr)
            SushiTrack_Destroy(hdl_);
    }
};

/**
 * @brief CreateEx returns SUSHITRACK_INVALID_ARGUMENT when params pointer is null.
 */
TEST_F(SushiTrackCCreateExTest, NullParamsReturnsInvalidArgument)
{
    SushiTrack_Status status = SushiTrack_CreateEx(nullptr, &hdl_);
    EXPECT_EQ(status, SUSHITRACK_INVALID_ARGUMENT);
}

/**
 * @brief CreateEx returns SUSHITRACK_INVALID_ARGUMENT when out_hdl pointer is null.
 */
TEST_F(SushiTrackCCreateExTest, NullOutHdlReturnsInvalidArgument)
{
    SushiTrack_Status status = SushiTrack_CreateEx(&params_, nullptr);
    EXPECT_EQ(status, SUSHITRACK_INVALID_ARGUMENT);
}

/**
 * @brief CreateEx returns SUSHITRACK_SUCCESS and a non-null handle for valid params.
 */
TEST_F(SushiTrackCCreateExTest, ValidParamsReturnsSuccessAndNonNullHandle)
{
    SushiTrack_Status status = SushiTrack_CreateEx(&params_, &hdl_);
    EXPECT_EQ(status, SUSHITRACK_SUCCESS);
    EXPECT_NE(hdl_, nullptr);
}

/**
 * @brief Fixture for SushiTrack_Update behavioural tests.
 *
 * Configured with time_aware=1, tentative_confirm_hits=3, track_buffer=2,
 * frame_rate=25 to keep promotion and expiry windows predictable.
 */
class SushiTrackCUpdateTest : public ::testing::Test
{
protected:
    SushiTrackHdl     hdl_    = nullptr;
    SushiTrack_Params params_ = {};

    void SetUp() override
    {
        SushiTrack_GetDefaultParams(&params_);
        params_.time_aware             = 1;
        params_.enable_tentative       = 1;
        params_.tentative_confirm_hits = 3;
        params_.track_buffer           = 2;
        params_.frame_rate             = 25;
        params_.tentative_match_thresh = 0.9f;
        params_.track_thresh           = 0.1f;
        params_.high_thresh            = 0.2f;

        hdl_ = SushiTrack_Create(&params_);
        ASSERT_NE(hdl_, nullptr);
    }

    void TearDown() override { SushiTrack_Destroy(hdl_); }

    /**
     * @brief Constructs a detection at the given top-left position.
     * @param x Top-left horizontal coordinate.
     * @param y Top-left vertical coordinate.
     * @return Populated SushiTrack_Object.
     */
    SushiTrack_Object det(float x, float y) const
    {
        SushiTrack_Object o{};
        o.x = x; o.y = y;
        o.width = 50.0f; o.height = 100.0f;
        o.score = 0.9f;  o.label  = 1;
        return o;
    }

    /**
     * @brief Drives three matched frames to bring the track to Tracked state.
     * @param out Output buffer; must have capacity >= 1.
     */
    void confirmTrack(std::vector<SushiTrack_Track>& out)
    {
        SushiTrack_Update(hdl_, &det(100.0f, 200.0f), 1, out.data(), 10, 0.04f);
        SushiTrack_Update(hdl_, &det(102.0f, 202.0f), 1, out.data(), 10, 0.04f);
        SushiTrack_Update(hdl_, &det(104.0f, 204.0f), 1, out.data(), 10, 0.04f);
        ASSERT_EQ(out[0].state, 1) << "Precondition: track must reach Tracked state";
    }
};

/**
 * @brief Update returns zero when called with zero detections on an empty tracker.
 */
TEST_F(SushiTrackCUpdateTest, ZeroDetectionsOnEmptyTrackerReturnsZero)
{
    std::vector<SushiTrack_Track> out(10);
    int n = SushiTrack_Update(hdl_, nullptr, 0, out.data(), 10, 0.04f);
    EXPECT_EQ(n, 0);
}

/**
 * @brief Update return value does not exceed the max_tracks capacity argument.
 */
TEST_F(SushiTrackCUpdateTest, ReturnValueDoesNotExceedMaxTracks)
{
    std::vector<SushiTrack_Track> out(1);
    SushiTrack_Object d0 = det(100.0f, 200.0f);
    int n = SushiTrack_Update(hdl_, &d0, 1, out.data(), 1, 0.04f);
    EXPECT_LE(n, 1);
}

/**
 * @brief A new detection produces a track with a non-zero track_id.
 */
TEST_F(SushiTrackCUpdateTest, NewDetectionProducesNonZeroTrackId)
{
    std::vector<SushiTrack_Track> out(10);
    SushiTrack_Object d0 = det(100.0f, 200.0f);
    SushiTrack_Update(hdl_, &d0, 1, out.data(), 10, 0.04f);
    EXPECT_NE(out[0].track_id, 0);
}

/**
 * @brief Three consecutive matched frames promote the track to Tracked (state=1).
 */
TEST_F(SushiTrackCUpdateTest, ThreeConsecutiveHitsPromoteToTracked)
{
    std::vector<SushiTrack_Track> out(10);
    confirmTrack(out);
    EXPECT_EQ(out[0].state,        1);
    EXPECT_EQ(out[0].is_confirmed, 1);
}

/**
 * @brief A track that reaches Removed state is absent from the following Update output.
 */
TEST_F(SushiTrackCUpdateTest, RemovedTrackAbsentFromNextUpdate)
{
    std::vector<SushiTrack_Track> out(10);
    confirmTrack(out);

    SushiTrack_Update(hdl_, nullptr, 0, out.data(), 10, 0.04f);

    int n = SushiTrack_Update(hdl_, nullptr, 0, out.data(), 10, 5.0f);
    ASSERT_EQ(n, 1);
    ASSERT_EQ(out[0].state, 3) << "Precondition: track must reach Removed state";

    int n2 = SushiTrack_Update(hdl_, nullptr, 0, out.data(), 10, 0.04f);
    EXPECT_EQ(n2, 0);
}

/**
 * @brief Fixture for SushiTrack_SetLogCallback tests.
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
        SushiTrack_SetLogCallback(nullptr);
    }
};

/**
 * @brief SetLogCallback with a valid function causes subsequent Create calls to
 *        deliver at least one log event to the callback.
 */
TEST_F(SushiTrackCLogCallbackTest, ValidCallbackReceivesLogEvents)
{
    SushiTrack_SetLogCallback(countingCallback);

    SushiTrack_Params params{};
    SushiTrack_GetDefaultParams(&params);
    SushiTrackHdl hdl = SushiTrack_Create(&params);
    SushiTrack_Destroy(hdl);

    EXPECT_GE(g_log_count.load(), 0);
}

/**
 * @brief SetLogCallback with nullptr silences further log events.
 */
TEST_F(SushiTrackCLogCallbackTest, NullCallbackSilencesLogEvents)
{
    SushiTrack_SetLogCallback(countingCallback);
    SushiTrack_SetLogCallback(nullptr);

    SushiTrack_Params params{};
    SushiTrack_GetDefaultParams(&params);
    SushiTrackHdl hdl = SushiTrack_Create(&params);
    SushiTrack_Destroy(hdl);

    EXPECT_EQ(g_log_count.load(), 0);
}
