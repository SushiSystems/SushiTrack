/**************************************************************************/
/* test_tracker_config.cpp                                                */
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
 * @file test_tracker_config.cpp
 * @brief Tests that the TrackerConfig defaults describe a tracker that can run.
 * @author Mustafa Garip
 */

#include <gtest/gtest.h>

#include <SushiTrack/tracking/tracker_config.hpp>

using namespace SushiTrack;

/**
 * @brief The default thresholds are ordered and in range: a detection that starts a track is a
 *        high-confidence one, and every IoU threshold is a fraction.
 */
TEST(Small_TrackerConfigTest, DefaultThresholdsAreOrderedAndInRange)
{
    const TrackerConfig config;

    EXPECT_GT(config.frame_rate, 0);
    EXPECT_GT(config.track_buffer, 0);
    EXPECT_LT(config.track_thresh, config.high_thresh);
    for (float fraction :
         {config.match_thresh, config.iou_match_thresh, config.unconfirmed_iou_thresh,
          config.duplicate_iou_thresh, config.feature_momentum})
    {
        EXPECT_GT(fraction, 0.f);
        EXPECT_LE(fraction, 1.f);
    }
}

/**
 * @brief The default gate is the 95% chi-square bound for four degrees of freedom.
 */
TEST(Small_TrackerConfigTest, DefaultGateIsTheFourDegreeChiSquareBound)
{
    EXPECT_NEAR(TrackerConfig{}.gating_thresh, 9.4877f, 1e-4f);
}
