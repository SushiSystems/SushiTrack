/**************************************************************************/
/* test_sushitrack_adapter.cpp                                            */
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
 * @file test_sushitrack_adapter.cpp
 * @brief Tests SushiTrackAdapter.
 * @author Mustafa Garip
 */

#include <vector>

#include <gtest/gtest.h>

#include <mot_detection.hpp>
#include <sushitrack_adapter.hpp>

using namespace TrackerBridge;

/**
 * @brief The adapter is named after the predictions folder `st eval` reads.
 */
TEST(Small_SushiTrackAdapterTest, IsNamedSushitrack)
{
    EXPECT_EQ(SushiTrackAdapter().name(), "sushitrack");
}

/**
 * @brief A frame with no detections gives no track.
 */
TEST(Small_SushiTrackAdapterTest, EmptyFrameGivesNoTrack)
{
    SushiTrackAdapter adapter;
    EXPECT_TRUE(adapter.update({}, 1.f / 30.f).empty());
}

/**
 * @brief A reported track carries the box of the detection it follows, within a pixel.
 */
TEST(Small_SushiTrackAdapterTest, ReportedTrackFollowsItsDetection)
{
    SushiTrackAdapter adapter;
    std::vector<TrackResult> tracks;
    for (int frame = 0; frame < 10; ++frame)
    {
        MOTDetection det;
        det.x = 200.f;
        det.y = 100.f;
        det.w = 40.f;
        det.h = 90.f;
        det.score = 0.95f;
        tracks = adapter.update({det}, 1.f / 30.f);
    }

    ASSERT_EQ(tracks.size(), 1u);
    EXPECT_NEAR(tracks[0].x, 200.f, 1.f);
    EXPECT_NEAR(tracks[0].h, 90.f, 1.f);
}
