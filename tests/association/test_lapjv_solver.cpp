/**************************************************************************/
/* test_lapjv_solver.cpp                                                  */
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
 * @file test_lapjv_solver.cpp
 * @brief Tests LAPJVSolver.
 * @author Mustafa Garip
 */

#include <algorithm>
#include <limits>
#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/association/cost_matrix.hpp>
#include <SushiTrack/association/lapjv_solver.hpp>

#include <captured_log.hpp>

using namespace SushiTrack;

// Helpers

/**
 * @brief Constructs an n×n identity-like cost matrix:
 *        diagonal = 0 (perfect match), off-diagonal = 1 (no match).
 */
static CostMatrix make_identity_cost(int n)
{
    CostMatrix m;
    m.resize(n, n, 1.0f);
    for (int i = 0; i < n; ++i)
        m(i, i) = 0.0f;
    return m;
}

// Empty matrix

/**
 * @brief solve() on a 0×0 matrix reports no matches and all indices unmatched.
 */
TEST(Small_LAPJVSolverTest, EmptyMatrixAllUnmatched)
{
    LAPJVSolver solver;
    CostMatrix cost;

    std::vector<std::vector<int>> matches;
    std::vector<int> a_unm, b_unm;
    solver.solve(cost, 0, 0, 1.0f, matches, a_unm, b_unm);

    EXPECT_TRUE(matches.empty());
    EXPECT_TRUE(a_unm.empty());
    EXPECT_TRUE(b_unm.empty());
}

// 1×1 matrix

/**
 * @brief A 1×1 matrix with cost below threshold produces exactly one match.
 */
TEST(Small_LAPJVSolverTest, OneByOneMatchBelow)
{
    LAPJVSolver solver;
    CostMatrix cost;
    cost.resize(1, 1, 0.5f);

    std::vector<std::vector<int>> matches;
    std::vector<int> a_unm, b_unm;
    solver.solve(cost, 1, 1, 0.9f, matches, a_unm, b_unm);

    ASSERT_EQ(matches.size(), 1u);
    EXPECT_EQ(matches[0][0], 0);
    EXPECT_EQ(matches[0][1], 0);
    EXPECT_TRUE(a_unm.empty());
    EXPECT_TRUE(b_unm.empty());
}

/**
 * @brief A 1×1 matrix with cost above threshold produces no matches.
 */
TEST(Small_LAPJVSolverTest, OneByOneMatchAboveThreshold)
{
    LAPJVSolver solver;
    CostMatrix cost;
    cost.resize(1, 1, 0.9f);

    std::vector<std::vector<int>> matches;
    std::vector<int> a_unm, b_unm;
    solver.solve(cost, 1, 1, 0.5f, matches, a_unm, b_unm);

    EXPECT_TRUE(matches.empty());
    ASSERT_EQ(a_unm.size(), 1u);
    EXPECT_EQ(a_unm[0], 0);
    ASSERT_EQ(b_unm.size(), 1u);
    EXPECT_EQ(b_unm[0], 0);
}

// Square identity matrix

/**
 * @brief An n×n identity cost matrix produces n diagonal matches with zero
 *        cost, and no unmatched rows or columns.
 */
TEST(Small_LAPJVSolverTest, IdentityMatrixPerfectDiagonalMatch)
{
    const int n = 4;
    LAPJVSolver solver;
    CostMatrix cost = make_identity_cost(n);

    std::vector<std::vector<int>> matches;
    std::vector<int> a_unm, b_unm;
    solver.solve(cost, n, n, 0.5f, matches, a_unm, b_unm);

    EXPECT_EQ(static_cast<int>(matches.size()), n);
    EXPECT_TRUE(a_unm.empty());
    EXPECT_TRUE(b_unm.empty());

    // Every row should match its own column.
    std::vector<bool> col_seen(n, false);
    for (const auto& pair : matches)
    {
        EXPECT_EQ(pair[0], pair[1]) << "row " << pair[0] << " matched col " << pair[1];
        EXPECT_FALSE(col_seen[pair[1]]) << "column " << pair[1] << " matched twice";
        col_seen[pair[1]] = true;
    }
}

// Rectangular: more rows than columns

/**
 * @brief The surplus side of a rectangular matrix stays unmatched and the short side matches in
 *        full: 4 x 2 leaves rows 2 and 3, 2 x 4 leaves columns 2 and 3, and both match the
 *        diagonal.
 */
