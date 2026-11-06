/**************************************************************************/
/* test_null_gate_validator.cpp                                           */
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
 * @file test_null_gate_validator.cpp
 * @brief Tests NullGateValidator, which accepts every measurement.
 * @author Mustafa Garip
 */

#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/filtering/kalman_types.hpp>
#include <SushiTrack/filtering/null_gate_validator.hpp>

#include <measurement_builders.hpp>

using namespace SushiTrack;

/**
 * @brief A measurement however far away is in the gate, and every flag is true.
 */
TEST(Small_NullGateValidatorTest, AcceptsEveryMeasurement)
{
    const NullGateValidator validator;
    const KalmanTypes::StateMean mean = KalmanTypes::StateMean::Zero();
    const KalmanTypes::StateCov covariance = KalmanTypes::StateCov::Identity();
    KalmanTypes::DetectBoxVector far{make_box(1e6f, 1e6f, 1.f, 1.f),
                                     make_box(-1e6f, 0.f, 1.f, 1.f)};

    EXPECT_TRUE(validator.is_in_gate(mean, covariance, far[0], 0.f));
    EXPECT_EQ(validator.get_gate_flags(mean, covariance, far, 0.f),
              (std::vector<bool>{true, true}));
    EXPECT_EQ(validator.get_gate_sq_distances(mean, covariance, far),
              (std::vector<float>{0.f, 0.f}));
}

/**
 * @brief get_instance hands out one shared validator.
 */
TEST(Small_NullGateValidatorTest, InstanceIsShared)
{
    EXPECT_EQ(NullGateValidator::get_instance(), NullGateValidator::get_instance());
}
