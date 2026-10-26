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
 * @file test_appearance_features.cpp
 * @brief Tests the appearance features Tracklet keeps.
 * @author Mustafa Garip
 */

#include <cmath>
#include <memory>
#include <numeric>
#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/detection/rect.hpp>
#include <SushiTrack/filtering/kalman_filter.hpp>
#include <SushiTrack/tracking/tracklet.hpp>

#include <captured_log.hpp>
#include <tracklet_builders.hpp>

using namespace SushiTrack;

// Helpers

/** @brief Creates a Tracklet with the given max_feature_history limit. */
static std::unique_ptr<Tracklet> make_tracklet(int max_hist = 100, float alpha = 0.9f)
{
    return std::make_unique<Tracklet>(std::make_unique<KalmanFilter>(),
                                      Rect<float>(0.f, 0.f, 50.f, 80.f),
                                      /*label=*/1, /*score=*/0.9f,
                                      /*feat=*/std::vector<float>{}, alpha,
                                      /*enable_xyah=*/false, max_hist);
}

/** @brief Returns a unit vector [1, 0, 0, ...] of size n. */
static std::vector<float> unit_basis(int n, int axis = 0)
{
    std::vector<float> v(n, 0.f);
    v[axis] = 1.f;
    return v;
}

/** @brief Computes the L2 norm of a vector. */
static float l2Norm(const std::vector<float>& v)
{
    float s = 0.f;
    for (float x : v)
        s += x * x;
    return std::sqrt(s);
}

// has_feature()

/**
 * @brief A freshly constructed Tracklet has no feature (has_feature = false).
 */
TEST(Small_TrackletAppearanceTest, NoFeatureInitially)
{
    auto t = make_tracklet();
    EXPECT_FALSE(t->has_feature());
}

/**
 * @brief After one update_features() call, has_feature() returns true.
 */
TEST(Small_TrackletAppearanceTest, HasFeatureAfterUpdate)
{
    auto t = make_tracklet();
    t->update_features({1.f, 0.f, 0.f});
    EXPECT_TRUE(t->has_feature());
}

// L2 normalisation

/**
 * @brief smooth_feat is unit-normalised after update_features() for a
 *        non-unit input vector.
 */
TEST(Small_TrackletAppearanceTest, SmoothFeatIsUnitNormalised)
{
    auto t = make_tracklet();
    t->update_features({3.f, 4.f, 0.f}); // L2 norm = 5

    const auto& sf = t->get_smooth_feat();
    EXPECT_NEAR(l2Norm(sf), 1.f, 1e-5f);
}

/**
 * @brief curr_feat is unit-normalised after update_features().
 */
TEST(Small_TrackletAppearanceTest, CurrFeatIsUnitNormalised)
{
    auto t = make_tracklet();
    t->update_features({0.f, 3.f, 4.f});

    const auto& cf = t->get_curr_feat();
    EXPECT_NEAR(l2Norm(cf), 1.f, 1e-5f);
}

// EMA blending

/**
 * @brief After two update_features() calls, smooth_feat lies strictly between
 *        the two unit observations (EMA blends them).
 *
 * alpha = 0.9: smooth = 0.9 * e1 + 0.1 * e2 (normalised)
 * The smooth vector's projection onto e1 must be > 0 and onto e2 must be > 0.
 */
TEST(Small_TrackletAppearanceTest, EMABlendsConsecutiveFeatures)
{
    auto t = make_tracklet(100, 0.5f);

    std::vector<float> e1 = {1.f, 0.f, 0.f};
    std::vector<float> e2 = {0.f, 1.f, 0.f};

    t->update_features(e1);
    t->update_features(e2);

    const auto& sf = t->get_smooth_feat();
    ASSERT_EQ(sf.size(), static_cast<size_t>(3));

    float proj_e1 = sf[0];
    float proj_e2 = sf[1];

    EXPECT_GT(proj_e1, 0.f) << "EMA should retain component of e1";
    EXPECT_GT(proj_e2, 0.f) << "EMA should have component of e2";
}

