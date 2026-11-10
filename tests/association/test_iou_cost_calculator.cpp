/**************************************************************************/
/* test_iou_cost_calculator.cpp                                           */
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
 * @file test_iou_cost_calculator.cpp
 * @brief Tests IoUCostCalculator.
 * @author Mustafa Garip
 */

#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/association/cost_matrix.hpp>
#include <SushiTrack/association/iou_cost_calculator.hpp>
#include <SushiTrack/tracking/tracklet.hpp>

#include <support/tolerance.hpp>
#include <tracklet_builders.hpp>

using namespace SushiTrack;

namespace
{
    /// @brief Rounded operations on the longest path from a box to its cost, counted from the
    ///        formula: corners, areas, intersection, union, quotients and the penalty terms.
    constexpr std::size_t K_ROUNDED_OPERATIONS = 10;

    /// @brief Bound of the cost of identical boxes: the cost is 1 minus a ratio of magnitude 1 that
    ///        K_ROUNDED_OPERATIONS rounded operations built, each adding at most the unit roundoff.
    constexpr double K_IDENTICAL_BOXES_BOUND =
        static_cast<double>(K_ROUNDED_OPERATIONS) * TestSupport::unit_roundoff<float>();
} // namespace

// IoUCostCalculator

/** @brief Identical boxes — IoU = 1.0 → cost ≈ 0. */
TEST(Small_IoUCostCalculatorTest, IdenticalBoxesCostIsZero)
{
    IoUCostCalculator calc;
    auto a = make_t(100.f, 100.f, 50.f, 80.f);
    auto b = make_t(100.f, 100.f, 50.f, 80.f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_TRUE(TestSupport::is_within(
        TestSupport::measure_error(m(0, 0), 0.0, K_IDENTICAL_BOXES_BOUND)));
}

/** @brief Non-overlapping boxes — IoU = 0 → cost = 1.0. */
TEST(Small_IoUCostCalculatorTest, NonOverlappingBoxesCostIsOne)
{
    IoUCostCalculator calc;
    auto a = make_t(0.f, 0.f, 50.f, 50.f);
    auto b = make_t(200.f, 0.f, 50.f, 50.f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_FLOAT_EQ(m(0, 0), 1.f);
}

/** @brief Partial overlap produces 0 < cost < 1. */
TEST(Small_IoUCostCalculatorTest, PartialOverlapCostInRange)
{
    IoUCostCalculator calc;
    // 50×50 boxes with 25-pixel horizontal shift → intersection = 25×50.
    auto a = make_t(0.f, 0.f, 50.f, 50.f);
    auto b = make_t(25.f, 0.f, 50.f, 50.f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_GT(m(0, 0), 0.f);
    EXPECT_LT(m(0, 0), 1.f);
}

/** @brief Area-ratio violation (max_area_ratio=4) → sentinel 1e4f. */
TEST(Small_IoUCostCalculatorTest, AreaRatioViolationIsSentinel)
{
    IoUCostCalculator calc(4.0f);
    // Area ratio = (200*200)/(10*10) = 400, far above 4.
    auto big = make_t(0.f, 0.f, 200.f, 200.f);
    auto small_ = make_t(0.f, 0.f, 10.f, 10.f);

    std::vector<std::shared_ptr<Tracklet>> av{big}, bv{small_};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_GE(m(0, 0), 1e3f);
}

/** @brief Empty column collection produces an empty matrix. */
TEST(Small_IoUCostCalculatorTest, EmptyBCollectionProducesEmptyMatrix)
{
    IoUCostCalculator calc;
    auto a = make_t(0.f, 0.f, 50.f, 50.f);
    std::vector<std::shared_ptr<Tracklet>> av{a}, bv;
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_TRUE(m.empty());
}

/**
 * @brief The batch cost of a partial overlap pair is the hand value: intersection 50 x 100 = 5000
 *        over union 20000 - 5000 = 15000 gives IoU 1/3 and cost 2/3.
 */
TEST(Small_IoUCostCalculatorTest, BatchCostOfAPartialOverlapIsTheHandValue)
{
    IoUCostCalculator calc;
    std::vector<std::shared_ptr<Tracklet>> av{make_t(0.f, 0.f, 100.f, 100.f)};
    std::vector<std::shared_ptr<Tracklet>> bv{make_t(50.f, 0.f, 100.f, 100.f)};
    CostMatrix m;

    calc.calculate(av, bv, m);

    EXPECT_TRUE(TestSupport::is_within(TestSupport::measure_relative_error<float>(
        m(0, 0), 2.0 / 3.0, K_ROUNDED_OPERATIONS)));
}

/**
 * @brief The single cost of the same pair is the same hand value.
 */
TEST(Small_IoUCostCalculatorTest, SingleCostOfAPartialOverlapIsTheHandValue)
{
    IoUCostCalculator calc;
    const std::shared_ptr<Tracklet> a = make_t(0.f, 0.f, 100.f, 100.f);
    const std::shared_ptr<Tracklet> b = make_t(50.f, 0.f, 100.f, 100.f);

    EXPECT_TRUE(TestSupport::is_within(TestSupport::measure_relative_error<float>(
        calc.calculate_single(a, b), 2.0 / 3.0, K_ROUNDED_OPERATIONS)));
}
