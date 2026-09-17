/**************************************************************************/
/* TestReID.cpp                                                           */
/**************************************************************************/
/*                          This file is part of:                         */
/*                               SushiTrack                               */
/*              https://github.com/SushiSystems/SushiTrack                */
/*                          https://sushisystems.io                       */
/**************************************************************************/
/* Copyright (c) 2026-present Mustafa Garip & Sushi Systems               */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License");        */
/* you may not use this file except in compliance with the License.       */
/* You may obtain a copy of the License at                                */
/*                                                                        */
/*     http://www.apache.org/licenses/LICENSE-2.0                         */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                          */
/**************************************************************************/

/**
 * @file TestReID.cpp
 * @brief Unit tests for appearance feature ReID association.
 * @author Mustafa Garip
 */

#include <gtest/gtest.h>
#include <SushiTrack/tracklet.hpp>
#include <SushiTrack/kalman_filter.hpp>

#include <memory>
#include <vector>
#include <cmath>
#include <numeric>

using namespace SushiTrack;

// Helpers

/** @brief Creates a Tracklet with the given max_feature_history limit. */
static std::unique_ptr<Tracklet> makeTracklet(int max_hist = 100, float alpha = 0.9f)
{
    return std::make_unique<Tracklet>(
        std::make_unique<KalmanFilter>(),
        Rect<float>(0.f, 0.f, 50.f, 80.f),
        /*label=*/1, /*score=*/0.9f,
        /*feat=*/std::vector<float>{},
        alpha,
        /*enable_xyah=*/false,
        max_hist
    );
}

/** @brief Returns a unit vector [1, 0, 0, ...] of size n. */
static std::vector<float> unitBasis(int n, int axis = 0)
{
    std::vector<float> v(n, 0.f);
    v[axis] = 1.f;
    return v;
}

/** @brief Computes the L2 norm of a vector. */
static float l2Norm(const std::vector<float>& v)
{
    float s = 0.f;
    for (float x : v) s += x * x;
    return std::sqrt(s);
}

// hasFeature()

/**
 * @brief A freshly constructed Tracklet has no feature (hasFeature = false).
 */
TEST(ReIDTest, NoFeatureInitially)
{
    auto t = makeTracklet();
    EXPECT_FALSE(t->hasFeature());
}

/**
 * @brief After one updateFeatures() call, hasFeature() returns true.
 */
TEST(ReIDTest, HasFeatureAfterUpdate)
{
    auto t = makeTracklet();
    t->updateFeatures({1.f, 0.f, 0.f});
    EXPECT_TRUE(t->hasFeature());
}

// L2 normalisation

/**
 * @brief smooth_feat is unit-normalised after updateFeatures() for a
 *        non-unit input vector.
 */
TEST(ReIDTest, SmoothFeatIsUnitNormalised)
{
    auto t = makeTracklet();
    t->updateFeatures({3.f, 4.f, 0.f});   // L2 norm = 5

    const auto& sf = t->getSmoothFeat();
    EXPECT_NEAR(l2Norm(sf), 1.f, 1e-5f);
}

/**
 * @brief curr_feat is unit-normalised after updateFeatures().
 */
TEST(ReIDTest, CurrFeatIsUnitNormalised)
{
    auto t = makeTracklet();
    t->updateFeatures({0.f, 3.f, 4.f});

    const auto& cf = t->getCurrFeat();
    EXPECT_NEAR(l2Norm(cf), 1.f, 1e-5f);
}

// EMA blending

/**
 * @brief After two updateFeatures() calls, smooth_feat lies strictly between
 *        the two unit observations (EMA blends them).
 *
 * alpha = 0.9: smooth = 0.9 * e1 + 0.1 * e2 (normalised)
 * The smooth vector's projection onto e1 must be > 0 and onto e2 must be > 0.
 */
