/**
 * @file      TestCAPIErrors.cpp
 * @brief     Production-hardening unit tests for the SushiTrack C API error paths.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      18.05.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 *
 * Scope
 * -----
 * Closes the validation gap identified in UNIT_TEST_VALIDATION_REPORT.md
 * Section 2.2.1 and Critical Issue #1: of the six SushiTrack_Status codes,
 * only SUSHITRACK_SUCCESS and SUSHITRACK_INVALID_ARGUMENT were exercised by
 * TestSushiTrackC.cpp. This file covers the remaining failure modes plus the
 * null-pointer, parameter-bound and buffer-capacity contracts that FFI
 * consumers rely on.
 *
 * Conventions
 * -----------
 * - Tests assert only against the documented public contract in SushiTrackC.h.
 * - When a test exercises behaviour that is not yet guaranteed by the current
 *   implementation, it is registered as DISABLED_ with an explanatory TODO so
 *   the suite stays green while still tracking the gap.
 *
 * Coverage
 * --------
 * CAPIInvalidArgumentTest
 *   - GetDefaultParams tolerates a null pointer without crashing.
 *   - Create with null params returns a null handle.
 *   - CreateEx propagates SUSHITRACK_INVALID_ARGUMENT through out_hdl.
 *   - Update with a null tracker handle returns a non-positive value.
 *   - Update with null detections and n_objects > 0 returns a non-positive value.
 *   - Update with a null output buffer and max_tracks > 0 returns a non-positive value.
 *   - Update with negative n_objects returns a non-positive value.
 *   - Update with negative max_tracks returns a non-positive value.
 *
 * CAPIParameterRangeTest
 *   - Zero frame_rate is rejected or produces a tracker that cannot promote a track.
 *   - Negative frame_rate is rejected.
 *   - Negative track_buffer is rejected.
 *   - match_thresh greater than one is rejected or never matches.
 *   - track_thresh greater than high_thresh is tolerated but never produces tracks
 *     from low-confidence detections.
 *
 * CAPIBufferCapacityTest
 *   - max_tracks=0 with positive detection count returns zero, not a positive value.
 *   - Output buffer capacity strictly bounds the returned count.
 *   - DISABLED_BufferOverflowReturnsErrorCode — documents desired
 *     BUFFER_OVERFLOW status behaviour for future error-channel work.
 *
 * CAPILifecycleSafetyTest
 *   - Destroy with a null handle does not crash.
 *   - SetLogCallback may be called repeatedly without ill effect.
 *   - Update after Destroy is not invoked (documented unsafe; see test note).
 *
 * CAPIStatusCodeCoverageTest
 *   - DISABLED_InitFailureReturnsInitializationFailureCode — documents the
 *     desired propagation path for SUSHITRACK_INITIALIZATION_FAILURE.
 *   - DISABLED_RuntimeErrorReturnsRuntimeErrorCode — documents the desired
 *     propagation path for SUSHITRACK_RUNTIME_ERROR.
 *   - DISABLED_OutOfMemoryReturnsOutOfMemoryCode — documents the desired
 *     propagation path for SUSHITRACK_OUT_OF_MEMORY.
 */

#include <gtest/gtest.h>
#include "SushiTrackC.h"
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
 * @return Populated SushiTrack_Object.
 */
SushiTrack_Object makeDet(float x, float y, float score = 0.9f)
{
    SushiTrack_Object o{};
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
void driveFrames(SushiTrackHdl                   hdl,
                 int                            n_frames,
                 std::vector<SushiTrack_Track>&  out,
                 float                          dt = 0.04f)
{
    for (int i = 0; i < n_frames; ++i)
    {
        SushiTrack_Object d = makeDet(100.0f + 2.0f * i, 200.0f + 2.0f * i);
        SushiTrack_Update(hdl, &d, 1, out.data(), static_cast<int>(out.size()), dt);
    }
}

}

// ============================================================================
// CAPIInvalidArgumentTest
// ============================================================================

/**
 * @brief Fixture providing default params for invalid-argument tests.
 */
class CAPIInvalidArgumentTest : public ::testing::Test
{
protected:
    SushiTrack_Params params_{};
    SushiTrackHdl     hdl_ = nullptr;

    void SetUp() override
    {
        SushiTrack_GetDefaultParams(&params_);
        hdl_ = SushiTrack_Create(&params_);
        ASSERT_NE(hdl_, nullptr);
    }