TEST(Small_LAPJVSolverTest, TheSurplusSideOfARectangularMatrixIsUnmatched)
{
    LAPJVSolver solver;
    CostMatrix tall;
    CostMatrix wide;
    tall.resize(4, 2, 1.0f);
    wide.resize(2, 4, 1.0f);
    tall(0, 0) = 0.0f;
    tall(1, 1) = 0.0f;
    wide(0, 0) = 0.0f;
    wide(1, 1) = 0.0f;
    std::vector<std::vector<int>> tall_matches;
    std::vector<std::vector<int>> wide_matches;
    std::vector<int> tall_a_unm, tall_b_unm, wide_a_unm, wide_b_unm;

    solver.solve(tall, 4, 2, 0.5f, tall_matches, tall_a_unm, tall_b_unm);
    solver.solve(wide, 2, 4, 0.5f, wide_matches, wide_a_unm, wide_b_unm);

    const std::vector<std::vector<int>> diagonal{{0, 0}, {1, 1}};
    std::sort(tall_matches.begin(), tall_matches.end());
    std::sort(wide_matches.begin(), wide_matches.end());
    EXPECT_EQ(tall_matches, diagonal);
    EXPECT_EQ(tall_a_unm, (std::vector<int>{2, 3}));
    EXPECT_TRUE(tall_b_unm.empty());
    EXPECT_EQ(wide_matches, diagonal);
    EXPECT_TRUE(wide_a_unm.empty());
    EXPECT_EQ(wide_b_unm, (std::vector<int>{2, 3}));
}

// Threshold sensitivity

/**
 * @brief Tightening the threshold from 0.9 to 0.3 suppresses matches whose
 *        cost exceeds the new limit.
 */
TEST(Small_LAPJVSolverTest, ThresholdControlsAcceptedAssignments)
{
    LAPJVSolver solver;
    CostMatrix cost;
    cost.resize(2, 2);
    cost(0, 0) = 0.2f; // good match
    cost(0, 1) = 0.9f;
    cost(1, 0) = 0.9f;
    cost(1, 1) = 0.7f; // borderline match

    // Loose threshold: both diagonal matches are accepted.
    {
        std::vector<std::vector<int>> matches;
        std::vector<int> a_unm, b_unm;
        solver.solve(cost, 2, 2, 0.9f, matches, a_unm, b_unm);
        EXPECT_EQ(matches.size(), 2u);
    }

    // Tight threshold: only the 0.2 match survives.
    {
        std::vector<std::vector<int>> matches;
        std::vector<int> a_unm, b_unm;
        solver.solve(cost, 2, 2, 0.3f, matches, a_unm, b_unm);
        EXPECT_EQ(matches.size(), 1u);
        if (!matches.empty())
        {
            EXPECT_EQ(matches[0][0], 0);
            EXPECT_EQ(matches[0][1], 0);
        }
    }
}

// No double-assignments

// Repeated calls

/**
 * @brief Two consecutive calls on the same solver both match the diagonal of an identity-like
 *        matrix: the workspace of the first call does not corrupt the second.
 */
TEST(Small_LAPJVSolverTest, EveryCallOnOneSolverMatchesTheDiagonal)
{
    LAPJVSolver solver;
    CostMatrix cost = make_identity_cost(3);
    const std::vector<std::vector<int>> diagonal{{0, 0}, {1, 1}, {2, 2}};

    for (int call = 0; call < 2; ++call)
    {
        std::vector<std::vector<int>> matches;
        std::vector<int> a_unm, b_unm;
        solver.solve(cost, 3, 3, 0.5f, matches, a_unm, b_unm);
        std::sort(matches.begin(), matches.end());
        EXPECT_EQ(matches, diagonal) << "call " << call;
    }
}

// Tier 3 edge cases

/**
 * @brief A matrix where every cell equals the sentinel value (1e4f) produces
 *        no matches; all rows and columns are reported as unmatched.
 */
TEST(Small_LAPJVSolverTest, AllSentinelMatrixProducesNoMatches)
{
    LAPJVSolver solver;
    CostMatrix cost;
    cost.resize(3, 3, 1e4f);

    std::vector<std::vector<int>> matches;
    std::vector<int> a_unm, b_unm;
    solver.solve(cost, 3, 3, 0.9f, matches, a_unm, b_unm);

    EXPECT_TRUE(matches.empty());
    EXPECT_EQ(a_unm.size(), 3u);
    EXPECT_EQ(b_unm.size(), 3u);
}

