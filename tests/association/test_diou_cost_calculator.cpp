/**************************************************************************/
/* test_diou_cost_calculator.cpp                                          */
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
 * @file test_diou_cost_calculator.cpp
 * @brief Tests DIoUCostCalculator.
 * @author Mustafa Garip
 */

#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/association/cost_matrix.hpp>
#include <SushiTrack/association/diou_cost_calculator.hpp>
#include <SushiTrack/association/iou_cost_calculator.hpp>
#include <SushiTrack/tracking/tracklet.hpp>

#include <support/tolerance.hpp>
#include <tracklet_builders.hpp>

using namespace SushiTrack;

namespace
{
    /// @brief Rounded operations on the longest path from a box to its cost, counted from the
    ///        formula: corners, areas, intersection, union, quotients and the penalty terms.
    constexpr std::size_t K_ROUNDED_OPERATIONS = 24;
} // namespace

// DIoUCostCalculator

/** @brief Identical boxes — cost ≈ 0. */
TEST(Small_DIoUCostCalculatorTest, IdenticalBoxesCostIsZero)
{
    DIoUCostCalculator calc;
    auto a = make_t(50.f, 50.f, 80.f, 80.f);
    auto b = make_t(50.f, 50.f, 80.f, 80.f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), 0.f, 1e-4f);
}

/** @brief Separated boxes (no overlap) give a cost > 0. */
TEST(Small_DIoUCostCalculatorTest, SeparatedBoxesCostPositive)
{
    DIoUCostCalculator calc;
    auto a = make_t(0.f, 0.f, 50.f, 50.f);
    auto b = make_t(300.f, 0.f, 50.f, 50.f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_GT(m(0, 0), 0.f);
}

/**
 * @brief The batch cost of a partial overlap pair is the hand value: intersection 60 x 60 = 3600 over union 20000 - 3600 = 16400 gives IoU 9/41; the centres are 40 px apart on both axes, so d2 = 3200, and the enclosing box is 140 x 140, so c2 = 39200 and d2 / c2 = 4/49; cost = 1 - IoU + d2 / c2.
 */
TEST(Small_DIoUCostCalculatorTest, BatchCostOfAPartialOverlapIsTheHandValue)
{
    DIoUCostCalculator calc;
    std::vector<std::shared_ptr<Tracklet>> av{make_t(0.f, 0.f, 100.f, 100.f)};
    std::vector<std::shared_ptr<Tracklet>> bv{make_t(40.f, 40.f, 100.f, 100.f)};
    CostMatrix m;

    calc.calculate(av, bv, m);

    EXPECT_TRUE(TestSupport::is_within(TestSupport::measure_relative_error<float>(
        m(0, 0), 1.0 - 9.0 / 41.0 + 4.0 / 49.0, K_ROUNDED_OPERATIONS)));
}

/**
 * @brief The single cost of the same pair is the same hand value.
 */
TEST(Small_DIoUCostCalculatorTest, SingleCostOfAPartialOverlapIsTheHandValue)
{
    DIoUCostCalculator calc;
    const std::shared_ptr<Tracklet> a = make_t(0.f, 0.f, 100.f, 100.f);
    const std::shared_ptr<Tracklet> b = make_t(40.f, 40.f, 100.f, 100.f);

    EXPECT_TRUE(TestSupport::is_within(TestSupport::measure_relative_error<float>(
        calc.calculate_single(a, b), 1.0 - 9.0 / 41.0 + 4.0 / 49.0, K_ROUNDED_OPERATIONS)));
}

/**
 * @brief DIoU cost for a misaligned contained box is strictly GREATER
 * than the corresponding plain IoU cost because the center-distance
 * penalty reduces the DIoU score, thus increasing the cost.
 */
TEST(Small_DIoUCostCalculatorTest, ContainedMisalignedBoxCostGreaterThanIoUCost)
{
    IoUCostCalculator iou_calc;
    DIoUCostCalculator diou_calc;

    auto outer = make_t(0.f, 0.f, 100.f, 100.f);
    auto inner = make_t(10.f, 10.f, 50.f, 50.f);

    std::vector<std::shared_ptr<Tracklet>> av{outer}, bv{inner};
    CostMatrix iou_m, diou_m;

    iou_calc.calculate(av, bv, iou_m);
    diou_calc.calculate(av, bv, diou_m);

    EXPECT_GT(diou_m(0, 0), iou_m(0, 0))
        << "DIoU cost should be higher than IoU cost due to center distance penalty";
}
