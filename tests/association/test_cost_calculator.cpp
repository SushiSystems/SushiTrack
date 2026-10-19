/**************************************************************************/
/* test_cost_calculator.cpp                                               */
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
 * @file test_cost_calculator.cpp
 * @brief Tests that every CostCalculator gives one row per track and one column per detection.
 * @author Mustafa Garip
 */

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/association/cosine_cost_calculator.hpp>
#include <SushiTrack/association/cost_calculator.hpp>
#include <SushiTrack/association/cost_matrix.hpp>
#include <SushiTrack/association/diou_cost_calculator.hpp>
#include <SushiTrack/association/fuse_score_decorator.hpp>
#include <SushiTrack/association/fused_cost_calculator.hpp>
#include <SushiTrack/association/gated_iou_cost_calculator.hpp>
#include <SushiTrack/association/giou_cost_calculator.hpp>
#include <SushiTrack/association/iou_cost_calculator.hpp>
#include <SushiTrack/association/observation_momentum_decorator.hpp>
#include <SushiTrack/association/pure_reid_cost_calculator.hpp>
#include <SushiTrack/association/reid_first_cost_calculator.hpp>
#include <SushiTrack/association/siou_cost_calculator.hpp>
#include <SushiTrack/filtering/null_gate_validator.hpp>
#include <SushiTrack/tracking/tracker.hpp>

#include <tracklet_builders.hpp>

using namespace SushiTrack;

namespace
{
    /** @brief Names one calculator and builds it. */
    struct CalculatorCase
    {
            std::string name;                                      /**< The calculator's name. */
            std::function<std::shared_ptr<CostCalculator>()> make; /**< Builds the calculator. */
    };

    /** @brief Returns every calculator, each with the collaborators it needs. */
    std::vector<CalculatorCase> calculator_cases()
    {
        const auto iou = []
        {
            return std::make_shared<IoUCostCalculator>();
        };
        const auto cosine = []
        {
            return std::make_shared<CosineCostCalculator>();
        };
        const auto gate = []
        {
            return NullGateValidator::get_instance();
        };
        return {
            {"IoU", iou},
            {"DIoU",
             []
             {
                 return std::make_shared<DIoUCostCalculator>();
             }},
            {"GIoU",
             []
             {
                 return std::make_shared<GIoUCostCalculator>();
             }},
            {"SIoU",
             []
             {
                 return std::make_shared<SIoUCostCalculator>();
             }},
            {"Cosine", cosine},
            {"ReIDFirst",
             [=]
             {
                 return std::make_shared<ReIDFirstCostCalculator>(0.3f, 0.3f, 0.2f, iou(),
                                                                  cosine());
             }},
            {"PureReID",
             [=]
             {
                 return std::make_shared<PureReIDCostCalculator>(0.4f, 9.4877f, gate());
             }},
            {"Fused",
             [=]
             {
                 return std::make_shared<FusedCostCalculator>(0.5f, 0.3f, 9.4877f, gate(), iou(),
                                                              cosine());
             }},
            {"FuseScore",
             [=]
             {
                 return std::make_shared<FuseScoreDecorator>(iou());
             }},
            {"ObservationMomentum",
             [=]
             {
                 return std::make_shared<ObservationMomentumDecorator>(iou(), 0.1f, 1.0f, 1);
             }},
            {"GatedIoU",
             [=]
             {
                 return std::make_shared<GatedIoUCostCalculator>(9.4877f, gate(), iou());
             }},
        };
    }

    /** @brief Runs each calculator with its name as the test parameter's label. */
    class Small_CostCalculatorTest : public ::testing::TestWithParam<CalculatorCase>
    {
    };
} // namespace

/**
 * @brief Through CostCalculator&, two tracks against three detections give a 2 x 3 matrix.
 */
TEST_P(Small_CostCalculatorTest, GivesOneRowPerTrackAndOneColumnPerDetection)
{
    std::shared_ptr<CostCalculator> calculator = GetParam().make();
    const std::vector<CostCalculator::TrackletPtr> tracks{make_feat_t(100.f, 100.f, 4, 0),
                                                          make_feat_t(300.f, 100.f, 4, 1)};
    const std::vector<CostCalculator::TrackletPtr> detections{make_feat_t(100.f, 100.f, 4, 0),
                                                              make_feat_t(300.f, 100.f, 4, 1),
                                                              make_feat_t(500.f, 100.f, 4, 2)};

    CostMatrix matrix;
    calculator->calculate(tracks, detections, matrix);

    EXPECT_EQ(matrix.rows(), 2u);
    EXPECT_EQ(matrix.cols(), 3u);
}

INSTANTIATE_TEST_SUITE_P(EveryCalculator, Small_CostCalculatorTest,
                         ::testing::ValuesIn(calculator_cases()),
                         [](const ::testing::TestParamInfo<CalculatorCase>& info)
                         {
                             return info.param.name;
                         });