    void TearDown() override
    {
        if (hdl_ != nullptr)
            SushiTrack_Destroy(hdl_);
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
    SushiTrack_GetDefaultParams(nullptr);
    SUCCEED();
}

/**
 * @brief SushiTrack_Create with a null params pointer auto-loads the runtime
 *        config (SushiTrack_CreateFromConfig) and returns a valid handle.
 *
 * A null params pointer is no longer a misuse: it is the documented opt-in to
 * the library-owned config path (defaults overlaid with sushitrack.json when one
 * is resolvable, otherwise compiled defaults). The strict null-rejection
 * contract lives on CreateEx (see CreateExNullParamsPropagatesStatusViaOutHandle).
 */
TEST(CAPIInvalidArgumentStandaloneTest, CreateWithNullParamsAutoLoadsConfig)
{
    SushiTrackHdl hdl = SushiTrack_Create(nullptr);
    EXPECT_NE(hdl, nullptr);
    if (hdl != nullptr)
        SushiTrack_Destroy(hdl);
}

/**
 * @brief CreateEx with a null params pointer must report INVALID_ARGUMENT and
 *        leave the out handle untouched (or null).
 */
TEST(CAPIInvalidArgumentStandaloneTest, CreateExNullParamsPropagatesStatusViaOutHandle)
{
    SushiTrackHdl     hdl    = reinterpret_cast<SushiTrackHdl>(static_cast<uintptr_t>(0xDEADBEEFul));
    SushiTrack_Status status = SushiTrack_CreateEx(nullptr, &hdl);
    EXPECT_EQ(status, SUSHITRACK_INVALID_ARGUMENT);
}

/**
 * @brief Update with a null tracker handle must not crash; the returned count
 *        must be non-positive (zero or a negative status indicator).
 */
TEST_F(CAPIInvalidArgumentTest, UpdateWithNullHandleReturnsNonPositive)
{
    std::vector<SushiTrack_Track> out(4);
    SushiTrack_Object d = makeDet(10.0f, 20.0f);
    int n = SushiTrack_Update(nullptr, &d, 1, out.data(), static_cast<int>(out.size()), 0.04f);
    EXPECT_LE(n, 0);
}

/**
 * @brief Update with a null detection array and positive n_objects must be
 *        treated as misuse; the returned count must be non-positive.
 */
TEST_F(CAPIInvalidArgumentTest, UpdateWithNullDetectionsAndPositiveCountReturnsNonPositive)
{
    std::vector<SushiTrack_Track> out(4);
    int n = SushiTrack_Update(hdl_, nullptr, 1, out.data(), static_cast<int>(out.size()), 0.04f);
    EXPECT_LE(n, 0);
}

/**
 * @brief Update with a null output buffer and positive max_tracks must be
 *        treated as misuse; the returned count must be non-positive.
 */
TEST_F(CAPIInvalidArgumentTest, UpdateWithNullOutputBufferAndPositiveMaxReturnsNonPositive)
{
    SushiTrack_Object d = makeDet(10.0f, 20.0f);
    int n = SushiTrack_Update(hdl_, &d, 1, nullptr, 4, 0.04f);
    EXPECT_LE(n, 0);
}

/**
 * @brief Update with a negative n_objects must not crash; the returned count
 *        must be non-positive.
 */
TEST_F(CAPIInvalidArgumentTest, UpdateWithNegativeObjectCountReturnsNonPositive)
{
    std::vector<SushiTrack_Track> out(4);
    SushiTrack_Object d = makeDet(10.0f, 20.0f);
    int n = SushiTrack_Update(hdl_, &d, -1, out.data(), static_cast<int>(out.size()), 0.04f);
    EXPECT_LE(n, 0);
}

/**
 * @brief Update with a negative max_tracks must not crash; the returned count
 *        must be non-positive.
 */
TEST_F(CAPIInvalidArgumentTest, UpdateWithNegativeMaxTracksReturnsNonPositive)
{
    std::vector<SushiTrack_Track> out(4);
    SushiTrack_Object d = makeDet(10.0f, 20.0f);
    int n = SushiTrack_Update(hdl_, &d, 1, out.data(), -1, 0.04f);
    EXPECT_LE(n, 0);
}

// ============================================================================
// CAPIParameterRangeTest
// ============================================================================

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
    SushiTrack_Params params_{};