// get_max_feature_similarity()

/**
 * @brief Querying with the same (unit-normalised) feature that was stored
 *        returns ~1.0.
 */
TEST(Small_TrackletAppearanceTest, SimilarityIdenticalFeatureIsOne)
{
    auto t = make_tracklet();
    t->update_features(unit_basis(4, 0));

    std::vector<float> query = unit_basis(4, 0);
    EXPECT_NEAR(t->get_max_feature_similarity(query), 1.f, 1e-4f);
}

/**
 * @brief Querying with an orthogonal feature returns ~0.
 */
TEST(Small_TrackletAppearanceTest, SimilarityOrthogonalFeatureIsZero)
{
    auto t = make_tracklet();
    t->update_features(unit_basis(4, 0)); // e0

    std::vector<float> ortho = unit_basis(4, 1); // e1 — orthogonal
    float sim = t->get_max_feature_similarity(ortho);
    EXPECT_LT(sim, 0.05f);
}

/**
 * @brief get_max_feature_similarity() scans the history, not only smooth_feat.
 *
 * After several updates that blend the original feature away, querying the
 * exact original vector should still return a high similarity via history.
 */
TEST(Small_TrackletAppearanceTest, SimilarityChecksHistory)
{
    auto t = make_tracklet(50, 0.9f);
    std::vector<float> original = unit_basis(4, 0);
    t->update_features(original);

    // Overwrite smooth_feat with a different direction via many EMA steps
    for (int i = 0; i < 30; ++i)
        t->update_features(unit_basis(4, 1));

    // The original vector is still in history; similarity should be non-trivial.
    float sim = t->get_max_feature_similarity(original);
    EXPECT_GT(sim, 0.5f) << "history should preserve the original feature";
}

// Feature history capacity

/**
 * @brief Feature history never exceeds max_feature_history entries.
 */
TEST(Small_TrackletAppearanceTest, HistoryDoesNotExceedCapacity)
{
    const int cap = 8;
    auto t = make_tracklet(cap);

    for (int i = 0; i < cap + 5; ++i)
    {
        std::vector<float> f = {static_cast<float>(i), 0.f};
        t->update_features(f);
    }

    EXPECT_LE(static_cast<int>(t->get_feature_history().size()), cap);
}

// Empty / zero feature vector

/**
 * @brief Calling update_features() with an empty vector is a no-op:
 *        has_feature() remains false if called before any real update.
 */
TEST(Small_TrackletAppearanceTest, EmptyFeatureVectorIsNoOp)
{
    auto t = make_tracklet();
    ASSERT_FALSE(t->has_feature());

    // Empty vector should not set a feature (The implementation moves the empty vector in; if it's empty it skips the update) We test by checking the Tracklet's own guard: feat.empty() check
    std::vector<float> empty;
    // update_features with empty: has_feature should stay false (Implementation: if feat.empty() guard is in place via !feat.empty() in constructor path, but update_features itself does not guard — the feature IS empty after move. The has_feature() check is smooth_feat_.empty(). So if update_features assigns an empty vector it normalises nothing and smooth_feat_ stays empty.)
    t->update_features(empty);
    // After empty update, smooth_feat_ is still empty
    EXPECT_FALSE(t->has_feature());
}

/**
 * @brief Features of changing lengths log one WARN naming both lengths, however many times the
 *        length changes.
 */
TEST(Small_TrackletAppearanceFeaturesTest, ChangingFeatureLengthWarnsOnce)
{
    CapturedLog log;
    auto track = SushiTrack::make_feat_t(10.f, 10.f, 4, 0);

    track->update_features(std::vector<float>(8, 1.0f));
    track->update_features(std::vector<float>(4, 1.0f));
    track->update_features(std::vector<float>(8, 1.0f));

    EXPECT_EQ(log.count("Feature length 8 does not match the track's 4"), 1);
    EXPECT_EQ(log.level_of("does not match"), SushiLog::LogLevel::Warn);
}
