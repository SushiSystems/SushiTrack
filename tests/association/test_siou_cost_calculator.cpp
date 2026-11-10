/**************************************************************************/
/* test_siou_cost_calculator.cpp                                          */
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
 * @file test_siou_cost_calculator.cpp
 * @brief Tests SIoUCostCalculator.
 * @author Mustafa Garip
 */

#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/association/cost_matrix.hpp>
#include <SushiTrack/association/siou_cost_calculator.hpp>
#include <SushiTrack/tracking/tracklet.hpp>

#include <support/tolerance.hpp>
#include <tracklet_builders.hpp>

using namespace SushiTrack;

namespace
{
    /// @brief Rounded operations on the longest path from a box to its cost, counted from the
    ///        formula: corners, areas, intersection, union, quotients and the penalty terms.
    constexpr std::size_t K_ROUNDED_OPERATIONS = 48;
} // namespace

// SIoUCostCalculator

/** @brief Identical boxes — cost ≈ 0. */
TEST(Small_SIoUCostCalculatorTest, IdenticalBoxesCostIsZero)
{
    SIoUCostCalculator calc;
    auto a = make_t(50.f, 50.f, 80.f, 80.f);
    auto b = make_t(50.f, 50.f, 80.f, 80.f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), 0.f, 1e-4f);
}

/** @brief Separated boxes (no overlap) give a cost > 0. */
TEST(Small_SIoUCostCalculatorTest, SeparatedBoxesCostPositive)
{
    SIoUCostCalculator calc;
    auto a = make_t(0.f, 0.f, 50.f, 50.f);
    auto b = make_t(300.f, 0.f, 50.f, 50.f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_GT(m(0, 0), 0.f);
}

/**
 * @brief The batch cost of a partial overlap pair is the hand value: IoU is 9/41; the centres
 *        differ by 40 px on both axes, so the angle cost is 1 and gamma = 1; the normalised offset
 *        is 40 / 140 on each axis, so the distance cost is 2 (1 - exp(-4/49)); the widths and
 *        heights are equal, so the shape cost is 0; cost = 1 - (IoU - distance cost / 2).
 */
TEST(Small_SIoUCostCalculatorTest, BatchCostOfAPartialOverlapIsTheHandValue)
{
    SIoUCostCalculator calc;
    std::vector<std::shared_ptr<Tracklet>> av{make_t(0.f, 0.f, 100.f, 100.f)};
    std::vector<std::shared_ptr<Tracklet>> bv{make_t(40.f, 40.f, 100.f, 100.f)};
    CostMatrix m;

    calc.calculate(av, bv, m);

    EXPECT_TRUE(TestSupport::is_within(TestSupport::measure_relative_error<float>(
        m(0, 0), 2.0 - 9.0 / 41.0 - std::exp(-4.0 / 49.0), K_ROUNDED_OPERATIONS)));
}

/**
 * @brief The single cost of the same pair is the same hand value.
 */
TEST(Small_SIoUCostCalculatorTest, SingleCostOfAPartialOverlapIsTheHandValue)
{
    SIoUCostCalculator calc;
    const std::shared_ptr<Tracklet> a = make_t(0.f, 0.f, 100.f, 100.f);
    const std::shared_ptr<Tracklet> b = make_t(40.f, 40.f, 100.f, 100.f);

    EXPECT_TRUE(TestSupport::is_within(TestSupport::measure_relative_error<float>(
        calc.calculate_single(a, b), 2.0 - 9.0 / 41.0 - std::exp(-4.0 / 49.0),
        K_ROUNDED_OPERATIONS)));
}
