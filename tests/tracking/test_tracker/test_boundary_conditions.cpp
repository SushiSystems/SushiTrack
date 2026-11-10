/**************************************************************************/
/* test_boundary_conditions.cpp                                           */
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
 * @file test_boundary_conditions.cpp
 * @brief Tests the tracker on malformed and extreme input.
 * @author Mustafa Garip
 */

#include <cmath>
#include <limits>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/c_api/sushitrack_c.h>

#include <c_detection_builders.hpp>
#include <support/tolerance.hpp>

using CDetections::find_by_id;
using CDetections::make_det;
using CDetections::make_det_with_feat;
using CDetections::one_hot_feature;
using CDetections::update_frame;

namespace
{
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

    /**
     * @brief Returns the last track of the first @p n in @p out whose x is above @p x, or null.
     */
    const sushitrack_track_t* find_track_right_of(const std::vector<sushitrack_track_t>& out, int n,
                                                  float x)
    {
        const sushitrack_track_t* found = nullptr;
        for (int i = 0; i < n; ++i)
        {
            if (out[static_cast<std::size_t>(i)].x > x)
                found = &out[static_cast<std::size_t>(i)];
        }
        return found;
    }

    /**
     * @brief One malformed or degenerate detection and whether its feature is NaN.
     */
    struct MalformedRow
    {
            const char* name;
            float x;
            float y;
            float width;
            float height;
            float score;
            bool nan_feature;
    };

    constexpr float K_INF = std::numeric_limits<float>::infinity();

    /**
     * @brief One step of a large-coordinate row: the box of the first frame and of the second.
     */
    struct JumpRow
    {
            const char* name;
            float x0;
            float y0;
            float x1;
            float y1;
            int min_tracks;
    };

    /**
     * @brief Returns the detection of @p row, with @p nan_feature as its feature when the row
     *        asks for a NaN feature.
     */
    sushitrack_object_t malformed_detection(const MalformedRow& row,
                                            const std::vector<float>& nan_feature)
    {
        sushitrack_object_t detection =
            make_det(row.x, row.y, row.score, row.width, row.height);
        if (row.nan_feature)
        {
            detection.feat = nan_feature.data();
            detection.feat_dim = static_cast<int>(nan_feature.size());
        }
        return detection;
    }

    /**
     * @brief Feeds @p frames frames of the stationary box (100, 200, 50, 100) at 30 fps and
     *        leaves the last frame's tracks in @p out; returns the last frame's track count.
     */
    int feed_stationary_frames(sushitrack_tracker_t tracker, std::vector<sushitrack_track_t>& out,
                               int frames)
    {
        int count = 0;
        const sushitrack_object_t detection = make_det(100.0f, 200.0f);
        for (int frame = 0; frame < frames; ++frame)
            count = update_frame(tracker, &detection, 1, out.data(), 4, 1.0f / 30.0f);
        return count;
    }
} // namespace

/**
 * @brief Fixture providing a clean tracker for each malformed-input row; a row with a NaN
 *        feature turns ReID on.
 */
class Small_TrackerMalformedDetectionTest : public ::testing::TestWithParam<MalformedRow>
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
            params_.enable_reid = GetParam().nan_feature ? 1 : 0;

            hdl_ = sushitrack_create(&params_);
            ASSERT_NE(hdl_, nullptr);
        }

        void TearDown() override { sushitrack_destroy(hdl_); }
};

/**
 * @brief A degenerate or non-finite detection does not crash the update and leaves every
 *        reported track with finite geometry.
 */
TEST_P(Small_TrackerMalformedDetectionTest, LeavesEveryReportedTrackFinite)
{
    std::vector<sushitrack_track_t> out(4);
    const std::vector<float> nan_feature(16, std::numeric_limits<float>::quiet_NaN());
    const sushitrack_object_t detection = malformed_detection(GetParam(), nan_feature);

    const int n = update_frame(hdl_, &detection, 1, out.data(), 4, 0.04f);

    EXPECT_GE(n, 0);
    expect_finite_geometry(out, n);
}

INSTANTIATE_TEST_SUITE_P(
    DegenerateInput, Small_TrackerMalformedDetectionTest,
    ::testing::Values(MalformedRow{"ZeroWidth", 100.0f, 200.0f, 0.0f, 80.0f, 0.9f, false},
                      MalformedRow{"ZeroHeight", 100.0f, 200.0f, 50.0f, 0.0f, 0.9f, false},
                      MalformedRow{"NegativeScore", 100.0f, 200.0f, 50.0f, 100.0f, -0.5f, false},
                      MalformedRow{"InfCoordinates", K_INF, K_INF, 50.0f, 100.0f, 0.9f, false},
                      MalformedRow{"NaNFeature", 100.0f, 200.0f, 50.0f, 100.0f, 0.95f, true}),
    [](const ::testing::TestParamInfo<MalformedRow>& info)
    {
        return std::string(info.param.name);
    });

