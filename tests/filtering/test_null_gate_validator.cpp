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

namespace
{
    /**
     * @brief Returns two boxes a million pixels from the origin, in opposite directions.
     */
    KalmanTypes::DetectBoxVector far_boxes()
    {
        return KalmanTypes::DetectBoxVector{make_box(1e6f, 1e6f, 1.f, 1.f),
                                            make_box(-1e6f, 0.f, 1.f, 1.f)};
    }
} // namespace

/**
 * @brief A measurement however far away is in the gate.
 */
TEST(Small_NullGateValidatorTest, IsInGateAcceptsAFarMeasurement)
{
    const NullGateValidator validator;
    const KalmanTypes::DetectBoxVector far = far_boxes();

    EXPECT_TRUE(validator.is_in_gate(KalmanTypes::StateMean::Zero(),
                                     KalmanTypes::StateCov::Identity(), far[0], 0.f));
}

/**
 * @brief The batch flags of far measurements are all true.
 */
TEST(Small_NullGateValidatorTest, GateFlagsAreAllTrue)
{
    const NullGateValidator validator;

    EXPECT_EQ(validator.get_gate_flags(KalmanTypes::StateMean::Zero(),
                                       KalmanTypes::StateCov::Identity(), far_boxes(), 0.f),
              (std::vector<bool>{true, true}));
}

/**
 * @brief The batch squared distances of far measurements are all zero.
 */
TEST(Small_NullGateValidatorTest, GateSquaredDistancesAreAllZero)
{
    const NullGateValidator validator;

    EXPECT_EQ(validator.get_gate_sq_distances(KalmanTypes::StateMean::Zero(),
                                              KalmanTypes::StateCov::Identity(), far_boxes()),
              (std::vector<float>{0.f, 0.f}));
}
