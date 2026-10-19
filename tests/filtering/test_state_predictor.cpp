/**************************************************************************/
/* test_state_predictor.cpp                                               */
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
 * @file test_state_predictor.cpp
 * @brief Tests the defaults StatePredictor gives a predictor with no mode bank.
 * @author Mustafa Garip
 */

#include <gtest/gtest.h>

#include <SushiTrack/filtering/imm_filter.hpp>
#include <SushiTrack/filtering/kalman_filter.hpp>
#include <SushiTrack/filtering/kalman_types.hpp>
#include <SushiTrack/filtering/state_predictor.hpp>

#include <measurement_builders.hpp>

using namespace SushiTrack;

/**
 * @brief A single-model predictor reports -1 for both probabilities, meaning it has none.
 */
TEST(Small_StatePredictorTest, SingleModelReportsNoModeProbabilities)
{
    KalmanFilter filter;
    const StatePredictor& predictor = filter;

    EXPECT_FLOAT_EQ(predictor.maneuver_probability(), -1.f);
    EXPECT_FLOAT_EQ(predictor.stationary_probability(), -1.f);
}

/**
 * @brief The default checkpoint calls leave a single-model predictor's state where it was.
 */
TEST(Small_StatePredictorTest, DefaultCheckpointLeavesTheStateAlone)
{
    KalmanFilter filter;
    StatePredictor& predictor = filter;
    KalmanTypes::StateMean mean;
    KalmanTypes::StateCov covariance;
    predictor.initiate(mean, covariance, make_box(100.f, 200.f, 50.f, 80.f));
    const KalmanTypes::StateMean before = mean;

    predictor.save_checkpoint();
    predictor.restore_checkpoint();

    EXPECT_TRUE(mean.isApprox(before));
}

/**
 * @brief The IMM overrides the defaults with probabilities in [0, 1].
 */
TEST(Small_StatePredictorTest, MultiModelReportsProbabilities)
{
    IMMFilter filter;
    StatePredictor& predictor = filter;
    KalmanTypes::StateMean mean;
    KalmanTypes::StateCov covariance;
    predictor.initiate(mean, covariance, make_box(100.f, 200.f, 50.f, 80.f), 0.9f);

    EXPECT_GE(predictor.maneuver_probability(), 0.f);
    EXPECT_LE(predictor.maneuver_probability(), 1.f);
}
