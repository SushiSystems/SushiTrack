/****************************************************************/
/* TestCAPIErrors.cpp                                           */
/* SushiTrack - https://github.com/SushiSystems/SushiTrack      */
/* Copyright (c) 2026 Sushi Systems                             */
/* Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.    */
/* Commercial use requires a licence from Sushi Systems.        */
/****************************************************************/

/**
 * @file TestCAPIErrors.cpp
 * @brief Unit tests for C API error handling and status codes.
 * @author Mustafa Garip
 */

#include <gtest/gtest.h>
#include <SushiTrack/sushitrack_c.h>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

namespace
{

/**
 * @brief Constructs a benign detection with default geometry and the given score.
 * @param x     Top-left horizontal coordinate.
 * @param y     Top-left vertical coordinate.
 * @param score Detection confidence score in [0, 1].
 * @return Populated sushitrack_object_t.
 */
sushitrack_object_t makeDet(float x, float y, float score = 0.9f)
{
    sushitrack_object_t o{};
    o.x      = x;
    o.y      = y;
    o.width  = 50.0f;
    o.height = 100.0f;
    o.score  = score;
    o.label  = 1;
    return o;
}

/**
 * @brief Drives N consecutive frames of a single moving detection through the
 *        tracker; convenience helper for tests that need an established track.
 * @param hdl        Tracker handle.
 * @param n_frames   Number of update cycles to execute.
 * @param out        Output buffer; must have capacity >= 1.
 * @param dt         Time delta passed to each Update call.
 */
void driveFrames(sushitrack_tracker_t                   hdl,
                 int                            n_frames,
                 std::vector<sushitrack_track_t>&  out,
                 float                          dt = 0.04f)
{
    for (int i = 0; i < n_frames; ++i)
    {
        sushitrack_object_t d = makeDet(100.0f + 2.0f * i, 200.0f + 2.0f * i);
        sushitrack_update(hdl, &d, 1, out.data(), static_cast<int>(out.size()), dt);
    }
}

}

// CAPIInvalidArgumentTest

/**
 * @brief Fixture providing default params for invalid-argument tests.
 */
class CAPIInvalidArgumentTest : public ::testing::Test
{
protected:
    sushitrack_params_t params_{};
    sushitrack_tracker_t     hdl_ = nullptr;

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
TEST(CAPIInvalidArgumentStandaloneTest, GetDefaultParamsNullPointerDoesNotCrash)
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
TEST(CAPIInvalidArgumentStandaloneTest, CreateWithNullParamsAutoLoadsConfig)
{
    sushitrack_tracker_t hdl = sushitrack_create(nullptr);
    EXPECT_NE(hdl, nullptr);
    if (hdl != nullptr)
        sushitrack_destroy(hdl);
}

/**
 * @brief CreateEx with a null params pointer must report INVALID_ARGUMENT and
 *        leave the out handle untouched (or null).
 */
TEST(CAPIInvalidArgumentStandaloneTest, CreateExNullParamsPropagatesStatusViaOutHandle)
{
    sushitrack_tracker_t     hdl    = reinterpret_cast<sushitrack_tracker_t>(static_cast<uintptr_t>(0xDEADBEEFul));
    sushitrack_status_t status = sushitrack_create_ex(nullptr, &hdl);
    EXPECT_EQ(status, SUSHITRACK_INVALID_ARGUMENT);
}

/**
 * @brief Update with a null tracker handle must not crash; the returned count
 *        must be non-positive (zero or a negative status indicator).
 */
TEST_F(CAPIInvalidArgumentTest, UpdateWithNullHandleReturnsNonPositive)
{
    std::vector<sushitrack_track_t> out(4);
    sushitrack_object_t d = makeDet(10.0f, 20.0f);
    int n = sushitrack_update(nullptr, &d, 1, out.data(), static_cast<int>(out.size()), 0.04f);
    EXPECT_LE(n, 0);
}

/**
 * @brief Update with a null detection array and positive n_objects must be
 *        treated as misuse; the returned count must be non-positive.
 */
TEST_F(CAPIInvalidArgumentTest, UpdateWithNullDetectionsAndPositiveCountReturnsNonPositive)
{
    std::vector<sushitrack_track_t> out(4);
    int n = sushitrack_update(hdl_, nullptr, 1, out.data(), static_cast<int>(out.size()), 0.04f);
    EXPECT_LE(n, 0);
}

/**
 * @brief Update with a null output buffer and positive max_tracks must be
 *        treated as misuse; the returned count must be non-positive.
 */
TEST_F(CAPIInvalidArgumentTest, UpdateWithNullOutputBufferAndPositiveMaxReturnsNonPositive)
{
    sushitrack_object_t d = makeDet(10.0f, 20.0f);
    int n = sushitrack_update(hdl_, &d, 1, nullptr, 4, 0.04f);
    EXPECT_LE(n, 0);
}

/**
 * @brief Update with a negative n_objects must not crash; the returned count
 *        must be non-positive.
 */
TEST_F(CAPIInvalidArgumentTest, UpdateWithNegativeObjectCountReturnsNonPositive)
{
    std::vector<sushitrack_track_t> out(4);
    sushitrack_object_t d = makeDet(10.0f, 20.0f);
    int n = sushitrack_update(hdl_, &d, -1, out.data(), static_cast<int>(out.size()), 0.04f);
    EXPECT_LE(n, 0);
}

/**
 * @brief Update with a negative max_tracks must not crash; the returned count
 *        must be non-positive.
 */
TEST_F(CAPIInvalidArgumentTest, UpdateWithNegativeMaxTracksReturnsNonPositive)
{
    std::vector<sushitrack_track_t> out(4);
    sushitrack_object_t d = makeDet(10.0f, 20.0f);
    int n = sushitrack_update(hdl_, &d, 1, out.data(), -1, 0.04f);
    EXPECT_LE(n, 0);
}

// CAPIParameterRangeTest

/**
 * @brief Fixture for parameter-range validation tests.
 *
 * Each test mutates a single field of an otherwise default params struct and
 * checks that the tracker either refuses creation or behaves defensively (no
 * crash, no spurious tracks).
 */
class CAPIParameterRangeTest : public ::testing::Test
{
protected:
    sushitrack_params_t params_{};

