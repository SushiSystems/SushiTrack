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

#include <tracklet_builders.hpp>

using namespace SushiTrack;

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
 * @brief Neither tracklet has a feature vector; the blended path runs with
 *        reid_dist = 1 and IoU cost, producing a cost in [0, 1].
 */
TEST(Small_ReIDFirstCostCalculatorTest, BlendedPathWhenNoFeatures)
{
    auto iou = std::make_shared<IoUCostCalculator>();
    auto cosine = std::make_shared<CosineCostCalculator>();
    ReIDFirstCostCalculator calc(0.3f, 0.3f, 0.2f, iou, cosine);

    auto a = make_t(100.f, 100.f, 50.f, 80.f);
    auto b = make_t(100.f, 100.f, 50.f, 80.f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_GE(m(0, 0), 0.f);
    EXPECT_LE(m(0, 0), 1.f);
}

/**
 * @brief Tracklet A has a feature but B does not; the blended path applies
 *        with reid_dist = 1 (missing feature penalty), giving cost ≥ 0.
 */
TEST(Small_ReIDFirstCostCalculatorTest, MissingFeatureOnBProducesFallback)
{
    auto iou = std::make_shared<IoUCostCalculator>();
    auto cosine = std::make_shared<CosineCostCalculator>();
    ReIDFirstCostCalculator calc(0.3f, 0.3f, 0.2f, iou, cosine);

    auto a = make_feat_t(100.f, 100.f, 4, 0);
    auto b = make_t(100.f, 100.f, 50.f, 80.f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_GE(m(0, 0), 0.f);
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
 * @brief calculate_single returns the same value as the corresponding batch cell.
 */
TEST(Small_ReIDFirstCostCalculatorTest, SingleMatchesBatch)
{
    auto iou = std::make_shared<IoUCostCalculator>();
    auto cosine = std::make_shared<CosineCostCalculator>();
    ReIDFirstCostCalculator calc(0.3f, 0.3f, 0.2f, iou, cosine);

    auto a = make_feat_t(0.f, 0.f, 4, 2);
    auto b = make_feat_t(30.f, 0.f, 4, 2);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), calc.calculate_single(a, b), 1e-5f);
}
