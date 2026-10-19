/**************************************************************************/
/* test_mot_detection.cpp                                                 */
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
 * @file test_mot_detection.cpp
 * @brief Tests the defaults of MOTDetection.
 * @author Mustafa Garip
 */

#include <gtest/gtest.h>

#include <mot_detection.hpp>

/**
 * @brief A detection built with no values is an empty class-1 box with no feature.
 */
TEST(Small_MOTDetectionTest, DefaultsToAnEmptyClassOneBox)
{
    const TrackerBridge::MOTDetection det;
    EXPECT_FLOAT_EQ(det.w, 0.f);
    EXPECT_FLOAT_EQ(det.score, 0.f);
    EXPECT_EQ(det.label, 1);
    EXPECT_TRUE(det.feature.empty());
}
