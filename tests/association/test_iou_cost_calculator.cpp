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

#include <tracklet_builders.hpp>

using namespace SushiTrack;

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

    EXPECT_NEAR(m(0, 0), 0.f, 1e-4f);
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

/** @brief calculate_single agrees with the batch matrix for an overlapping pair. */
TEST(Small_IoUCostCalculatorTest, SingleMatchesBatch)
{
    IoUCostCalculator calc;
    auto a = make_t(0.f, 0.f, 100.f, 100.f);
    auto b = make_t(50.f, 0.f, 100.f, 100.f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    float single = calc.calculate_single(a, b);
    EXPECT_NEAR(m(0, 0), single, 1e-5f);
}
