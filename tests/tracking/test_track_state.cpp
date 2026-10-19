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
