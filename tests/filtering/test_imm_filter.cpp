/**************************************************************************/
/* test_imm_filter.cpp                                                    */
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
 * @file test_imm_filter.cpp
 * @brief Tests IMMFilter, the four-mode interacting multiple model estimator.
 * @author Mustafa Garip
 */

#include <cmath>

#include <gtest/gtest.h>

#include <SushiTrack/filtering/imm_filter.hpp>
#include <SushiTrack/filtering/kalman_types.hpp>

#include <measurement_builders.hpp>

using namespace SushiTrack;

/**
 * @brief A started filter holds the constant-velocity prior: manoeuvre 0.2 (CA and CT), stationary
 *        0.1 (NCP).
 */
TEST(Small_IMMFilterTest, StartsOnTheConstantVelocityPrior)
{
    IMMFilter filter;
    KalmanTypes::StateMean mean;
    KalmanTypes::StateCov covariance;
    filter.initiate(mean, covariance, make_box(100.f, 200.f, 50.f, 80.f), 0.9f);

    EXPECT_NEAR(filter.maneuver_probability(), 0.2f, 1e-5f);
    EXPECT_NEAR(filter.stationary_probability(), 0.1f, 1e-5f);
}

/**
 * @brief The mode probabilities stay a distribution through predict and update.
 */
TEST(Small_IMMFilterTest, ModeProbabilitiesStayWithinZeroAndOne)
{
    IMMFilter filter;
    KalmanTypes::StateMean mean;
    KalmanTypes::StateCov covariance;
    filter.initiate(mean, covariance, make_box(100.f, 200.f, 50.f, 80.f), 0.9f);
    for (int frame = 1; frame <= 10; ++frame)
    {
        filter.predict(mean, covariance, 1.0f);
        filter.update(mean, covariance, make_box(100.f + 4.f * frame, 200.f, 50.f, 80.f), 0.9f);
    }

    const float manoeuvre = filter.maneuver_probability();
    const float stationary = filter.stationary_probability();
    EXPECT_GE(manoeuvre, 0.f);
    EXPECT_GE(stationary, 0.f);
    EXPECT_LE(manoeuvre + stationary, 1.f + 1e-5f);
    EXPECT_TRUE(std::isfinite(mean(0)));
}

/**
 * @brief restore_checkpoint brings back the mode probabilities save_checkpoint took.
 */
TEST(Small_IMMFilterTest, RestoreCheckpointBringsBackTheModeProbabilities)
{
    IMMFilter filter;
    KalmanTypes::StateMean mean;
    KalmanTypes::StateCov covariance;
    filter.initiate(mean, covariance, make_box(100.f, 200.f, 50.f, 80.f), 0.9f);
    filter.save_checkpoint();
    const float saved = filter.maneuver_probability();

    for (int frame = 1; frame <= 5; ++frame)
    {
        filter.predict(mean, covariance, 1.0f);
        filter.update(mean, covariance, make_box(100.f + 40.f * frame * frame, 200.f, 50.f, 80.f),
                      0.9f);
    }
    filter.restore_checkpoint();

    EXPECT_NEAR(filter.maneuver_probability(), saved, 1e-6f);
}
