/**************************************************************************/
/* test_appearance_features.cpp                                           */
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
 * @file test_appearance_features.cpp
 * @brief Tests the appearance features Tracklet keeps.
 * @author Mustafa Garip
 */

#include <cmath>
#include <cstddef>
#include <memory>
#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/detection/rect.hpp>
#include <SushiTrack/filtering/kalman_filter.hpp>
#include <SushiTrack/tracking/tracklet.hpp>

#include <captured_log.hpp>
#include <support/tolerance.hpp>
#include <tracklet_builders.hpp>

using namespace SushiTrack;

namespace
{
    /**
     * @brief Rounded operations on the longest path to a stored norm: the three squares, two sums,
     *        square root and quotient of the normalisation, then the same six of the test's norm.
     */
    constexpr std::size_t K_NORM_ROUNDED_OPERATIONS = 13;

    /** @brief Creates a Tracklet with the given max_feature_history limit. */
    std::unique_ptr<Tracklet> make_tracklet(int max_hist = 100, float alpha = 0.9f)
    {
        return std::make_unique<Tracklet>(std::make_unique<KalmanFilter>(),
                                          Rect<float>(0.f, 0.f, 50.f, 80.f),
                                          /*label=*/1, /*score=*/0.9f,
                                          /*feat=*/std::vector<float>{}, alpha,
                                          /*enable_xyah=*/false, max_hist);
    }

    /** @brief Returns a unit vector of size n along @p axis. */
    std::vector<float> unit_basis(int n, int axis = 0)
    {
        std::vector<float> v(static_cast<std::size_t>(n), 0.f);
        v[static_cast<std::size_t>(axis)] = 1.f;
        return v;
    }

    /** @brief Computes the L2 norm of a vector. */
    float l2_norm(const std::vector<float>& v)
    {
        float s = 0.f;
        for (float x : v)
            s += x * x;
        return std::sqrt(s);
    }

    /** @brief Whether an update was made before has_feature() is read, and what it must read. */
    struct HasFeatureRow
    {
            bool updated;
            bool expected;
    };

    /** @brief A feature to store and whether the smoothed or the current vector is measured. */
    struct NormRow
    {
            std::vector<float> input;
            bool measure_current;
    };
} // namespace

/**
 * @brief The has_feature() rows, run over a tracklet before and after one update.
 */
class Small_TrackletHasFeatureTest : public ::testing::TestWithParam<HasFeatureRow>
{
};

/**
 * @brief has_feature() is false until the first update_features() call and true after it.
 */
TEST_P(Small_TrackletHasFeatureTest, FollowsTheFirstUpdate)
{
    auto t = make_tracklet();
    if (GetParam().updated)
        t->update_features({1.f, 0.f, 0.f});

    EXPECT_EQ(t->has_feature(), GetParam().expected);
}

INSTANTIATE_TEST_SUITE_P(Updates, Small_TrackletHasFeatureTest,
                         ::testing::Values(HasFeatureRow{false, false}, HasFeatureRow{true, true}));

/**
 * @brief The normalisation rows, run over a non-unit input and the vector measured.
 */
class Small_TrackletNormalisationTest : public ::testing::TestWithParam<NormRow>
{
};

/**
 * @brief update_features() stores a unit-length smooth_feat and curr_feat for a non-unit input.
 */
TEST_P(Small_TrackletNormalisationTest, StoredFeatureIsUnitNormalised)
{
    auto t = make_tracklet();
    t->update_features(GetParam().input);

    const std::vector<float>& stored =
        GetParam().measure_current ? t->get_curr_feat() : t->get_smooth_feat();

    EXPECT_TRUE(TestSupport::is_within(TestSupport::measure_relative_error<float>(
        l2_norm(stored), 1.0, K_NORM_ROUNDED_OPERATIONS)));
}

INSTANTIATE_TEST_SUITE_P(Vectors, Small_TrackletNormalisationTest,
                         ::testing::Values(NormRow{{3.f, 4.f, 0.f}, false},
                                           NormRow{{0.f, 3.f, 4.f}, true}));

/**
 * @brief After two update_features() calls with alpha 0.5, smooth_feat keeps a component of
 *        each of the two orthogonal unit observations: the EMA blends them.
 */
TEST(Small_TrackletAppearanceTest, EMABlendsConsecutiveFeatures)
{
    auto t = make_tracklet(100, 0.5f);

    t->update_features({1.f, 0.f, 0.f});
    t->update_features({0.f, 1.f, 0.f});

    const auto& sf = t->get_smooth_feat();
    ASSERT_EQ(sf.size(), static_cast<size_t>(3));
    EXPECT_GT(sf[0], 0.f) << "EMA should retain component of e1";
    EXPECT_GT(sf[1], 0.f) << "EMA should have component of e2";
}

/**
 * @brief Querying with the feature that was stored gives exactly 1: the dot product of a unit
 *        basis vector with itself is 1 * 1.
 */
TEST(Small_TrackletAppearanceTest, SimilarityIdenticalFeatureIsOne)
{
    auto t = make_tracklet();
    t->update_features(unit_basis(4, 0));

    EXPECT_FLOAT_EQ(t->get_max_feature_similarity(unit_basis(4, 0)), 1.f);
}

/**
 * @brief Querying with an orthogonal basis vector gives exactly 0: every product has a zero
 *        factor.
 */
TEST(Small_TrackletAppearanceTest, SimilarityOrthogonalFeatureIsZero)
{
    auto t = make_tracklet();
    t->update_features(unit_basis(4, 0));

    EXPECT_FLOAT_EQ(t->get_max_feature_similarity(unit_basis(4, 1)), 0.f);
}

/**
 * @brief get_max_feature_similarity() scans the history, not only smooth_feat.
 *
 * The original vector is followed by 30 updates of an orthogonal one, so the smoothed feature has
 * moved away. The history holds 31 entries under its capacity of 50, so the original is still
 * in it and its similarity with itself is exactly 1.
 */
TEST(Small_TrackletAppearanceTest, SimilarityChecksHistory)
{
    auto t = make_tracklet(50, 0.9f);
    t->update_features(unit_basis(4, 0));
    for (int i = 0; i < 30; ++i)
        t->update_features(unit_basis(4, 1));

    EXPECT_FLOAT_EQ(t->get_max_feature_similarity(unit_basis(4, 0)), 1.f);
}

/**
 * @brief Calling update_features() with an empty vector leaves has_feature() false.
 */
TEST(Small_TrackletAppearanceTest, EmptyFeatureVectorIsNoOp)
{
    auto t = make_tracklet();
    ASSERT_FALSE(t->has_feature());

    t->update_features(std::vector<float>{});

    EXPECT_FALSE(t->has_feature());
}

/**
 * @brief Features of changing lengths log one WARN naming both lengths, however many times the
 *        length changes.
 */
TEST(Small_TrackletAppearanceTest, ChangingFeatureLengthWarnsOnce)
{
    CapturedLog log;
    auto track = SushiTrack::make_feat_t(10.f, 10.f, 4, 0);

    track->update_features(std::vector<float>(8, 1.0f));
    track->update_features(std::vector<float>(4, 1.0f));
    track->update_features(std::vector<float>(8, 1.0f));

    EXPECT_EQ(log.count("Feature length 8 does not match the track's 4"), 1);
    EXPECT_EQ(log.level_of("does not match"), SushiLog::LogLevel::Warn);
}
