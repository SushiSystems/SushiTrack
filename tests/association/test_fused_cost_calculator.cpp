/**************************************************************************/
/* test_fused_cost_calculator.cpp                                         */
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
 * @file test_fused_cost_calculator.cpp
 * @brief Tests FusedCostCalculator.
 * @author Mustafa Garip
 */

#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/association/cosine_cost_calculator.hpp>
#include <SushiTrack/association/cost_matrix.hpp>
#include <SushiTrack/association/fused_cost_calculator.hpp>
#include <SushiTrack/association/iou_cost_calculator.hpp>
#include <SushiTrack/filtering/kalman_filter.hpp>
#include <SushiTrack/filtering/null_gate_validator.hpp>
#include <SushiTrack/tracking/tracklet.hpp>

#include <gating_log_fixture.hpp>
#include <support/tolerance.hpp>
#include <tracklet_builders.hpp>

using namespace SushiTrack;

namespace
{
    /// @brief Bound of 1 - cos(a, a) on a unit vector times the direct weight: four rounded operations of unit roundoff on a magnitude of 1.
    const double K_COSINE_BOUND = 4.0 * TestSupport::unit_roundoff<float>();
} // namespace

// FusedCostCalculator

/**
 * @brief NullGate + identical features → cosine_dist = 0 < reid_thresh → direct
 *        assignment path → cost near 0.
 */
TEST(Small_FusedCostCalculatorTest, NullGateIdenticalFeaturesDirectAssignment)
{
    auto iou = std::make_shared<IoUCostCalculator>();
    auto cosine = std::make_shared<CosineCostCalculator>();
    FusedCostCalculator calc(0.5f, 0.3f, 9.4877f, NullGateValidator::get_instance(), iou, cosine,
                             /*reid_direct_assignment_weight=*/0.5f);

    auto a = make_feat_t(100.f, 100.f, 4, 0);
    auto b = make_feat_t(100.f, 100.f, 4, 0);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), 0.f, 1e-4f);
}

/**
 * @brief A detection 5000 px from an activated tracklet fails the real KF gate
 *        and receives a sentinel value.
 */
TEST(Small_FusedCostCalculatorTest, GatedPairKeepsTheSentinelWhenTheKalmanGateFails)
{
    auto iou = std::make_shared<IoUCostCalculator>();
    auto cosine = std::make_shared<CosineCostCalculator>();
    FusedCostCalculator calc(0.5f, 0.3f, 9.4877f, std::make_shared<KalmanFilter>(), iou, cosine);

    auto a = make_t(100.f, 100.f, 50.f, 80.f);
    a->activate(std::make_unique<KalmanFilter>(), 1, 1);
    a->predict();

    auto b = make_t(5100.f, 100.f, 50.f, 80.f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_GE(m(0, 0), 1e3f);
}

/**
 * @brief An empty detection collection produces an empty cost matrix.
 */
TEST(Small_FusedCostCalculatorTest, EmptyCollectionProducesEmptyMatrix)
{
    auto iou = std::make_shared<IoUCostCalculator>();
    auto cosine = std::make_shared<CosineCostCalculator>();
    FusedCostCalculator calc(0.5f, 0.3f, 9.4877f, NullGateValidator::get_instance(), iou, cosine);

    auto a = make_feat_t(0.f, 0.f, 4, 0);
    std::vector<std::shared_ptr<Tracklet>> av{a}, bv;
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_TRUE(m.empty());
}

/**
 * @brief The batch cost of the pair is 0: identical features have a distance of 0, under the threshold of 0.3, so the direct assignment path returns 0 times its weight of 0.5, whatever the 30 px offset.
 */
TEST(Small_FusedCostCalculatorTest, BatchDirectAssignmentCostIgnoresTheBoxOffset)
{
    auto iou = std::make_shared<IoUCostCalculator>();
    auto cosine = std::make_shared<CosineCostCalculator>();
    FusedCostCalculator calc(0.5f, 0.3f, 9.4877f, NullGateValidator::get_instance(), iou, cosine,
                             /*reid_direct_assignment_weight=*/0.5f);
    std::vector<std::shared_ptr<Tracklet>> av{make_feat_t(0.f, 0.f, 4, 1)};
    std::vector<std::shared_ptr<Tracklet>> bv{make_feat_t(30.f, 0.f, 4, 1)};
    CostMatrix m;

    calc.calculate(av, bv, m);

    EXPECT_TRUE(TestSupport::is_within(
        TestSupport::measure_error(m(0, 0), 0.0, K_COSINE_BOUND)));
}

/**
 * @brief The single cost of the same pair is the same value.
 */
TEST(Small_FusedCostCalculatorTest, SingleDirectAssignmentCostIgnoresTheBoxOffset)
{
    auto iou = std::make_shared<IoUCostCalculator>();
    auto cosine = std::make_shared<CosineCostCalculator>();
    FusedCostCalculator calc(0.5f, 0.3f, 9.4877f, NullGateValidator::get_instance(), iou, cosine,
                             /*reid_direct_assignment_weight=*/0.5f);
    const std::shared_ptr<Tracklet> a = make_feat_t(0.f, 0.f, 4, 1);
    const std::shared_ptr<Tracklet> b = make_feat_t(30.f, 0.f, 4, 1);

    EXPECT_TRUE(TestSupport::is_within(
        TestSupport::measure_error(calc.calculate_single(a, b), 0.0, K_COSINE_BOUND)));
}

/** @brief Counts the gating log lines of one call. */
class Small_FusedCostCalculatorLogTest : public GatingLogFixture
{
};

/**
 * @brief FusedCostCalculator reports every gated pair of one call in one line.
 */
TEST_F(Small_FusedCostCalculatorLogTest, LogsEveryGatedPairOfACallInOneLine)
{
    auto iou = std::make_shared<IoUCostCalculator>();
    auto cosine = std::make_shared<CosineCostCalculator>();
    FusedCostCalculator calc(0.5f, 0.3f, 9.4877f, std::make_shared<KalmanFilter>(), iou, cosine);

    std::vector<std::shared_ptr<Tracklet>> av{live(make_t(100.f, 100.f, 50.f, 80.f))};
    std::vector<std::shared_ptr<Tracklet>> bv;
    for (int k = 0; k < 3; ++k)
        bv.push_back(make_t(5100.f + 200.f * k, 100.f, 50.f, 80.f));

    CostMatrix m;
    calc.calculate(av, bv, m);

    expect_one_line_for_three_gated_pairs(m);
}
