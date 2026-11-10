/**************************************************************************/
/* test_gate_validator.cpp                                                */
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
 * @file test_gate_validator.cpp
 * @brief Tests the GateValidator contract and the default it gives get_gate_sq_distances.
 * @author Mustafa Garip
 */

#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/filtering/gate_validator.hpp>
#include <SushiTrack/filtering/in_memory_gate_validator.hpp>
#include <SushiTrack/filtering/kalman_types.hpp>

#include <measurement_builders.hpp>

using namespace SushiTrack;

/**
 * @brief The default get_gate_sq_distances gives one zero per measurement.
 *
 * InMemoryGateValidator leaves the default in place, and two measurements give two zeros.
 */
TEST(Small_GateValidatorTest, DefaultSquaredDistancesAreZeroPerMeasurement)
{
    const InMemoryGateValidator validator;
    const GateValidator& gate = validator;
    KalmanTypes::DetectBoxVector measurements{make_box(0.f, 0.f, 1.f, 1.f),
                                              make_box(5.f, 5.f, 1.f, 1.f)};

    const std::vector<float> distances = gate.get_gate_sq_distances(
        KalmanTypes::StateMean::Zero(), KalmanTypes::StateCov::Identity(), measurements);

    EXPECT_EQ(distances, (std::vector<float>{0.f, 0.f}));
}
