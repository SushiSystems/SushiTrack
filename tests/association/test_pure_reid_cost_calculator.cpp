/**************************************************************************/
/* test_pure_reid_cost_calculator.cpp                                     */
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
 * @file test_pure_reid_cost_calculator.cpp
 * @brief Tests PureReIDCostCalculator.
 * @author Mustafa Garip
 */

#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/association/cost_matrix.hpp>
#include <SushiTrack/association/pure_reid_cost_calculator.hpp>
#include <SushiTrack/filtering/kalman_filter.hpp>
#include <SushiTrack/filtering/null_gate_validator.hpp>
#include <SushiTrack/tracking/tracklet.hpp>

#include <gating_log_fixture.hpp>
#include <tracklet_builders.hpp>

using namespace SushiTrack;

// PureReIDCostCalculator

/**
 * @brief NullGate + identical features → reid_dist = 0 < thresh = 0.4 → cost near 0.
 */
TEST(Small_PureReIDCostCalculatorTest, NullGateIdenticalFeaturesPassesThreshold)
{
    PureReIDCostCalculator calc(0.4f, 9.4877f, NullGateValidator::get_instance());

    auto a = make_feat_t(100.f, 100.f, 4, 0);
    auto b = make_feat_t(100.f, 100.f, 4, 0);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), 0.f, 1e-4f);
}

/**
 * @brief NullGate + orthogonal features → reid_dist = 1 ≥ thresh = 0.4 → sentinel.
 */
TEST(Small_PureReIDCostCalculatorTest, NullGateOrthogonalFeaturesExceedsThreshold)
{
    PureReIDCostCalculator calc(0.4f, 9.4877f, NullGateValidator::get_instance());

    auto a = make_feat_t(100.f, 100.f, 4, 0);
    auto b = make_feat_t(100.f, 100.f, 4, 1);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_GE(m(0, 0), 1e3f);
}

/**
 * @brief A detection 5000 px from an activated tracklet fails the Mahalanobis
 *        gate and receives a sentinel value regardless of feature similarity.
 */
TEST(Small_PureReIDCostCalculatorTest, FailedGateProducesSentinel)
{
    PureReIDCostCalculator calc(0.4f, 9.4877f, std::make_shared<KalmanFilter>());

    auto a = make_feat_t(100.f, 100.f, 4, 0);
    a->activate(std::make_unique<KalmanFilter>(), 1, 1);
    a->predict();

    auto b = make_feat_t(5100.f, 100.f, 4, 0);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_GE(m(0, 0), 1e3f);
}

/**
 * @brief An empty detection collection produces an empty cost matrix.
 */
TEST(Small_PureReIDCostCalculatorTest, EmptyCollectionProducesEmptyMatrix)
{
    PureReIDCostCalculator calc(0.4f, 9.4877f, NullGateValidator::get_instance());

    auto a = make_feat_t(0.f, 0.f, 4, 0);
    std::vector<std::shared_ptr<Tracklet>> av{a}, bv;
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_TRUE(m.empty());
}

/**
 * @brief calculate_single returns the same value as the corresponding batch cell
 *        for a pair that passes the threshold.
 */
TEST(Small_PureReIDCostCalculatorTest, SingleMatchesBatch)
{
    PureReIDCostCalculator calc(0.4f, 9.4877f, NullGateValidator::get_instance());

    auto a = make_feat_t(0.f, 0.f, 4, 3);
    auto b = make_feat_t(30.f, 0.f, 4, 3);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), calc.calculate_single(a, b), 1e-5f);
}

/** @brief Counts the gating log lines of one call. */
class Small_PureReIDCostCalculatorLogTest : public GatingLogFixture
{
};

/**
 * @brief PureReIDCostCalculator reports every gated pair of one call in one line.
 */
TEST_F(Small_PureReIDCostCalculatorLogTest, LogsEveryGatedPairOfACallInOneLine)
{
    PureReIDCostCalculator calc(0.4f, 9.4877f, std::make_shared<KalmanFilter>());

    std::vector<std::shared_ptr<Tracklet>> av{live(make_feat_t(100.f, 100.f, 4, 0))};
    std::vector<std::shared_ptr<Tracklet>> bv;
    for (int k = 0; k < 3; ++k)
        bv.push_back(make_feat_t(5100.f + 200.f * k, 100.f, 4, 0));

    CostMatrix m;
    calc.calculate(av, bv, m);

    expect_one_line_for_three_gated_pairs(m);
}
