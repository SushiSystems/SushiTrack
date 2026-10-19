/**************************************************************************/
/* test_gated_state_predictor.cpp                                         */
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
 * @file test_gated_state_predictor.cpp
 * @brief Tests that every registered GatedStatePredictor gates its own start in and a far box out.
 * @author Mustafa Garip
 */

#include <memory>

#include <gtest/gtest.h>

#include <SushiTrack/filtering/gated_state_predictor.hpp>
#include <SushiTrack/filtering/kalman_filter_registry.hpp>
#include <SushiTrack/filtering/kalman_types.hpp>

#include <measurement_builders.hpp>

using namespace SushiTrack;

namespace
{
    /** @brief Runs each registered Kalman type: 0 linear, 1 NSA, 2 IMM. */
    class Small_GatedStatePredictorTest : public ::testing::TestWithParam<int>
    {
    };
} // namespace

/**
 * @brief After one predict step, the start box is in the gate and a box 5000 px away is not.
 */
TEST_P(Small_GatedStatePredictorTest, GatesItsStartInAndAFarBoxOut)
{
    std::unique_ptr<GatedStatePredictor> filter =
        KalmanFilterRegistry::get_instance().create(GetParam(), false);
    ASSERT_NE(filter, nullptr);

    KalmanTypes::StateMean mean;
    KalmanTypes::StateCov covariance;
    const KalmanTypes::DetectBox start = make_box(100.f, 200.f, 50.f, 80.f);
    filter->initiate(mean, covariance, start, 0.9f);
    filter->predict(mean, covariance, 1.0f);

    EXPECT_TRUE(filter->is_in_gate(mean, covariance, start, 9.4877f, 0.9f));
    EXPECT_FALSE(
        filter->is_in_gate(mean, covariance, make_box(5100.f, 200.f, 50.f, 80.f), 9.4877f, 0.9f));
}

/**
 * @brief The covariance stays symmetric through a predict and an update.
 */
TEST_P(Small_GatedStatePredictorTest, CovarianceStaysSymmetric)
{
    std::unique_ptr<GatedStatePredictor> filter =
        KalmanFilterRegistry::get_instance().create(GetParam(), false);
    ASSERT_NE(filter, nullptr);

    KalmanTypes::StateMean mean;
    KalmanTypes::StateCov covariance;
    filter->initiate(mean, covariance, make_box(100.f, 200.f, 50.f, 80.f), 0.9f);
    filter->predict(mean, covariance, 1.0f);
    filter->update(mean, covariance, make_box(104.f, 201.f, 50.f, 80.f), 0.9f);

    EXPECT_TRUE(covariance.isApprox(covariance.transpose(), 1e-4f));
}

INSTANTIATE_TEST_SUITE_P(EveryRegisteredType, Small_GatedStatePredictorTest,
                         ::testing::Values(0, 1, 2));
