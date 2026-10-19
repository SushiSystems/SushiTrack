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
#include <tracklet_builders.hpp>

using namespace SushiTrack;

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
 * @brief calculate_single returns the same value as the corresponding batch cell.
 */
TEST(Small_GatedIoUCostCalculatorTest, SingleMatchesBatch)
{
    auto iou = std::make_shared<IoUCostCalculator>();
    GatedIoUCostCalculator calc(9.4877f, NullGateValidator::get_instance(), iou);

    auto a = make_t(0.f, 0.f, 100.f, 100.f);
    auto b = make_t(30.f, 0.f, 100.f, 100.f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), calc.calculate_single(a, b), 1e-5f);
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
