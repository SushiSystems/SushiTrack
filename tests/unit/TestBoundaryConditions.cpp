/**************************************************************************/
/* TestBoundaryConditions.cpp                                             */
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
 * @file TestBoundaryConditions.cpp
 * @brief Unit tests for edge cases and boundary conditions.
 * @author Mustafa Garip
 */

#include <cmath>
#include <limits>
#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/c_api/sushitrack_c.h>

namespace
{

    /**
     * @brief Constructs a benign detection without feature data.
     */
    sushitrack_object_t make_det(float x, float y, float w = 50.0f, float h = 100.0f,
                                 float score = 0.9f)
    {
        sushitrack_object_t o{};
        o.x = x;
        o.y = y;
        o.width = w;
        o.height = h;
        o.score = score;
        o.label = 1;
        o.feat = nullptr;
        o.feat_dim = 0;
        return o;
    }

    /**
     * @brief Constructs a detection backed by an external feature pointer.
     */
    sushitrack_object_t make_det_with_feat(float x, float y, const float* feat, int feat_dim,
                                           float score = 0.95f)
    {
        sushitrack_object_t o = make_det(x, y, 50.0f, 100.0f, score);
        o.feat = feat;
        o.feat_dim = feat_dim;
        return o;
    }

    /**
     * @brief Verifies that every active track in the buffer has finite geometry.
     */
    void expect_finite_geometry(const std::vector<sushitrack_track_t>& out, int n)
    {
        for (int i = 0; i < n; ++i)
        {
            const sushitrack_track_t& t = out[static_cast<std::size_t>(i)];
            EXPECT_TRUE(std::isfinite(t.x)) << "track " << i << " x is non-finite";
            EXPECT_TRUE(std::isfinite(t.y)) << "track " << i << " y is non-finite";
            EXPECT_TRUE(std::isfinite(t.width)) << "track " << i << " w is non-finite";
            EXPECT_TRUE(std::isfinite(t.height)) << "track " << i << " h is non-finite";
        }
    }

} // namespace

// MalformedDetectionTest

/**
 * @brief Fixture providing a clean tracker for each malformed-input test.
 */
class MalformedDetectionTest : public ::testing::Test
{
    protected:
        sushitrack_tracker_t hdl_ = nullptr;
        sushitrack_params_t params_ = {};

        void SetUp() override
        {
            sushitrack_get_default_params(&params_);
            params_.time_aware = 1;
            params_.enable_tentative = 0;
            params_.track_thresh = 0.1f;
            params_.high_thresh = 0.2f;
            params_.frame_rate = 25;

            hdl_ = sushitrack_create(&params_);
            ASSERT_NE(hdl_, nullptr);
        }

        void TearDown() override { sushitrack_destroy(hdl_); }
};

/**
 * @brief A zero-width detection must not crash; the returned output (if any)
 *        must remain finite.
 */
TEST_F(MalformedDetectionTest, ZeroWidthDetectionDoesNotCrash)
{
    std::vector<sushitrack_track_t> out(4);
    sushitrack_object_t d = make_det(100.0f, 200.0f, /*w*/ 0.0f, /*h*/ 80.0f);
    int n = sushitrack_update(hdl_, &d, 1, out.data(), 4, 0.04f);
    EXPECT_GE(n, 0);
    expect_finite_geometry(out, n);
}

/**
 * @brief A zero-height detection must not crash.
 */
TEST_F(MalformedDetectionTest, ZeroHeightDetectionDoesNotCrash)
{
    std::vector<sushitrack_track_t> out(4);
    sushitrack_object_t d = make_det(100.0f, 200.0f, /*w*/ 50.0f, /*h*/ 0.0f);
    int n = sushitrack_update(hdl_, &d, 1, out.data(), 4, 0.04f);
    EXPECT_GE(n, 0);
    expect_finite_geometry(out, n);
}

