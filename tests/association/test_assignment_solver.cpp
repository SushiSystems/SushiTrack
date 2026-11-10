/**************************************************************************/
/* test_assignment_solver.cpp                                             */
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
 * @file test_assignment_solver.cpp
 * @brief Tests the AssignmentSolver contract through its one implementation, LAPJVSolver.
 * @author Mustafa Garip
 */

#include <memory>
#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/association/assignment_solver.hpp>
#include <SushiTrack/association/cost_matrix.hpp>
#include <SushiTrack/association/lapjv_solver.hpp>

using namespace SushiTrack;

/**
 * @brief Through AssignmentSolver&, a 2 x 2 matrix whose cheap cells are off the diagonal is
 *        matched off the diagonal.
 */
TEST(Small_AssignmentSolverTest, SolvesThroughTheInterface)
{
    std::unique_ptr<AssignmentSolver> solver = std::make_unique<LAPJVSolver>();
    CostMatrix cost;
    cost.resize(2, 2, 0.9f);
    cost(0, 1) = 0.1f;
    cost(1, 0) = 0.2f;

    std::vector<std::vector<int>> matches;
    std::vector<int> a_unmatched;
    std::vector<int> b_unmatched;
    solver->solve(cost, 2, 2, 0.5f, matches, a_unmatched, b_unmatched);

    ASSERT_EQ(matches.size(), 2u);
    for (const std::vector<int>& match : matches)
        EXPECT_NE(match[0], match[1]);
    EXPECT_TRUE(a_unmatched.empty());
    EXPECT_TRUE(b_unmatched.empty());
}
