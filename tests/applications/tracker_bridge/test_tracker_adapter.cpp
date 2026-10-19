/**************************************************************************/
/* test_tracker_adapter.cpp                                               */
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
 * @file test_tracker_adapter.cpp
 * @brief Tests the TrackerAdapter contract every adapter keeps: a steady object keeps one identity.
 * @author Mustafa Garip
 */

#include <functional>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <byte_track_adapter.hpp>
#include <mot_detection.hpp>
#include <oc_sort_adapter.hpp>
#include <sushitrack_adapter.hpp>
#include <tracker_adapter.hpp>

namespace
{
    /** @brief Builds one adapter. */
    using AdapterFactory = std::function<std::unique_ptr<TrackerBridge::TrackerAdapter>()>;

    /** @brief Runs each adapter: SushiTrack, ByteTrack and OC-SORT with their defaults. */
    class Small_TrackerAdapterTest : public ::testing::TestWithParam<AdapterFactory>
    {
    };
} // namespace

/**
 * @brief Through TrackerAdapter&, one object moving 2 px a frame is reported under one identity.
 */
TEST_P(Small_TrackerAdapterTest, SteadyObjectKeepsOneIdentity)
{
    std::unique_ptr<TrackerBridge::TrackerAdapter> adapter = GetParam()();
    adapter->set_frame_rate(30);

    std::set<int> identities;
    for (int frame = 0; frame < 20; ++frame)
    {
        TrackerBridge::MOTDetection det;
        det.x = 100.f + 2.f * frame;
        det.y = 100.f;
        det.w = 50.f;
        det.h = 80.f;
        det.score = 0.9f;
        for (const TrackerBridge::TrackResult& track : adapter->update({det}, 1.f / 30.f))
            identities.insert(track.track_id);
    }

    EXPECT_FALSE(adapter->name().empty());
    EXPECT_EQ(identities.size(), 1u);
}

INSTANTIATE_TEST_SUITE_P(
    EveryAdapter, Small_TrackerAdapterTest,
    ::testing::Values(AdapterFactory(
                          []
                          {
                              return std::make_unique<TrackerBridge::SushiTrackAdapter>();
                          }),
                      AdapterFactory(
                          []
                          {
                              return std::make_unique<TrackerBridge::ByteTrackAdapter>();
                          }),
                      AdapterFactory(
                          []
                          {
                              return std::make_unique<TrackerBridge::OCSORTAdapter>();
                          })));
