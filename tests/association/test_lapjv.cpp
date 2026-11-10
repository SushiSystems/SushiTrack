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
/* Licensed under the Apache License, Version 2.0 (the "License"); you    */
/* may not use this file except in compliance with the License. You may   */
/* obtain a copy of the License at                                        */
/*                                                                        */
/*     https://www.apache.org/licenses/LICENSE-2.0                        */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                         */
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
 * @brief A workspace that was asked for 5 and then for 2 still solves a 5 x 5 problem: the second
 *        resize did not shrink what the first one grew.
 */
TEST(Small_LAPJVTest, WorkspaceResizedDownStillSolvesTheLargerProblem)
{
    // The cheap cell of row r is column (r + 2) % 5.
    std::vector<double> data(25, 9.0);
    std::vector<double*> rows;
    for (int row = 0; row < 5; ++row)
    {
        data[row * 5 + (row + 2) % 5] = 1.0;
        rows.push_back(&data[row * 5]);
    }
    std::vector<int> x(5, -1);
    std::vector<int> y(5, -1);
    LAPJVWorkspace workspace;
    workspace.resize(5);
    workspace.resize(2);

    ASSERT_EQ(lapjv_internal(5, rows.data(), x.data(), y.data(), &workspace), 0);

    EXPECT_EQ(x, (std::vector<int>{2, 3, 4, 0, 1}));
}