/**
 * @brief A 1×N matrix where all costs exceed the threshold produces no matches
 *        and reports the single row and all N columns as unmatched.
 */
TEST(Small_LAPJVSolverTest, OneRowAllAboveThresholdNoMatch)
{
    LAPJVSolver solver;
    CostMatrix cost;
    cost.resize(1, 4, 0.9f);

    std::vector<std::vector<int>> matches;
    std::vector<int> a_unm, b_unm;
    solver.solve(cost, 1, 4, 0.5f, matches, a_unm, b_unm);

    EXPECT_TRUE(matches.empty());
    EXPECT_EQ(a_unm.size(), 1u);
    EXPECT_EQ(b_unm.size(), 4u);
}

/**
 * @brief A 3×3 permutation matrix (each row's minimum is in a different column)
 *        produces three correct cross-diagonal matches.
 */
TEST(Small_LAPJVSolverTest, PermutationMatrixMatchesOptimally)
{
    LAPJVSolver solver;
    CostMatrix cost;
    cost.resize(3, 3, 1.0f);
    cost(0, 2) = 0.0f;
    cost(1, 0) = 0.0f;
    cost(2, 1) = 0.0f;

    std::vector<std::vector<int>> matches;
    std::vector<int> a_unm, b_unm;
    solver.solve(cost, 3, 3, 0.5f, matches, a_unm, b_unm);

    EXPECT_EQ(matches.size(), 3u);
    EXPECT_TRUE(a_unm.empty());
    EXPECT_TRUE(b_unm.empty());

    std::vector<int> row_to_col(3, -1);
    for (const auto& p : matches)
        row_to_col[p[0]] = p[1];

    EXPECT_EQ(row_to_col[0], 2);
    EXPECT_EQ(row_to_col[1], 0);
    EXPECT_EQ(row_to_col[2], 1);
}

// Records

/**
 * @brief Non-zero counts over an empty matrix log one ERROR naming the size, and every row and
 *        column stays unmatched.
 */
TEST(Small_LAPJVSolverTest, EmptyMatrixWithCountsLogsAnError)
{
    CapturedLog log;
    LAPJVSolver solver;
    CostMatrix cost;
    std::vector<std::vector<int>> matches;
    std::vector<int> a_unm, b_unm;

    solver.solve(cost, 2, 3, 1.0f, matches, a_unm, b_unm);

    EXPECT_TRUE(matches.empty());
    EXPECT_EQ(a_unm.size(), 2U);
    EXPECT_EQ(b_unm.size(), 3U);
    EXPECT_EQ(log.count("Cost matrix is empty for 2 x 3"), 1);
    EXPECT_EQ(log.level_of("Cost matrix is empty"), SushiLog::LogLevel::Error);
}

/**
 * @brief A matrix holding a NaN logs one WARN however many times it is solved.
 */
TEST(Small_LAPJVSolverTest, NonFiniteCostLogsOneWarn)
{
    CapturedLog log;
    LAPJVSolver solver;
    CostMatrix cost;
    cost.resize(2, 2, 1.0f);
    cost(0, 0) = std::numeric_limits<float>::quiet_NaN();
    cost(1, 1) = 0.0f;

    for (int call = 0; call < 3; ++call)
    {
        std::vector<std::vector<int>> matches;
        std::vector<int> a_unm, b_unm;
        solver.solve(cost, 2, 2, 0.5f, matches, a_unm, b_unm);
    }

    EXPECT_EQ(log.count("holds 1 non-finite costs"), 1);
    EXPECT_EQ(log.level_of("non-finite"), SushiLog::LogLevel::Warn);
}

/**
 * @brief The pair whose cost is NaN stays unmatched: only the zero-cost pair (1, 1) under the 0.5
 *        threshold matches, and row 0 and column 0 are left over.
 */
TEST(Small_LAPJVSolverTest, NonFiniteCostPairStaysUnmatched)
{
    LAPJVSolver solver;
    CostMatrix cost;
    cost.resize(2, 2, 1.0f);
    cost(0, 0) = std::numeric_limits<float>::quiet_NaN();
    cost(1, 1) = 0.0f;
    std::vector<std::vector<int>> matches;
    std::vector<int> a_unm, b_unm;

    solver.solve(cost, 2, 2, 0.5f, matches, a_unm, b_unm);

    EXPECT_EQ(matches, (std::vector<std::vector<int>>{{1, 1}}));
    EXPECT_EQ(a_unm, std::vector<int>{0});
    EXPECT_EQ(b_unm, std::vector<int>{0});
}
