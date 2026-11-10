/**************************************************************************/
/* test_gated_iou_cost_calculator.cpp                                     */
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
 * @file test_gated_iou_cost_calculator.cpp
 * @brief Tests GatedIoUCostCalculator.
 * @author Mustafa Garip
 */

#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/association/cost_matrix.hpp>
#include <SushiTrack/association/gated_iou_cost_calculator.hpp>
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
    /// @brief Rounded operations from a box to its IoU cost: corners, areas, intersection, union
    ///        and the quotient.
    constexpr std::size_t K_ROUNDED_OPERATIONS = 10;
} // namespace

// GatedIoUCostCalculator

/**
 * @brief NullGateValidator passes every cell; identical boxes yield IoU cost ≈ 0.
 */
TEST(Small_GatedIoUCostCalculatorTest, NullGatePassesAllCellsFillsIoUCost)
{
    auto iou = std::make_shared<IoUCostCalculator>();
    GatedIoUCostCalculator calc(9.4877f, NullGateValidator::get_instance(), iou);

    auto a = make_t(100.f, 100.f, 50.f, 80.f);
    auto b = make_t(100.f, 100.f, 50.f, 80.f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), 0.f, 1e-4f);
}

/**
 * @brief NullGateValidator passes every cell; non-overlapping boxes yield cost = 1.
 */
TEST(Small_GatedIoUCostCalculatorTest, NullGateNonOverlappingProducesCostOne)
{
    auto iou = std::make_shared<IoUCostCalculator>();
    GatedIoUCostCalculator calc(9.4877f, NullGateValidator::get_instance(), iou);

    auto a = make_t(0.f, 0.f, 50.f, 50.f);
    auto b = make_t(200.f, 0.f, 50.f, 50.f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_FLOAT_EQ(m(0, 0), 1.f);
}

/**
 * @brief A detection placed 5000 px from the activated tracklet fails the
 *        Mahalanobis gate and produces a sentinel value.
 */
TEST(Small_GatedIoUCostCalculatorTest, FailedGateProducesSentinel)
{
    auto iou = std::make_shared<IoUCostCalculator>();
    GatedIoUCostCalculator calc(9.4877f, std::make_shared<KalmanFilter>(), iou);

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
TEST(Small_GatedIoUCostCalculatorTest, EmptyCollectionProducesEmptyMatrix)
{
    auto iou = std::make_shared<IoUCostCalculator>();
    GatedIoUCostCalculator calc(9.4877f, NullGateValidator::get_instance(), iou);

    auto a = make_t(0.f, 0.f, 50.f, 50.f);
    std::vector<std::shared_ptr<Tracklet>> av{a}, bv;
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_TRUE(m.empty());
}

/**
 * @brief The batch cost of the pair is the hand value 6/13: intersection 70 x 100 = 7000 over union
 *        20000 - 7000 = 13000 gives IoU 7/13, and the null gate passes the cell.
 */
TEST(Small_GatedIoUCostCalculatorTest, BatchCostOfAPartialOverlapIsTheHandValue)
{
    auto iou = std::make_shared<IoUCostCalculator>();
    GatedIoUCostCalculator calc(9.4877f, NullGateValidator::get_instance(), iou);
    std::vector<std::shared_ptr<Tracklet>> av{make_t(0.f, 0.f, 100.f, 100.f)};
    std::vector<std::shared_ptr<Tracklet>> bv{make_t(30.f, 0.f, 100.f, 100.f)};
    CostMatrix m;

    calc.calculate(av, bv, m);

    EXPECT_TRUE(TestSupport::is_within(
        TestSupport::measure_relative_error<float>(m(0, 0), 6.0 / 13.0, K_ROUNDED_OPERATIONS)));
}

/**
 * @brief The single cost of the same pair is the same value.
 */
TEST(Small_GatedIoUCostCalculatorTest, SingleCostOfAPartialOverlapIsTheHandValue)
{
    auto iou = std::make_shared<IoUCostCalculator>();
    GatedIoUCostCalculator calc(9.4877f, NullGateValidator::get_instance(), iou);
    const std::shared_ptr<Tracklet> a = make_t(0.f, 0.f, 100.f, 100.f);
    const std::shared_ptr<Tracklet> b = make_t(30.f, 0.f, 100.f, 100.f);

    EXPECT_TRUE(TestSupport::is_within(TestSupport::measure_relative_error<float>(
        calc.calculate_single(a, b), 6.0 / 13.0, K_ROUNDED_OPERATIONS)));
}

/** @brief Counts the gating log lines of one call. */
class Small_GatedIoUCostCalculatorLogTest : public GatingLogFixture
{
};

/**
 * @brief GatedIoUCostCalculator reports every gated pair of one call in one line.
 */
TEST_F(Small_GatedIoUCostCalculatorLogTest, LogsEveryGatedPairOfACallInOneLine)
{
    auto iou = std::make_shared<IoUCostCalculator>();
    GatedIoUCostCalculator calc(9.4877f, std::make_shared<KalmanFilter>(), iou);

    std::vector<std::shared_ptr<Tracklet>> av{live(make_t(100.f, 100.f, 50.f, 80.f))};
    std::vector<std::shared_ptr<Tracklet>> bv;
    for (int k = 0; k < 3; ++k)
        bv.push_back(make_t(5100.f + 200.f * k, 100.f, 50.f, 80.f));

    CostMatrix m;
    calc.calculate(av, bv, m);

    expect_one_line_for_three_gated_pairs(m);
}