/**
 * @brief A detection with NaN coordinates is ignored: the call does not crash, no track
 *        appears for it, and the other track of the same frame stays on its path.
 *
 * The good track starts at (100, 200) with zero velocity, so the second frame predicts the same
 * place; its detection at (102, 202) pulls the estimate by a gain between 0 and 1, so the
 * reported position lies in [100, 102] by [200, 202].
 */
TEST(Small_TrackerNaNDetectionTest, NaNCoordinatesDoNotCorruptOtherTracks)
{
    sushitrack_params_t params{};
    sushitrack_get_default_params(&params);
    params.time_aware = 1;
    params.enable_tentative = 0;
    params.track_thresh = 0.1f;
    params.high_thresh = 0.2f;
    params.frame_rate = 25;
    sushitrack_tracker_t hdl = sushitrack_create(&params);
    ASSERT_NE(hdl, nullptr);
    std::vector<sushitrack_track_t> out(8);
    const sushitrack_object_t good = make_det(100.0f, 200.0f);
    update_frame(hdl, &good, 1, out.data(), 8, 0.04f);
    const int64_t good_id = out[0].track_id;
    const float not_a_number = std::numeric_limits<float>::quiet_NaN();
    const sushitrack_object_t detections[2] = {make_det(102.0f, 202.0f), make_det(not_a_number, not_a_number)};

    const int n = update_frame(hdl, detections, 2, out.data(), 8, 0.04f);

    EXPECT_EQ(n, 1);
    const sushitrack_track_t* kept = find_by_id(out, n, good_id);
    ASSERT_NE(kept, nullptr);
    EXPECT_GE(kept->x, 100.0f);
    EXPECT_LE(kept->x, 102.0f);
    EXPECT_GE(kept->y, 200.0f);
    EXPECT_LE(kept->y, 202.0f);
    sushitrack_destroy(hdl);
}

/**
 * @brief Fixture validating numerical headroom at large coordinate magnitudes.
 */
class Small_TrackerLargeCoordinateTest : public ::testing::TestWithParam<JumpRow>
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
 * @brief Tracks stay finite at coordinates of 1e6, and after a jump of tens of thousands of
 *        pixels in one frame, where identity may be lost.
 */
TEST_P(Small_TrackerLargeCoordinateTest, ReportedTracksStayFinite)
{
    std::vector<sushitrack_track_t> out(4);
    const sushitrack_object_t first = make_det(GetParam().x0, GetParam().y0);
    const sushitrack_object_t second = make_det(GetParam().x1, GetParam().y1);
    update_frame(hdl_, &first, 1, out.data(), 4, 1.0f / 30.0f);

    const int n = update_frame(hdl_, &second, 1, out.data(), 4, 1.0f / 30.0f);

    EXPECT_GE(n, GetParam().min_tracks);
    expect_finite_geometry(out, n);
}

INSTANTIATE_TEST_SUITE_P(
    Magnitudes, Small_TrackerLargeCoordinateTest,
    ::testing::Values(
        // a box near 1e6 that moves 2 px: the track continues
        JumpRow{"MillionPixels", 1.0e6f, 1.0e6f, 1.0e6f + 2.0f, 1.0e6f + 2.0f, 1},
        // a box that jumps from (100, 200) to (50000, 50000): the track may be lost
        JumpRow{"HugeJump", 100.0f, 200.0f, 50000.0f, 50000.0f, 0}),
    [](const ::testing::TestParamInfo<JumpRow>& info)
    {
        return std::string(info.param.name);
    });

/**
 * @brief Fixture exercising large ReID feature vectors.
 */
class Small_TrackerHighDimensionalReIDTest : public ::testing::TestWithParam<int>
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
TEST_P(Small_TrackerHighDimensionalReIDTest, HighDimUnitFeaturePreservesIdentity)
{
    const int dim = GetParam();
    const std::vector<float> feat = one_hot_feature(dim, 0);

    std::vector<sushitrack_track_t> out(4);
    const sushitrack_object_t d0 = make_det_with_feat(100.0f, 200.0f, feat.data(), dim, 0.95f);
    update_frame(hdl_, &d0, 1, out.data(), 4, 0.04f);
    ASSERT_EQ(out[0].state, 1);
    const int64_t id = out[0].track_id;

    const sushitrack_object_t d1 = make_det_with_feat(102.0f, 202.0f, feat.data(), dim, 0.95f);
    const int n = update_frame(hdl_, &d1, 1, out.data(), 4, 0.04f);

    ASSERT_GE(n, 1);
    EXPECT_EQ(out[0].track_id, id);
    expect_finite_geometry(out, n);
}

