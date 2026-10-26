/**************************************************************************/
/* test_profiler_keys.cpp                                                 */
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
 * @file test_profiler_keys.cpp
 * @brief Tests that every name SushiTrack records under is set and none is used twice.
 * @author Mustafa Garip
 */

#include <set>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/profiling/profiler_keys.hpp>

namespace Keys = SushiTrack::ProfilerKeys;

/**
 * @brief The zone, counter, instant and fact names are non-empty and pairwise different, so a
 *        record's analyses never mistake one for another.
 */
TEST(Small_ProfilerKeysTest, NamesAreSetAndUnique)
{
    const std::vector<std::string> names = {
        Keys::K_TIMELINE_FRAME,
        Keys::K_ZONE_UPDATE,
        Keys::K_ZONE_SPLIT,
        Keys::K_ZONE_PREDICT,
        Keys::K_ZONE_MATCH_HIGH,
        Keys::K_ZONE_MATCH_LOW,
        Keys::K_ZONE_LOST_RECOVERY,
        Keys::K_ZONE_TENTATIVE,
        Keys::K_ZONE_UNCONFIRMED,
        Keys::K_ZONE_CLEANUP_LOST,
        Keys::K_ZONE_FINALIZE,
        Keys::K_ZONE_OUTPUT,
        Keys::K_ZONE_SOLVE,
        Keys::K_ZONE_COST_MATRIX,
        Keys::K_COUNTER_DETECTIONS_IN,
        Keys::K_COUNTER_DETECTIONS_HIGH,
        Keys::K_COUNTER_DETECTIONS_LOW,
        Keys::K_COUNTER_TRACKS_POOL,
        Keys::K_COUNTER_TRACKS_TENTATIVE_IN,
        Keys::K_COUNTER_TRACKS_NON_ACTIVE,
        Keys::K_COUNTER_MATCHES_HIGH,
        Keys::K_COUNTER_MATCHES_LOW,
        Keys::K_COUNTER_TRACKS_NEWLY_LOST,
        Keys::K_COUNTER_DETECTIONS_LOW_REMAINING,
        Keys::K_COUNTER_TRACKS_RECOVERED,
        Keys::K_COUNTER_TENTATIVE_CONFIRMED,
        Keys::K_COUNTER_TENTATIVE_REMOVED,
        Keys::K_COUNTER_TRACKS_STARTED,
        Keys::K_COUNTER_ALIVE_TRACKED,
        Keys::K_COUNTER_ALIVE_LOST,
        Keys::K_COUNTER_ALIVE_TENTATIVE,
        Keys::K_COUNTER_REMOVED,
        Keys::K_COUNTER_TRACKS_WRITTEN,
        Keys::K_COUNTER_TRACKS_DROPPED,
        Keys::K_COUNTER_COST_CELLS,
        Keys::K_INSTANT_TRACK_LOST,
        Keys::K_INSTANT_TRACK_CONFIRMED,
        Keys::K_INSTANT_TRACK_REMOVED,
        Keys::K_INSTANT_TRACK_RECOVERED,
        Keys::K_FACT_LIBRARY,
        Keys::K_FACT_VERSION,
        Keys::K_FACT_COMMIT,
        Keys::K_FACT_DATE,
        Keys::K_FACT_PROFILE_LEVEL};

    const std::set<std::string> unique(names.begin(), names.end());
    EXPECT_EQ(unique.size(), names.size());
    EXPECT_EQ(unique.count(""), 0U);
}
