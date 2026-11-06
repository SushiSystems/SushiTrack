/**************************************************************************/
/* test_track_state.cpp                                                   */
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
 * @file test_track_state.cpp
 * @brief Tests the numbering of TrackState, which the C API reports as an integer.
 * @author Mustafa Garip
 */

#include <cstdint>

#include <gtest/gtest.h>

#include <SushiTrack/tracking/track_state.hpp>

using namespace SushiTrack;

static_assert(sizeof(TrackState) == sizeof(std::uint8_t), "a state fits one byte");

/**
 * @brief The states keep the numbers sushitrack_track_t.state carries.
 */
TEST(Small_TrackStateTest, KeepsTheNumbersTheCApiCarries)
{
    EXPECT_EQ(static_cast<int>(TrackState::New), 0);
    EXPECT_EQ(static_cast<int>(TrackState::Tracked), 1);
    EXPECT_EQ(static_cast<int>(TrackState::Lost), 2);
    EXPECT_EQ(static_cast<int>(TrackState::Removed), 3);
    EXPECT_EQ(static_cast<int>(TrackState::Tentative), 4);
}
