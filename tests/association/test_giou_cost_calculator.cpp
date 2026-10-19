/**************************************************************************/
/* test_giou_cost_calculator.cpp                                          */
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
 * @file test_giou_cost_calculator.cpp
 * @brief Tests GIoUCostCalculator.
 * @author Mustafa Garip
 */

#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/association/cost_matrix.hpp>
#include <SushiTrack/association/giou_cost_calculator.hpp>
#include <SushiTrack/tracking/tracklet.hpp>

#include <tracklet_builders.hpp>

using namespace SushiTrack;

// GIoUCostCalculator

/** @brief Identical boxes — cost ≈ 0. */
TEST(Small_GIoUCostCalculatorTest, IdenticalBoxesCostIsZero)
{
    GIoUCostCalculator calc;
    auto a = make_t(50.f, 50.f, 80.f, 80.f);
    auto b = make_t(50.f, 50.f, 80.f, 80.f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), 0.f, 1e-4f);
}

/** @brief Separated boxes (no overlap) give a cost > 0. */
TEST(Small_GIoUCostCalculatorTest, SeparatedBoxesCostPositive)
{
    GIoUCostCalculator calc;
    auto a = make_t(0.f, 0.f, 50.f, 50.f);
    auto b = make_t(300.f, 0.f, 50.f, 50.f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_GT(m(0, 0), 0.f);
}

/** @brief calculate_single agrees with batch for a partial overlap pair. */
TEST(Small_GIoUCostCalculatorTest, SingleMatchesBatch)
{
    GIoUCostCalculator calc;
    auto a = make_t(0.f, 0.f, 100.f, 100.f);
    auto b = make_t(40.f, 40.f, 100.f, 100.f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), calc.calculate_single(a, b), 1e-5f);
}