TEST(ReIDTest, EMABlendsConsecutiveFeatures)
{
    auto t = makeTracklet(100, 0.5f);

    std::vector<float> e1 = {1.f, 0.f, 0.f};
    std::vector<float> e2 = {0.f, 1.f, 0.f};

    t->updateFeatures(e1);
    t->updateFeatures(e2);

    const auto& sf = t->getSmoothFeat();
    ASSERT_EQ(sf.size(), static_cast<size_t>(3));

    float proj_e1 = sf[0];
    float proj_e2 = sf[1];

    EXPECT_GT(proj_e1, 0.f) << "EMA should retain component of e1";
    EXPECT_GT(proj_e2, 0.f) << "EMA should have component of e2";
}

// getMaxFeatureSimilarity()

/**
 * @brief Querying with the same (unit-normalised) feature that was stored
 *        returns ~1.0.
 */
TEST(ReIDTest, SimilarityIdenticalFeatureIsOne)
{
    auto t = makeTracklet();
    t->updateFeatures(unitBasis(4, 0));

    std::vector<float> query = unitBasis(4, 0);
    EXPECT_NEAR(t->getMaxFeatureSimilarity(query), 1.f, 1e-4f);
}

/**
 * @brief Querying with an orthogonal feature returns ~0.
 */
TEST(ReIDTest, SimilarityOrthogonalFeatureIsZero)
{
    auto t = makeTracklet();
    t->updateFeatures(unitBasis(4, 0));   // e0

    std::vector<float> ortho = unitBasis(4, 1);   // e1 — orthogonal
    float sim = t->getMaxFeatureSimilarity(ortho);
    EXPECT_LT(sim, 0.05f);
}

/**
 * @brief getMaxFeatureSimilarity() scans the history, not only smooth_feat.
 *
 * After several updates that blend the original feature away, querying the
 * exact original vector should still return a high similarity via history.
 */
TEST(ReIDTest, SimilarityChecksHistory)
{
    auto t = makeTracklet(50, 0.9f);
    std::vector<float> original = unitBasis(4, 0);
    t->updateFeatures(original);

    // Overwrite smooth_feat with a different direction via many EMA steps
    for (int i = 0; i < 30; ++i)
        t->updateFeatures(unitBasis(4, 1));

    // The original vector is still in history; similarity should be non-trivial.
    float sim = t->getMaxFeatureSimilarity(original);
    EXPECT_GT(sim, 0.5f) << "history should preserve the original feature";
}

// Feature history capacity

/**
 * @brief Feature history never exceeds max_feature_history entries.
 */
TEST(ReIDTest, HistoryDoesNotExceedCapacity)
{
    const int cap = 8;
    auto t = makeTracklet(cap);

    for (int i = 0; i < cap + 5; ++i)
    {
        std::vector<float> f = {static_cast<float>(i), 0.f};
        t->updateFeatures(f);
    }

    EXPECT_LE(static_cast<int>(t->getFeatureHistory().size()), cap);
}

// Empty / zero feature vector

/**
 * @brief Calling updateFeatures() with an empty vector is a no-op:
 *        hasFeature() remains false if called before any real update.
 */
TEST(ReIDTest, EmptyFeatureVectorIsNoOp)
{
    auto t = makeTracklet();
    ASSERT_FALSE(t->hasFeature());

    // Empty vector should not set a feature (The implementation moves the empty vector in; if it's empty it skips the update) We test by checking the Tracklet's own guard: feat.empty() check
    std::vector<float> empty;
    // updateFeatures with empty: hasFeature should stay false (Implementation: if feat.empty() guard is in place via !feat.empty() in constructor path, but updateFeatures itself does not guard — the feature IS empty after move. The hasFeature() check is smooth_feat_.empty(). So if updateFeatures assigns an empty vector it normalises nothing and smooth_feat_ stays empty.)
    t->updateFeatures(empty);
    // After empty update, smooth_feat_ is still empty
    EXPECT_FALSE(t->hasFeature());
}