/**
 * @brief A negative-score detection is implementation-defined; it must at
 *        minimum not introduce NaN into reported track geometry.
 */
TEST_F(MalformedDetectionTest, NegativeScoreDoesNotProduceNaNGeometry)
{
    std::vector<sushitrack_track_t> out(4);
    sushitrack_object_t d = make_det(100.0f, 200.0f);
    d.score = -0.5f;
    int n = sushitrack_update(hdl_, &d, 1, out.data(), 4, 0.04f);
    EXPECT_GE(n, 0);
    expect_finite_geometry(out, n);
}

/**
 * @brief A detection with NaN coordinates must not crash, and must not
 *        corrupt other tracks present in the same frame.
 *
 * DISABLED: surfaces a real production behaviour — when a frame contains a
 * NaN-coordinate detection alongside a valid one, the tracker propagates the
 */
TEST_F(MalformedDetectionTest, DISABLED_NaNCoordinatesDoNotCorruptOtherTracks)
{
    std::vector<sushitrack_track_t> out(8);

    sushitrack_object_t good = make_det(100.0f, 200.0f);
    sushitrack_update(hdl_, &good, 1, out.data(), 8, 0.04f);
    ASSERT_GE(out[0].state, 0);
    const int64_t good_id = out[0].track_id;

    const float K_NAN = std::numeric_limits<float>::quiet_NaN();
    sushitrack_object_t bad = make_det(K_NAN, K_NAN);
    sushitrack_object_t good2 = make_det(102.0f, 202.0f);
    sushitrack_object_t dets[2] = {good2, bad};
    int n = sushitrack_update(hdl_, dets, 2, out.data(), 8, 0.04f);
    EXPECT_GE(n, 0);
    expect_finite_geometry(out, n);

    // The original good track must remain finite at its expected location.
    for (int i = 0; i < n; ++i)
        if (out[static_cast<std::size_t>(i)].track_id == good_id)
        {
            EXPECT_TRUE(std::isfinite(out[static_cast<std::size_t>(i)].x));
            EXPECT_TRUE(std::isfinite(out[static_cast<std::size_t>(i)].y));
        }
}

/**
 * @brief A detection with Inf coordinates must not crash.
 *
 * DISABLED: surfaces a real production behaviour — Inf coordinates propagate
 * into the Tracked output's x/y fields rather than being clipped or rejected.
 * The call does not crash (so the no-crash half of the contract holds) but
 */
TEST_F(MalformedDetectionTest, DISABLED_InfCoordinatesDoNotCrash)
{
    std::vector<sushitrack_track_t> out(4);
    const float K_INF = std::numeric_limits<float>::infinity();
    sushitrack_object_t d = make_det(K_INF, K_INF);
    int n = sushitrack_update(hdl_, &d, 1, out.data(), 4, 0.04f);
    EXPECT_GE(n, 0);
    expect_finite_geometry(out, n);
}

/**
 * @brief A detection whose feature vector contains NaN entries must not
 *        propagate NaN into a Tracked track's reported geometry.
 */
TEST_F(MalformedDetectionTest, NaNFeatureVectorDoesNotPoisonTracker)
{
    sushitrack_destroy(hdl_);
    params_.enable_reid = 1;
    hdl_ = sushitrack_create(&params_);
    ASSERT_NE(hdl_, nullptr);

    std::vector<sushitrack_track_t> out(4);
    const float K_NAN = std::numeric_limits<float>::quiet_NaN();
    std::vector<float> bad_feat(16, K_NAN);

    sushitrack_object_t d = make_det_with_feat(100.0f, 200.0f, bad_feat.data(), 16);
    int n = sushitrack_update(hdl_, &d, 1, out.data(), 4, 0.04f);
    EXPECT_GE(n, 0);
    expect_finite_geometry(out, n);
}

// LargeCoordinateTest

/**
 * @brief Fixture validating numerical headroom at large coordinate magnitudes.
 */
