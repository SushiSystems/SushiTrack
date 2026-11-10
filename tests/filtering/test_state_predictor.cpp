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
 * @file test_state_predictor.cpp
 * @brief Tests the defaults StatePredictor gives a predictor with no mode bank.
 * @author Mustafa Garip
 */

#include <gtest/gtest.h>

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
