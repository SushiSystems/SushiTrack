/**************************************************************************/
/* test_kinematic_state.cpp                                               */
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
 * @file test_kinematic_state.cpp
 * @brief Tests the numbering of KinematicState, which the C API reports as an integer.
 * @author Mustafa Garip
 */

#include <cstdint>

#include <gtest/gtest.h>

#include <SushiTrack/tracking/kinematic_state.hpp>

using namespace SushiTrack;

static_assert(sizeof(KinematicState) == sizeof(std::uint8_t), "a regime fits one byte");

/**
 * @brief The regimes keep the numbers sushitrack_track_t.kinematic_state documents.
 */
TEST(Small_KinematicStateTest, KeepsTheNumbersTheCApiDocuments)
{
    EXPECT_EQ(static_cast<int>(KinematicState::Stationary), 0);
    EXPECT_EQ(static_cast<int>(KinematicState::Cruising), 1);
    EXPECT_EQ(static_cast<int>(KinematicState::Maneuvering), 2);
    EXPECT_EQ(static_cast<int>(KinematicState::Coasting), 3);
}