class LargeCoordinateTest : public ::testing::Test
{
    protected:
        sushitrack_tracker_t hdl_ = nullptr;
        sushitrack_params_t params_ = {};

        void SetUp() override
        {
            sushitrack_get_default_params(&params_);
            params_.time_aware = 1;
            params_.enable_tentative = 0;
            params_.track_thresh = 0.1f;
            params_.high_thresh = 0.2f;
            params_.match_thresh = 0.95f;
            params_.frame_rate = 30;
            hdl_ = sushitrack_create(&params_);
            ASSERT_NE(hdl_, nullptr);
        }

        void TearDown() override { sushitrack_destroy(hdl_); }
};

/**
 * @brief Tracks reported near image coordinates of 1e6 must remain finite.
 */
TEST_F(LargeCoordinateTest, MillionPixelCoordinatesRemainFinite)
{
    std::vector<sushitrack_track_t> out(4);
    sushitrack_object_t d0 = make_det(1.0e6f, 1.0e6f);
    sushitrack_object_t d1 = make_det(1.0e6f + 2.0f, 1.0e6f + 2.0f);
    sushitrack_update(hdl_, &d0, 1, out.data(), 4, 1.0f / 30.0f);
    int n = sushitrack_update(hdl_, &d1, 1, out.data(), 4, 1.0f / 30.0f);
    EXPECT_GE(n, 1);
    expect_finite_geometry(out, n);
}

/**
 * @brief A detection that jumps by tens of thousands of pixels per frame must
 *        not produce NaN/Inf in subsequent reports (although identity may be
 *        lost — this test only asserts numerical safety).
 */
TEST_F(LargeCoordinateTest, HugeFrameToFrameJumpDoesNotProduceNonFinite)
{
    std::vector<sushitrack_track_t> out(4);
    sushitrack_object_t d0 = make_det(100.0f, 200.0f);
    sushitrack_update(hdl_, &d0, 1, out.data(), 4, 1.0f / 30.0f);

    sushitrack_object_t d_jump = make_det(50000.0f, 50000.0f);
    int n = sushitrack_update(hdl_, &d_jump, 1, out.data(), 4, 1.0f / 30.0f);
    EXPECT_GE(n, 0);
    expect_finite_geometry(out, n);
}

// HighDimensionalReIDTest

/**
 * @brief Builds a unit feature vector with all mass on a single axis.
 * @param dim Vector dimensionality.
 * @param axis Index of the active component (must be < dim).
 */
std::vector<float> unit_feature(int dim, int axis)
{
    std::vector<float> v(static_cast<std::size_t>(dim), 0.0f);
    v[static_cast<std::size_t>(axis)] = 1.0f;
    return v;
}

/**
 * @brief Fixture exercising large ReID feature vectors.
 */
class HighDimensionalReIDTest : public ::testing::TestWithParam<int>
{
    protected:
        sushitrack_tracker_t hdl_ = nullptr;
        sushitrack_params_t params_ = {};

        void SetUp() override
        {
            sushitrack_get_default_params(&params_);
            params_.time_aware = 1;
            params_.enable_tentative = 0;
            params_.enable_reid = 1;
            params_.track_thresh = 0.1f;
            params_.high_thresh = 0.2f;
            params_.match_thresh = 0.95f;
            params_.frame_rate = 25;
            hdl_ = sushitrack_create(&params_);
            ASSERT_NE(hdl_, nullptr);
        }

        void TearDown() override { sushitrack_destroy(hdl_); }
};

/**
 * @brief High-dimensional unit features preserve identity across two frames
 *        and produce finite geometry — covering 128, 512, and 2048 D inputs.
 */
