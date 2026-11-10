/**************************************************************************/
/* test_in_memory_gate_validator.cpp                                      */
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
 * @file test_in_memory_gate_validator.cpp
 * @brief Tests InMemoryGateValidator, which gives one fixed verdict.
 * @author Mustafa Garip
 */

#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/filtering/in_memory_gate_validator.hpp>
#include <SushiTrack/filtering/kalman_types.hpp>

#include <measurement_builders.hpp>

using namespace SushiTrack;

/**
 * @brief The validator built to accept or to reject, as a row.
 */
class Small_InMemoryGateValidatorTest : public ::testing::TestWithParam<bool>
{
};

/**
 * @brief is_in_gate() returns the verdict the validator was built with, for a measurement near
 *        the mean and for one far from it.
 */
TEST_P(Small_InMemoryGateValidatorTest, IsInGateReturnsTheBuiltVerdict)
{
    const InMemoryGateValidator validator(GetParam());
    const KalmanTypes::StateMean mean = KalmanTypes::StateMean::Zero();
    const KalmanTypes::StateCov covariance = KalmanTypes::StateCov::Identity();

    EXPECT_EQ(validator.is_in_gate(mean, covariance, make_box(0.f, 0.f, 1.f, 1.f), 9.f),
              GetParam());
    EXPECT_EQ(validator.is_in_gate(mean, covariance, make_box(1e6f, 1e6f, 1.f, 1.f), 9.f),
              GetParam());
}

/**
 * @brief get_gate_flags() returns the built verdict once per measurement.
 */
TEST_P(Small_InMemoryGateValidatorTest, GateFlagsRepeatTheBuiltVerdictPerMeasurement)
{
    const InMemoryGateValidator validator(GetParam());
    const KalmanTypes::DetectBoxVector measurements{make_box(0.f, 0.f, 1.f, 1.f),
                                                    make_box(1e6f, 1e6f, 1.f, 1.f),
                                                    make_box(-5.f, 3.f, 2.f, 2.f)};

    EXPECT_EQ(validator.get_gate_flags(KalmanTypes::StateMean::Zero(),
                                       KalmanTypes::StateCov::Identity(), measurements, 9.f),
              (std::vector<bool>(3, GetParam())));
}

INSTANTIATE_TEST_SUITE_P(Verdicts, Small_InMemoryGateValidatorTest, ::testing::Bool());

/**
 * @brief A validator built with no argument accepts.
 */
TEST(Small_InMemoryGateValidatorDefaultTest, DefaultBuiltValidatorAccepts)
{
    const InMemoryGateValidator validator;

    EXPECT_TRUE(validator.is_in_gate(KalmanTypes::StateMean::Zero(),
                                     KalmanTypes::StateCov::Identity(),
                                     make_box(1e6f, 1e6f, 1.f, 1.f), 9.f));
}