INSTANTIATE_TEST_SUITE_P(FeatureDimensions, Small_TrackerHighDimensionalReIDTest,
                         ::testing::Values(128, 512, 2048));

/**
 * @brief A zero-norm feature vector must not cause downstream NaN in cosine
 *        similarity (division by zero must be guarded inside the tracker).
 */
TEST(Small_TrackerHighDimensionalReIDZeroNormTest, ZeroNormFeatureIsToleratedWithoutNaN)
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
    const std::vector<float> zero_feat(128, 0.0f);
    const sushitrack_object_t detection = make_det_with_feat(100.0f, 200.0f, zero_feat.data(), 128);
    std::vector<sushitrack_track_t> out(4);

    const int n = update_frame(hdl, &detection, 1, out.data(), 4, 0.04f);

    EXPECT_GE(n, 0);
    expect_finite_geometry(out, n);
    sushitrack_destroy(hdl);
}

/**
 * @brief 1000 frames of a stationary detection leave the track exactly where the detection is.
 *
 * Every innovation is zero, since the estimate equals the detection, so the Kalman update adds
 * nothing and the covariance has no cause to diverge. The box is dyadic (100, 200, 50, 100),
 * so the conversions to centre form and back are exact. The bound is 16 u times the value,
 * which only rounding noise would use.
 */
TEST(Small_TrackerLongRunStabilityTest, OneThousandFramesStationaryNoNumericalDrift)
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

    const int n = feed_stationary_frames(hdl, out, 1000);

    ASSERT_GE(n, 1);
    const double unit = TestSupport::unit_roundoff<float>();
    EXPECT_TRUE(TestSupport::is_within(
        TestSupport::measure_error(out[0].x, 100.0, 16.0 * unit * 100.0)));
    EXPECT_TRUE(TestSupport::is_within(
        TestSupport::measure_error(out[0].y, 200.0, 16.0 * unit * 200.0)));
    EXPECT_TRUE(TestSupport::is_within(
        TestSupport::measure_error(out[0].width, 50.0, 16.0 * unit * 50.0)));
    EXPECT_TRUE(TestSupport::is_within(
        TestSupport::measure_error(out[0].height, 100.0, 16.0 * unit * 100.0)));
    sushitrack_destroy(hdl);
}

/**
 * @brief Two nearly identical features (cosine near 1) keep one object's id across two frames.
 *
 * The unit vectors differ by 1e-4 on one component, then are renormalised.
 */
TEST(Small_TrackerSimilarityBoundaryTest, NearUnitySimilarityKeepsTheId)
{
    sushitrack_params_t params{};
    sushitrack_get_default_params(&params);
    params.time_aware = 1;
    params.enable_tentative = 0;
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
    const sushitrack_object_t d0 = make_det_with_feat(100.0f, 200.0f, a.data(), 32, 0.95f);
    const sushitrack_object_t d1 = make_det_with_feat(102.0f, 202.0f, b.data(), 32, 0.95f);
    std::vector<sushitrack_track_t> out(4);
    update_frame(hdl, &d0, 1, out.data(), 4, 0.04f);
    const int64_t first_id = out[0].track_id;

    const int n = update_frame(hdl, &d1, 1, out.data(), 4, 0.04f);

    ASSERT_GE(n, 1);
    EXPECT_NE(find_by_id(out, n, first_id), nullptr);
    sushitrack_destroy(hdl);
}

/**
 * @brief A far detection with a nearly orthogonal feature (cosine about 1e-4) starts a track
 *        with a new id.
 */
TEST(Small_TrackerSimilarityBoundaryTest, NearOrthogonalSimilarityGivesAFreshId)
{
    sushitrack_params_t params{};
    sushitrack_get_default_params(&params);
    params.time_aware = 1;
    params.enable_tentative = 0;
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
    b[0] = 1.0e-4f;
    const sushitrack_object_t d0 = make_det_with_feat(100.0f, 200.0f, a.data(), 32, 0.95f);
    const sushitrack_object_t d1 = make_det_with_feat(600.0f, 600.0f, b.data(), 32, 0.95f);
    std::vector<sushitrack_track_t> out(4);
    update_frame(hdl, &d0, 1, out.data(), 4, 0.04f);
    const int64_t first_id = out[0].track_id;

    const int n = update_frame(hdl, &d1, 1, out.data(), 4, 0.04f);

    ASSERT_GE(n, 1);
    const sushitrack_track_t* far = find_track_right_of(out, n, 500.0f);
    ASSERT_NE(far, nullptr);
    EXPECT_NE(far->track_id, first_id);
    sushitrack_destroy(hdl);
}