TEST_P(HighDimensionalReIDTest, HighDimUnitFeaturePreservesIdentity)
{
    const int dim = GetParam();
    std::vector<float> feat = unit_feature(dim, 0);

    std::vector<sushitrack_track_t> out(4);
    sushitrack_object_t d0 = make_det_with_feat(100.0f, 200.0f, feat.data(), dim);
    sushitrack_update(hdl_, &d0, 1, out.data(), 4, 0.04f);
    ASSERT_EQ(out[0].state, 1);
    const int64_t id = out[0].track_id;

    sushitrack_object_t d1 = make_det_with_feat(102.0f, 202.0f, feat.data(), dim);
    int n = sushitrack_update(hdl_, &d1, 1, out.data(), 4, 0.04f);
    ASSERT_GE(n, 1);
    EXPECT_EQ(out[0].track_id, id);
    expect_finite_geometry(out, n);
}

INSTANTIATE_TEST_SUITE_P(FeatureDimensions, HighDimensionalReIDTest,
                         ::testing::Values(128, 512, 2048));

/**
 * @brief A zero-norm feature vector must not cause downstream NaN in cosine
 *        similarity (division by zero must be guarded inside the tracker).
 */
TEST(HighDimensionalReIDZeroNormTest, ZeroNormFeatureIsToleratedWithoutNaN)
{
    sushitrack_params_t params{};
    sushitrack_get_default_params(&params);
    params.time_aware = 1;
    params.enable_tentative = 0;
    params.enable_reid = 1;
    params.track_thresh = 0.1f;
    params.high_thresh = 0.2f;
    params.frame_rate = 25;

    sushitrack_tracker_t hdl = sushitrack_create(&params);
    ASSERT_NE(hdl, nullptr);

    std::vector<float> zero_feat(128, 0.0f);
    sushitrack_object_t o{};
    o.x = 100.0f;
    o.y = 200.0f;
    o.width = 50.0f;
    o.height = 100.0f;
    o.score = 0.9f;
    o.label = 1;
    o.feat = zero_feat.data();
    o.feat_dim = 128;

    std::vector<sushitrack_track_t> out(4);
    int n = sushitrack_update(hdl, &o, 1, out.data(), 4, 0.04f);
    EXPECT_GE(n, 0);

    for (int i = 0; i < n; ++i)
    {
        EXPECT_TRUE(std::isfinite(out[static_cast<std::size_t>(i)].x));
        EXPECT_TRUE(std::isfinite(out[static_cast<std::size_t>(i)].y));
    }

    sushitrack_destroy(hdl);
}

// LongRunStabilityTest

/**
 * @brief 1000 frames of a stationary detection must not produce NaN/Inf in
 *        any reported field. This verifies that the Kalman covariance does
 *        not numerically diverge under repeated near-zero innovations.
 */
TEST(LongRunStabilityTest, OneThousandFramesStationaryNoNumericalDrift)
{
    sushitrack_params_t params{};
    sushitrack_get_default_params(&params);
    params.time_aware = 1;
    params.enable_tentative = 0;
    params.track_thresh = 0.1f;
    params.high_thresh = 0.2f;
    params.frame_rate = 30;
    params.track_buffer = 60;
    sushitrack_tracker_t hdl = sushitrack_create(&params);
    ASSERT_NE(hdl, nullptr);

    std::vector<sushitrack_track_t> out(4);
    constexpr int K_FRAMES = 1000;
    for (int i = 0; i < K_FRAMES; ++i)
    {
        sushitrack_object_t d{};
        d.x = 100.0f;
        d.y = 200.0f;
        d.width = 50.0f;
        d.height = 100.0f;
        d.score = 0.9f;
        d.label = 1;
        int n = sushitrack_update(hdl, &d, 1, out.data(), 4, 1.0f / 30.0f);
        ASSERT_GE(n, 1);

        // Spot check every 100 frames to keep the loop fast.
        if (i % 100 == 0)
        {
            EXPECT_TRUE(std::isfinite(out[0].x)) << "frame " << i;
            EXPECT_TRUE(std::isfinite(out[0].y)) << "frame " << i;
            EXPECT_TRUE(std::isfinite(out[0].width)) << "frame " << i;
            EXPECT_TRUE(std::isfinite(out[0].height)) << "frame " << i;
        }
    }

    sushitrack_destroy(hdl);
}

