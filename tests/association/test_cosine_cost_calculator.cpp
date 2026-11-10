/**************************************************************************/
/* test_cosine_cost_calculator.cpp                                        */
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
 * @file test_cosine_cost_calculator.cpp
 * @brief Tests CosineCostCalculator.
 * @author Mustafa Garip
 */

#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/association/cosine_cost_calculator.hpp>
#include <SushiTrack/association/cost_matrix.hpp>
#include <SushiTrack/tracking/tracklet.hpp>

#include <support/tolerance.hpp>
#include <tracklet_builders.hpp>

using namespace SushiTrack;

namespace
{
    /// @brief Bound of 1 - dot(a, a) / (|a| |a|) on a unit vector: a dot product, two norms and a
    ///        quotient, four rounded operations of unit roundoff on a magnitude of 1.
    const double K_COSINE_BOUND = 4.0 * TestSupport::unit_roundoff<float>();
} // namespace

// CosineCostCalculator

/** @brief Same unit-feature tracklets → cosine similarity 1 → cost ≈ 0. */
TEST(Small_CosineCostCalculatorTest, IdenticalFeaturesCostIsZero)
{
    CosineCostCalculator calc;
    auto a = make_feat_t(0.f, 0.f, 3, 0);
    auto b = make_feat_t(0.f, 0.f, 3, 0);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), 0.f, 1e-4f);
}

/** @brief Orthogonal features → cosine similarity 0 → cost ≈ 1. */
TEST(Small_CosineCostCalculatorTest, OrthogonalFeaturesCostIsOne)
{
    CosineCostCalculator calc;
    auto a = make_feat_t(0.f, 0.f, 3, 0); // [1, 0, 0]
    auto b = make_feat_t(0.f, 0.f, 3, 1); // [0, 1, 0]

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), 1.f, 1e-4f);
}

/** @brief A tracklet with no feature on either side costs 1 against one that has a feature. */
TEST(Small_CosineCostCalculatorTest, MissingFeatureOnEitherSideProducesCostOne)
{
    CosineCostCalculator calc;
    const std::shared_ptr<Tracklet> featured = make_feat_t(0.f, 0.f, 3, 0);
    const std::shared_ptr<Tracklet> bare = make_t(0.f, 0.f, 50.f, 80.f);
    CostMatrix featureless_on_a;
    CostMatrix featureless_on_b;

    calc.calculate({bare}, {featured}, featureless_on_a);
    calc.calculate({featured}, {bare}, featureless_on_b);

    EXPECT_FLOAT_EQ(featureless_on_a(0, 0), 1.f);
    EXPECT_FLOAT_EQ(featureless_on_b(0, 0), 1.f);
}

/** @brief Identical features cost 0 through the single-pair path too, as through the batch. */
TEST(Small_CosineCostCalculatorTest, SingleCostOfIdenticalFeaturesIsZero)
{
    CosineCostCalculator calc;
    const std::shared_ptr<Tracklet> a = make_feat_t(0.f, 0.f, 4, 2);
    const std::shared_ptr<Tracklet> b = make_feat_t(0.f, 0.f, 4, 2);

    EXPECT_TRUE(TestSupport::is_within(
        TestSupport::measure_error(calc.calculate_single(a, b), 0.0, K_COSINE_BOUND)));
}