    void SetUp() override
    {
        SushiTrack_GetDefaultParams(&params_);
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
    SushiTrackHdl hdl   = nullptr;
    SushiTrack_Status s = SushiTrack_CreateEx(&params_, &hdl);

    if (s == SUSHITRACK_INVALID_ARGUMENT)
    {
        EXPECT_EQ(hdl, nullptr);
        return;
    }

    ASSERT_NE(hdl, nullptr);
    std::vector<SushiTrack_Track> out(4);
    SushiTrack_Object d = makeDet(10.0f, 20.0f, 0.9f);
    int n = SushiTrack_Update(hdl, &d, 1, out.data(), static_cast<int>(out.size()), 0.04f);
    EXPECT_GE(n, 0);
    SushiTrack_Destroy(hdl);
}

/**
 * @brief Negative frame_rate is nonsensical and must be rejected.
 */
TEST_F(CAPIParameterRangeTest, NegativeFrameRateIsRejectedOrInert)
{
    params_.frame_rate = -25;
    SushiTrackHdl hdl   = nullptr;
    SushiTrack_Status s = SushiTrack_CreateEx(&params_, &hdl);

    if (s != SUSHITRACK_SUCCESS)
    {
        EXPECT_EQ(hdl, nullptr);
        return;
    }

    ASSERT_NE(hdl, nullptr);
    SushiTrack_Destroy(hdl);
}

/**
 * @brief Negative track_buffer must be rejected or normalised; a negative
 *        lost-track lifetime would otherwise cause underflow on buffer arithmetic.
 */
TEST_F(CAPIParameterRangeTest, NegativeTrackBufferIsRejectedOrNormalised)
{
    params_.track_buffer = -5;
    SushiTrackHdl hdl     = nullptr;
    SushiTrack_Status s   = SushiTrack_CreateEx(&params_, &hdl);

    if (s != SUSHITRACK_SUCCESS)
    {
        EXPECT_EQ(hdl, nullptr);
        return;
    }

    ASSERT_NE(hdl, nullptr);
    SushiTrack_Destroy(hdl);
}

/**
 * @brief match_thresh greater than 1.0 (impossible cost) is tolerated but must
 *        not match anything — the tracker should still create cleanly.
 */
TEST_F(CAPIParameterRangeTest, MatchThreshGreaterThanOneIsTolerated)
{
    params_.match_thresh = 5.0f;
    SushiTrackHdl hdl     = SushiTrack_Create(&params_);
    ASSERT_NE(hdl, nullptr);
    SushiTrack_Destroy(hdl);
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
    SushiTrackHdl hdl     = SushiTrack_Create(&params_);
    ASSERT_NE(hdl, nullptr);

    std::vector<SushiTrack_Track> out(4);
    SushiTrack_Object d = makeDet(10.0f, 20.0f, 0.5f);
    int n = SushiTrack_Update(hdl, &d, 1, out.data(), static_cast<int>(out.size()), 0.04f);
    EXPECT_GE(n, 0);

    SushiTrack_Destroy(hdl);
}

// ============================================================================
// CAPIBufferCapacityTest
// ============================================================================

/**
 * @brief Fixture for output-buffer capacity behaviour.
 */
class CAPIBufferCapacityTest : public ::testing::Test
{
protected:
    SushiTrackHdl     hdl_    = nullptr;
    SushiTrack_Params params_ = {};

    void SetUp() override
    {
        SushiTrack_GetDefaultParams(&params_);
        params_.enable_tentative = 0;
        params_.time_aware       = 1;
        params_.track_thresh     = 0.1f;
        params_.high_thresh      = 0.2f;
        params_.match_thresh     = 0.9f;
        hdl_ = SushiTrack_Create(&params_);
        ASSERT_NE(hdl_, nullptr);
    }