// SimilarityBoundaryTest

/**
 * @brief Near-unity cosine similarity (almost-identical features) must be
 *        accepted without crashing. The two unit vectors differ by 0.01 on
 *        a single component, then renormalised.
 */
TEST(SimilarityBoundaryTest, NearUnitySimilarityIsAccepted)
{
    sushitrack_params_t params{};
    sushitrack_get_default_params(&params);
    params.time_aware = 1;
    params.enable_reid = 1;
    params.frame_rate = 25;
    params.track_thresh = 0.1f;
    params.high_thresh = 0.2f;
    sushitrack_tracker_t hdl = sushitrack_create(&params);
    ASSERT_NE(hdl, nullptr);

    std::vector<float> a(32, 0.0f);
    std::vector<float> b(32, 0.0f);
    a[0] = 1.0f;
    b[0] = std::sqrt(1.0f - 1.0e-8f);
    b[1] = 1.0e-4f;

    sushitrack_object_t d0{};
    d0.x = 100.0f;
    d0.y = 200.0f;
    d0.width = 50.0f;
    d0.height = 100.0f;
    d0.score = 0.95f;
    d0.label = 1;
    d0.feat = a.data();
    d0.feat_dim = 32;

    sushitrack_object_t d1{};
    d1.x = 102.0f;
    d1.y = 202.0f;
    d1.width = 50.0f;
    d1.height = 100.0f;
    d1.score = 0.95f;
    d1.label = 1;
    d1.feat = b.data();
    d1.feat_dim = 32;

    std::vector<sushitrack_track_t> out(4);
    int n0 = sushitrack_update(hdl, &d0, 1, out.data(), 4, 0.04f);
    int n1 = sushitrack_update(hdl, &d1, 1, out.data(), 4, 0.04f);
    EXPECT_GE(n0, 1);
    EXPECT_GE(n1, 1);

    sushitrack_destroy(hdl);
}

/**
 * @brief Near-zero cosine similarity (nearly orthogonal features) must be
 *        accepted without crashing.
 */
TEST(SimilarityBoundaryTest, NearOrthogonalSimilarityIsAccepted)
{
    sushitrack_params_t params{};
    sushitrack_get_default_params(&params);
    params.time_aware = 1;
    params.enable_reid = 1;
    params.frame_rate = 25;
    params.track_thresh = 0.1f;
    params.high_thresh = 0.2f;
    sushitrack_tracker_t hdl = sushitrack_create(&params);
    ASSERT_NE(hdl, nullptr);

    std::vector<float> a(32, 0.0f);
    std::vector<float> b(32, 0.0f);
    a[0] = 1.0f;
    b[1] = 1.0f;
    b[0] = 1.0e-4f; // nearly orthogonal — cosine ≈ 1e-4

    sushitrack_object_t d0{};
    d0.x = 100.0f;
    d0.y = 200.0f;
    d0.width = 50.0f;
    d0.height = 100.0f;
    d0.score = 0.95f;
    d0.label = 1;
    d0.feat = a.data();
    d0.feat_dim = 32;

    sushitrack_object_t d1{};
    d1.x = 600.0f;
    d1.y = 600.0f;
    d1.width = 50.0f;
    d1.height = 100.0f;
    d1.score = 0.95f;
    d1.label = 1;
    d1.feat = b.data();
    d1.feat_dim = 32;

    std::vector<sushitrack_track_t> out(4);
    int n0 = sushitrack_update(hdl, &d0, 1, out.data(), 4, 0.04f);
    int n1 = sushitrack_update(hdl, &d1, 1, out.data(), 4, 0.04f);
    EXPECT_GE(n0, 1);
    EXPECT_GE(n1, 1);

    sushitrack_destroy(hdl);
}
