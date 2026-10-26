/**************************************************************************/
/* test_filtering_log_category.cpp                                        */
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
 * @file test_filtering_log_category.cpp
 * @brief Tests the category the filtering area logs under.
 * @author Mustafa Garip
 */

#include <gtest/gtest.h>

#include <SushiTrack/filtering/filtering_log_category.hpp>

using namespace SushiTrack;

/**
 * @brief The category belongs to `sushitrack` and its module is `filtering`.
 */
TEST(Small_FilteringLogCategoryTest, NamesTheProjectAndTheModule)
{
    EXPECT_EQ(K_FILTERING_LOG_CATEGORY.project().name(), "sushitrack");
    EXPECT_EQ(K_FILTERING_LOG_CATEGORY.module(), "filtering");
}
