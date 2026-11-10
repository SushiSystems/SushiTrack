/**************************************************************************/
/* test_reid_first_cost_calculator.cpp                                    */
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
 * @file test_reid_first_cost_calculator.cpp
 * @brief Tests ReIDFirstCostCalculator.
 * @author Mustafa Garip
 */

#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/association/cosine_cost_calculator.hpp>
#include <SushiTrack/association/cost_matrix.hpp>
#include <SushiTrack/association/iou_cost_calculator.hpp>
#include <SushiTrack/association/reid_first_cost_calculator.hpp>
#include <SushiTrack/tracking/tracklet.hpp>

#include <support/tolerance.hpp>
#include <tracklet_builders.hpp>

using namespace SushiTrack;

namespace
{
    /// @brief Rounded operations from a box to its IoU cost: corners, areas, intersection, union
    ///        and
    ///        the quotient.
    constexpr std::size_t K_IOU_OPERATIONS = 10;

    /// @brief Bound of 1 - cos(a, a) on a unit vector times the direct weight: four rounded
    ///        operations of unit roundoff on a magnitude of 1.
    const double K_COSINE_BOUND = 4.0 * TestSupport::unit_roundoff<float>();
} // namespace

// ReIDFirstCostCalculator

/**
 * @brief Both tracklets carry identical features; reid_dist = 0 < thresh → direct
 *        assignment path → cost = 0 * reid_direct_assignment_weight = 0.
 */
TEST(Small_ReIDFirstCostCalculatorTest, DirectAssignmentWhenBothHaveFeaturesAndCloseReID)
{
    auto iou = std::make_shared<IoUCostCalculator>();
    auto cosine = std::make_shared<CosineCostCalculator>();
    ReIDFirstCostCalculator calc(0.3f, 0.3f, 0.2f, iou, cosine);

    auto a = make_feat_t(100.f, 100.f, 4, 0);
    auto b = make_feat_t(100.f, 100.f, 4, 0);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), 0.f, 1e-4f);
}

/**
 * @brief Neither tracklet has a feature vector, so the cost is the geometry cost alone: two
 *        50 x 80 boxes 30 px apart overlap 20 x 80 = 1600 of a union of 8000 - 1600 = 6400, an
 *        IoU of 1/4 and a cost of 3/4.
 */
TEST(Small_ReIDFirstCostCalculatorTest, NoFeaturesFallBackToTheGeometryCost)
{
    auto iou = std::make_shared<IoUCostCalculator>();
    auto cosine = std::make_shared<CosineCostCalculator>();
    ReIDFirstCostCalculator calc(0.3f, 0.3f, 0.2f, iou, cosine);

    auto a = make_t(100.f, 100.f, 50.f, 80.f);
    auto b = make_t(130.f, 100.f, 50.f, 80.f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_TRUE(TestSupport::is_within(
        TestSupport::measure_relative_error<float>(m(0, 0), 0.75, K_IOU_OPERATIONS)));
}

/**
 * @brief Tracklet A has a feature but B does not, so the cost is the geometry cost alone: the
 *        same 3/4 as for two featureless boxes 30 px apart.
 */
TEST(Small_ReIDFirstCostCalculatorTest, MissingFeatureOnBFallsBackToTheGeometryCost)
{
    auto iou = std::make_shared<IoUCostCalculator>();
    auto cosine = std::make_shared<CosineCostCalculator>();
    ReIDFirstCostCalculator calc(0.3f, 0.3f, 0.2f, iou, cosine);

    auto a = make_feat_t(100.f, 100.f, 4, 0);
    auto b = make_t(130.f, 100.f, 50.f, 80.f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_TRUE(TestSupport::is_within(
        TestSupport::measure_relative_error<float>(m(0, 0), 0.75, K_IOU_OPERATIONS)));
}

/**
 * @brief An empty detection collection produces an empty cost matrix.
 */
TEST(Small_ReIDFirstCostCalculatorTest, EmptyCollectionProducesEmptyMatrix)
{
    auto iou = std::make_shared<IoUCostCalculator>();
    auto cosine = std::make_shared<CosineCostCalculator>();
    ReIDFirstCostCalculator calc(0.3f, 0.3f, 0.2f, iou, cosine);

    auto a = make_feat_t(0.f, 0.f, 4, 0);
    std::vector<std::shared_ptr<Tracklet>> av{a}, bv;
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_TRUE(m.empty());
}

/**
 * @brief The batch cost of the pair is 0: identical features have a distance of 0, under the
 *        confidence threshold of 0.3, and 0 times the direct assignment weight of 0.2 is 0.
 */
TEST(Small_ReIDFirstCostCalculatorTest, BatchCostOfIdenticalFeaturesIsZero)
{
    auto iou = std::make_shared<IoUCostCalculator>();
    auto cosine = std::make_shared<CosineCostCalculator>();
    ReIDFirstCostCalculator calc(0.3f, 0.3f, 0.2f, iou, cosine);
    std::vector<std::shared_ptr<Tracklet>> av{make_feat_t(0.f, 0.f, 4, 2)};
    std::vector<std::shared_ptr<Tracklet>> bv{make_feat_t(30.f, 0.f, 4, 2)};
    CostMatrix m;

    calc.calculate(av, bv, m);

    EXPECT_TRUE(TestSupport::is_within(
        TestSupport::measure_error(m(0, 0), 0.0, K_COSINE_BOUND)));
}

/**
 * @brief The single cost of the same pair is the same value.
 */
TEST(Small_ReIDFirstCostCalculatorTest, SingleCostOfIdenticalFeaturesIsZero)
{
    auto iou = std::make_shared<IoUCostCalculator>();
    auto cosine = std::make_shared<CosineCostCalculator>();
    ReIDFirstCostCalculator calc(0.3f, 0.3f, 0.2f, iou, cosine);
    const std::shared_ptr<Tracklet> a = make_feat_t(0.f, 0.f, 4, 2);
    const std::shared_ptr<Tracklet> b = make_feat_t(30.f, 0.f, 4, 2);

    EXPECT_TRUE(TestSupport::is_within(
        TestSupport::measure_error(calc.calculate_single(a, b), 0.0, K_COSINE_BOUND)));
}
