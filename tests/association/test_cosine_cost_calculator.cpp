/**************************************************************************/
/* test_cosine_cost_calculator.cpp                                        */
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
 * @file test_cosine_cost_calculator.cpp
 * @brief Tests CosineCostCalculator.
 * @author Mustafa Garip
 */

#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/association/cosine_cost_calculator.hpp>
#include <SushiTrack/association/cost_matrix.hpp>
#include <SushiTrack/tracking/tracklet.hpp>

#include <tracklet_builders.hpp>

using namespace SushiTrack;

// CosineCostCalculator

/** @brief Same unit-feature tracklets → cosine similarity 1 → cost ≈ 0. */
TEST(Small_CosineCostCalculatorTest, IdenticalFeaturesCostIsZero)
{
    CosineCostCalculator calc;
    auto a = make_feat_t(0.f, 0.f, 3, 0);
    auto b = make_feat_t(0.f, 0.f, 3, 0);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), 0.f, 1e-4f);
}

/** @brief Orthogonal features → cosine similarity 0 → cost ≈ 1. */
TEST(Small_CosineCostCalculatorTest, OrthogonalFeaturesCostIsOne)
{
    CosineCostCalculator calc;
    auto a = make_feat_t(0.f, 0.f, 3, 0); // [1, 0, 0]
    auto b = make_feat_t(0.f, 0.f, 3, 1); // [0, 1, 0]

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), 1.f, 1e-4f);
}

/** @brief Missing feature on 'a' side → cost = 1.0. */
TEST(Small_CosineCostCalculatorTest, MissingFeatureOnAProducesCostOne)
{
    CosineCostCalculator calc;
    auto a = make_t(0.f, 0.f, 50.f, 80.f); // no feature
    auto b = make_feat_t(0.f, 0.f, 3, 0);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_FLOAT_EQ(m(0, 0), 1.f);
}

/** @brief calculate_single agrees with batch for identical-feature pair. */
TEST(Small_CosineCostCalculatorTest, SingleMatchesBatch)
{
    CosineCostCalculator calc;
    auto a = make_feat_t(0.f, 0.f, 4, 2);
    auto b = make_feat_t(0.f, 0.f, 4, 2);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), calc.calculate_single(a, b), 1e-5f);
}

/**
 * @brief CosineCostCalculator returns cost = 1 when tracklet B carries no
 *        feature vector (missing feature on the detection side).
 */
TEST(Small_CosineCostCalculatorTest, MissingFeatureOnBProducesCostOne)
{
    CosineCostCalculator calc;
    auto a = make_feat_t(0.f, 0.f, 3, 0);
    auto b = make_t(0.f, 0.f, 50.f, 80.f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_FLOAT_EQ(m(0, 0), 1.f);
}
