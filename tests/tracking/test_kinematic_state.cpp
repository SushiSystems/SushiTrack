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