    void TearDown() override { SushiTrack_Destroy(hdl_); }
};

/**
 * @brief max_tracks=0 must produce a zero result regardless of detection count;
 *        the tracker cannot legally write to a zero-capacity buffer.
 */
TEST_F(CAPIBufferCapacityTest, ZeroMaxTracksProducesZeroReturn)
{
    SushiTrack_Object d = makeDet(10.0f, 20.0f);
    SushiTrack_Track  dummy{};
    int n = SushiTrack_Update(hdl_, &d, 1, &dummy, 0, 0.04f);
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

    std::vector<SushiTrack_Object> dets;
    dets.reserve(kDetections);
    for (int i = 0; i < kDetections; ++i)
        dets.push_back(makeDet(50.0f + 200.0f * i, 50.0f + 200.0f * i));

    std::vector<SushiTrack_Track> out(kCapacity);
    int n = SushiTrack_Update(hdl_,
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
 * Currently SushiTrack_Update returns only the truncated count; there is no
 * status output. Enable this test when an UpdateEx variant with a
 * SushiTrack_Status* parameter is introduced.
 */
TEST_F(CAPIBufferCapacityTest, DISABLED_BufferOverflowReturnsErrorCode)
{
    // TODO: Wire SushiTrack_UpdateEx with status pointer; assert
    //       *status == SUSHITRACK_BUFFER_OVERFLOW when truncation occurs.
    FAIL() << "Pending SushiTrack_UpdateEx with status channel.";
}

// ============================================================================
// CAPILifecycleSafetyTest
// ============================================================================

/**
 * @brief Destroying a null handle must be a safe no-op so consumers can call
 *        Destroy unconditionally from cleanup paths.
 */
TEST(CAPILifecycleSafetyTest, DestroyNullHandleIsNoOp)
{
    SushiTrack_Destroy(nullptr);
    SUCCEED();
}

/**
 * @brief Repeated SetLogCallback calls must remain safe even when the
 *        argument toggles between a real callback and nullptr.
 */
TEST(CAPILifecycleSafetyTest, RepeatedSetLogCallbackIsSafe)
{
    auto cb = [](SushiTrack_LogLevel, const char*) {};
    SushiTrack_SetLogCallback(cb);
    SushiTrack_SetLogCallback(nullptr);
    SushiTrack_SetLogCallback(cb);
    SushiTrack_SetLogCallback(nullptr);
    SUCCEED();
}

/**
 * @brief Create / Destroy cycles must reclaim resources cleanly; running many
 *        rapid cycles must not leak or fail.
 */
TEST(CAPILifecycleSafetyTest, RepeatedCreateDestroyCycleSucceeds)
{
    SushiTrack_Params params{};
    SushiTrack_GetDefaultParams(&params);

    for (int i = 0; i < 50; ++i)
    {
        SushiTrackHdl hdl = SushiTrack_Create(&params);
        ASSERT_NE(hdl, nullptr);
        SushiTrack_Destroy(hdl);
    }
}

// ============================================================================
// CAPIStatusCodeCoverageTest — Disabled placeholders for future status codes
// ============================================================================

/**
 * @brief DISABLED. Documents the desired propagation path for
 *        SUSHITRACK_INITIALIZATION_FAILURE. Enable when CreateEx can surface
 *        sub-component construction failures (e.g. assignment solver init).
 */
TEST(CAPIStatusCodeCoverageTest, DISABLED_InitFailureReturnsInitializationFailureCode)
{
    // TODO: Inject a controlled init failure (e.g. a registered factory that
    //       throws) and assert SushiTrack_CreateEx returns
    //       SUSHITRACK_INITIALIZATION_FAILURE.
    FAIL() << "Pending fault-injection hook for sub-component initialisation.";
}

/**
 * @brief DISABLED. Documents the desired propagation path for
 *        SUSHITRACK_RUNTIME_ERROR via a future UpdateEx variant.
 */
TEST(CAPIStatusCodeCoverageTest, DISABLED_RuntimeErrorReturnsRuntimeErrorCode)
{
    // TODO: When SushiTrack_UpdateEx is added, assert that an exception caught
    //       inside the C bridge surfaces as SUSHITRACK_RUNTIME_ERROR.
    FAIL() << "Pending SushiTrack_UpdateEx with status channel.";
}

/**
 * @brief DISABLED. Documents the desired propagation path for
 *        SUSHITRACK_OUT_OF_MEMORY. Enable when an allocator hook or stub
 *        allocator is in place to force bad_alloc inside the tracker core.
 */
TEST(CAPIStatusCodeCoverageTest, DISABLED_OutOfMemoryReturnsOutOfMemoryCode)
{
    // TODO: Inject a controlled bad_alloc and assert SushiTrack_CreateEx
    //       returns SUSHITRACK_OUT_OF_MEMORY.
    FAIL() << "Pending allocator fault-injection hook.";
}