    void SetUp() override
    {
        sushitrack_get_default_params(&params_);
    }
};

/**
 * @brief Zero frame_rate must either be rejected at construction or, if the
 *        tracker is created, must not produce promoted tracks (the timing
 *        denominator is undefined).
 */
TEST_F(CAPIParameterRangeTest, ZeroFrameRateIsRejectedOrInert)
{
    params_.frame_rate = 0;
    sushitrack_tracker_t hdl   = nullptr;
    sushitrack_status_t s = sushitrack_create_ex(&params_, &hdl);

    if (s == SUSHITRACK_INVALID_ARGUMENT)
    {
        EXPECT_EQ(hdl, nullptr);
        return;
    }

    ASSERT_NE(hdl, nullptr);
    std::vector<sushitrack_track_t> out(4);
    sushitrack_object_t d = makeDet(10.0f, 20.0f, 0.9f);
    int n = sushitrack_update(hdl, &d, 1, out.data(), static_cast<int>(out.size()), 0.04f);
    EXPECT_GE(n, 0);
    sushitrack_destroy(hdl);
}

/**
 * @brief Negative frame_rate is nonsensical and must be rejected.
 */
TEST_F(CAPIParameterRangeTest, NegativeFrameRateIsRejectedOrInert)
{
    params_.frame_rate = -25;
    sushitrack_tracker_t hdl   = nullptr;
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
TEST_F(CAPIParameterRangeTest, NegativeTrackBufferIsRejectedOrNormalised)
{
    params_.track_buffer = -5;
    sushitrack_tracker_t hdl     = nullptr;
    sushitrack_status_t s   = sushitrack_create_ex(&params_, &hdl);

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
TEST_F(CAPIParameterRangeTest, MatchThreshGreaterThanOneIsTolerated)
{
    params_.match_thresh = 5.0f;
    sushitrack_tracker_t hdl     = sushitrack_create(&params_);
    ASSERT_NE(hdl, nullptr);
    sushitrack_destroy(hdl);
}

/**
 * @brief track_thresh greater than high_thresh inverts the score-pool split
 *        rule. The tracker must not crash and should not silently fabricate
 *        a track from a low-score detection.
 */
TEST_F(CAPIParameterRangeTest, TrackThreshGreaterThanHighThreshDoesNotCrash)
{
    params_.track_thresh = 0.9f;
    params_.high_thresh  = 0.1f;
    sushitrack_tracker_t hdl     = sushitrack_create(&params_);
    ASSERT_NE(hdl, nullptr);

    std::vector<sushitrack_track_t> out(4);
    sushitrack_object_t d = makeDet(10.0f, 20.0f, 0.5f);
    int n = sushitrack_update(hdl, &d, 1, out.data(), static_cast<int>(out.size()), 0.04f);
    EXPECT_GE(n, 0);

    sushitrack_destroy(hdl);
}

// CAPIBufferCapacityTest

/**
 * @brief Fixture for output-buffer capacity behaviour.
 */
class CAPIBufferCapacityTest : public ::testing::Test
{
protected:
    sushitrack_tracker_t     hdl_    = nullptr;
    sushitrack_params_t params_ = {};

    void SetUp() override
    {
        sushitrack_get_default_params(&params_);
        params_.enable_tentative = 0;
        params_.time_aware       = 1;
        params_.track_thresh     = 0.1f;
        params_.high_thresh      = 0.2f;
        params_.match_thresh     = 0.9f;
        hdl_ = sushitrack_create(&params_);
        ASSERT_NE(hdl_, nullptr);
    }

    void TearDown() override { sushitrack_destroy(hdl_); }
};

/**
 * @brief max_tracks=0 must produce a zero result regardless of detection count;
 *        the tracker cannot legally write to a zero-capacity buffer.
 */
TEST_F(CAPIBufferCapacityTest, ZeroMaxTracksProducesZeroReturn)
{
    sushitrack_object_t d = makeDet(10.0f, 20.0f);
    sushitrack_track_t  dummy{};
    int n = sushitrack_update(hdl_, &d, 1, &dummy, 0, 0.04f);
    EXPECT_LE(n, 0);
}

/**
 * @brief The returned count must never exceed the capacity passed in
 *        max_tracks, even when many detections promote simultaneously.
 */
TEST_F(CAPIBufferCapacityTest, ReturnedCountIsBoundedByCapacity)
{
    constexpr int kDetections = 12;
    constexpr int kCapacity   = 3;

    std::vector<sushitrack_object_t> dets;
    dets.reserve(kDetections);
    for (int i = 0; i < kDetections; ++i)
        dets.push_back(makeDet(50.0f + 200.0f * i, 50.0f + 200.0f * i));

    std::vector<sushitrack_track_t> out(kCapacity);
    int n = sushitrack_update(hdl_,
                             dets.data(),
                             kDetections,
                             out.data(),
                             kCapacity,
                             0.04f);

    EXPECT_LE(n, kCapacity);
    EXPECT_GE(n, 0);
}

/**
 * @brief DISABLED. Documents the desired BUFFER_OVERFLOW semantics for future
 *        work: when the internal active-track count exceeds max_tracks, an
 *        accompanying status channel should signal SUSHITRACK_BUFFER_OVERFLOW
 *        rather than silently truncating.
 *
 */
TEST_F(CAPIBufferCapacityTest, DISABLED_BufferOverflowReturnsErrorCode)
{
    // Note: Wire sushitrack_update_ex with status pointer; assert *status == SUSHITRACK_BUFFER_OVERFLOW when truncation occurs.
    FAIL() << "Pending sushitrack_update_ex with status channel.";
}

// CAPILifecycleSafetyTest

/**
 * @brief Destroying a null handle must be a safe no-op so consumers can call
 *        Destroy unconditionally from cleanup paths.
 */
TEST(CAPILifecycleSafetyTest, DestroyNullHandleIsNoOp)
{
    sushitrack_destroy(nullptr);
    SUCCEED();
}

/**
 * @brief Repeated SetLogCallback calls must remain safe even when the
 *        argument toggles between a real callback and nullptr.
 */
TEST(CAPILifecycleSafetyTest, RepeatedSetLogCallbackIsSafe)
{
    auto cb = [](sushitrack_log_level_t, const char*) {};
    sushitrack_set_log_callback(cb);
    sushitrack_set_log_callback(nullptr);
    sushitrack_set_log_callback(cb);
    sushitrack_set_log_callback(nullptr);
    SUCCEED();
}

/**
 * @brief Create / Destroy cycles must reclaim resources cleanly; running many
 *        rapid cycles must not leak or fail.
 */
TEST(CAPILifecycleSafetyTest, RepeatedCreateDestroyCycleSucceeds)
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

// CAPIStatusCodeCoverageTest — Disabled placeholders for future status codes

/**
 * @brief DISABLED. Documents the desired propagation path for
 *        SUSHITRACK_INITIALIZATION_FAILURE. Enable when CreateEx can surface
 *        sub-component construction failures (e.g. assignment solver init).
 */
TEST(CAPIStatusCodeCoverageTest, DISABLED_InitFailureReturnsInitializationFailureCode)
{
    // Note: Inject a controlled init failure (e.g. a registered factory that throws) and assert sushitrack_create_ex returns SUSHITRACK_INITIALIZATION_FAILURE.
    FAIL() << "Pending fault-injection hook for sub-component initialisation.";
}

/**
 * @brief DISABLED. Documents the desired propagation path for
 *        SUSHITRACK_RUNTIME_ERROR via a future UpdateEx variant.
 */
TEST(CAPIStatusCodeCoverageTest, DISABLED_RuntimeErrorReturnsRuntimeErrorCode)
{
    // Note: When sushitrack_update_ex is added, assert that an exception caught inside the C bridge surfaces as SUSHITRACK_RUNTIME_ERROR.
    FAIL() << "Pending sushitrack_update_ex with status channel.";
}

/**
 * @brief DISABLED. Documents the desired propagation path for
 *        SUSHITRACK_OUT_OF_MEMORY. Enable when an allocator hook or stub
 *        allocator is in place to force bad_alloc inside the tracker core.
 */
TEST(CAPIStatusCodeCoverageTest, DISABLED_OutOfMemoryReturnsOutOfMemoryCode)
{
    // Note: Inject a controlled bad_alloc and assert sushitrack_create_ex returns SUSHITRACK_OUT_OF_MEMORY.
    FAIL() << "Pending allocator fault-injection hook.";
}
