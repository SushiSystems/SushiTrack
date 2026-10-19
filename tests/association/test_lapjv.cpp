/**************************************************************************/
/* test_lapjv.cpp                                                         */
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
 * @file test_lapjv.cpp
 * @brief Tests lapjv_internal, the Jonker-Volgenant routine LAPJVSolver runs.
 * @author Mustafa Garip
 */

#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/association/lapjv.hpp>

using namespace SushiTrack;

/**
 * @brief A 3 x 3 matrix with one cheap cell per row is solved to that permutation, and the row
 *        and column solutions are inverse to each other.
 */
TEST(Small_LAPJVTest, FindsTheMinimumCostPermutation)
{
    std::vector<double> data = {9.0, 1.0, 9.0, //
                                9.0, 9.0, 1.0, //
                                1.0, 9.0, 9.0};
    std::vector<double*> rows = {&data[0], &data[3], &data[6]};
    std::vector<int> x(3, -1);
    std::vector<int> y(3, -1);
    LAPJVWorkspace workspace;
    workspace.resize(3);

    ASSERT_EQ(lapjv_internal(3, rows.data(), x.data(), y.data(), &workspace), 0);

    EXPECT_EQ(x, (std::vector<int>{1, 2, 0}));
    for (int row = 0; row < 3; ++row)
        EXPECT_EQ(y[x[row]], row);
}

/**
 * @brief resize grows every buffer to the size asked for and never shrinks one.
 */
TEST(Small_LAPJVTest, WorkspaceResizeOnlyGrows)
{
    LAPJVWorkspace workspace;
    workspace.resize(5);
    workspace.resize(2);

    EXPECT_EQ(workspace.free_rows.size(), 5u);
    EXPECT_EQ(workspace.v.size(), 5u);
    EXPECT_EQ(workspace.cols.size(), 5u);
    EXPECT_EQ(workspace.d.size(), 5u);
    EXPECT_EQ(workspace.pred.size(), 5u);
}
