/**************************************************************************/
/* test_fuse_score_decorator.cpp                                          */
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
 * @file test_fuse_score_decorator.cpp
 * @brief Tests FuseScoreDecorator.
 * @author Mustafa Garip
 */

#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/association/cost_matrix.hpp>
#include <SushiTrack/association/fuse_score_decorator.hpp>
#include <SushiTrack/association/iou_cost_calculator.hpp>
#include <SushiTrack/tracking/tracklet.hpp>

#include <tracklet_builders.hpp>

using namespace SushiTrack;

// FuseScoreDecorator

/** @brief A detection score < 1 lowers the effective cost below the raw IoU cost. */
TEST(Small_FuseScoreDecoratorTest, HighScoreLowersCost)
{
    // Inner calculator: IoU on identical boxes → cost = 0. Decorated cost = 1 - (1 - 0) * 0.8 = 0.2.
    auto inner = std::make_shared<IoUCostCalculator>();
    FuseScoreDecorator dec(inner);

    auto a = make_t(0.f, 0.f, 50.f, 80.f, /*score=*/0.9f);
    auto b = make_t(0.f, 0.f, 50.f, 80.f, /*score=*/0.8f); // score on b (detection)

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    dec.calculate(av, bv, m);

    // Raw IoU cost = 0.  Fused = 1 - (1-0)*0.8 = 0.2.
    EXPECT_NEAR(m(0, 0), 0.2f, 1e-4f);
}

/** @brief Score = 1.0 leaves the cost unchanged. */
TEST(Small_FuseScoreDecoratorTest, ScoreOnePreservesCost)
{
    auto inner = std::make_shared<IoUCostCalculator>();
    FuseScoreDecorator dec(inner);

    // Partially overlapping boxes: cost will be some value c.
    auto a = make_t(0.f, 0.f, 100.f, 100.f, 0.9f);
    auto b = make_t(50.f, 0.f, 100.f, 100.f, 1.0f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix raw_m, fused_m;
    inner->calculate(av, bv, raw_m);
    dec.calculate(av, bv, fused_m);

    // fused = 1 - (1 - c) * 1.0 = c  → same as raw.
    EXPECT_NEAR(fused_m(0, 0), raw_m(0, 0), 1e-5f);
}

/** @brief Sentinel cells (≥ 1e3) produced by the inner calculator are preserved. */
TEST(Small_FuseScoreDecoratorTest, SentinelCellsPreserved)
{
    auto inner = std::make_shared<IoUCostCalculator>(4.0f);
    FuseScoreDecorator dec(inner);

    // Area ratio >> 4 → sentinel in the inner matrix.
    auto big = make_t(0.f, 0.f, 300.f, 300.f, 0.9f);
    auto small_ = make_t(0.f, 0.f, 10.f, 10.f, 0.5f);

    std::vector<std::shared_ptr<Tracklet>> av{big}, bv{small_};
    CostMatrix m;
    dec.calculate(av, bv, m);

    EXPECT_GE(m(0, 0), 1e3f);
}

/** @brief calculate_single applies the same formula as the batch path. */
TEST(Small_FuseScoreDecoratorTest, SingleMatchesBatch)
{
    auto inner = std::make_shared<IoUCostCalculator>();
    FuseScoreDecorator dec(inner);

    auto a = make_t(0.f, 0.f, 100.f, 100.f, 0.9f);
    auto b = make_t(30.f, 0.f, 100.f, 100.f, 0.7f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    dec.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), dec.calculate_single(a, b), 1e-5f);
}

/**
 * @brief FuseScoreDecorator with detection score = 0 always produces cost = 1:
 *        fused = 1 - (1 - inner_cost) * 0 = 1, regardless of the inner cost.
 */
TEST(Small_FuseScoreDecoratorTest, ScoreZeroProducesCostOne)
{
    auto inner = std::make_shared<IoUCostCalculator>();
    FuseScoreDecorator dec(inner);

    auto a = make_t(0.f, 0.f, 50.f, 80.f, 0.9f);
    auto b = make_t(0.f, 0.f, 50.f, 80.f, 0.0f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    dec.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), 1.f, 1e-5f);
}
