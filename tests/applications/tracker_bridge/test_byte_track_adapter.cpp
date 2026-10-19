/**************************************************************************/
/* test_byte_track_adapter.cpp                                            */
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
 * @file test_byte_track_adapter.cpp
 * @brief Tests ByteTrackAdapter.
 * @author Mustafa Garip
 */

#include <gtest/gtest.h>

#include <byte_track_adapter.hpp>

using namespace TrackerBridge;

/**
 * @brief The adapter is named after the predictions folder `st eval` reads.
 */
TEST(Small_ByteTrackAdapterTest, IsNamedBytetrack)
{
    EXPECT_EQ(ByteTrackAdapter().name(), "bytetrack");
}

/**
 * @brief A settings file that does not exist leaves the defaults, and the adapter still runs.
 */
TEST(Small_ByteTrackAdapterTest, MissingSettingsKeepTheDefaults)
{
    ByteTrackAdapter adapter("this/path/does/not/exist.json");
    EXPECT_TRUE(adapter.update({}, 1.f).empty());
}
