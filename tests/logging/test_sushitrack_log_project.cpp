/**************************************************************************/
/* test_sushitrack_log_project.cpp                                       */
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
 * @file test_sushitrack_log_project.cpp
 * @brief Tests the project every SushiTrack log category belongs to.
 * @author Mustafa Garip
 */

#include <gtest/gtest.h>

#include <SushiTrack/logging/sushitrack_log_project.hpp>

using namespace SushiTrack;

/**
 * @brief The project is `sushitrack` and its levels come from SUSHITRACK_LOG_LEVEL.
 */
TEST(Small_SushiTrackLogProjectTest, NamesTheProjectAndItsVariable)
{
    EXPECT_EQ(K_SUSHITRACK_LOG_PROJECT.name(), "sushitrack");
    EXPECT_EQ(K_SUSHITRACK_LOG_PROJECT.level_variable(), "SUSHITRACK_LOG_LEVEL");
}
