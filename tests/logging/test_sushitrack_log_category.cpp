/**************************************************************************/
/* test_sushitrack_log_category.cpp                                       */
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
 * @file test_sushitrack_log_category.cpp
 * @brief Tests the project and the category SushiTrack logs under.
 * @author Mustafa Garip
 */

#include <gtest/gtest.h>

#include <SushiTrack/logging/sushitrack_log_category.hpp>

using namespace SushiTrack;

/**
 * @brief The project is `sushitrack`, its levels come from SUSHITRACK_LOG_LEVEL, and the
 *        category's module is `sushitrack`.
 */
TEST(Small_SushiTrackLogCategoryTest, NamesTheProjectItsVariableAndTheModule)
{
    EXPECT_EQ(K_SUSHITRACK_LOG_PROJECT.name(), "sushitrack");
    EXPECT_EQ(K_SUSHITRACK_LOG_PROJECT.level_variable(), "SUSHITRACK_LOG_LEVEL");
    EXPECT_EQ(K_SUSHITRACK_LOG_CATEGORY.project().name(), "sushitrack");
    EXPECT_EQ(K_SUSHITRACK_LOG_CATEGORY.module(), "sushitrack");
}
