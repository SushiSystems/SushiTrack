/**************************************************************************/
/* TestAssignmentSolvers.cpp                                              */
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
 * @file TestAssignmentSolvers.cpp
 * @brief Unit tests for assignment solver implementations.
 * @author Mustafa Garip
 */

#include <gtest/gtest.h>
#include <SushiTrack/association/lapjv_solver.hpp>
#include <SushiTrack/association/cost_matrix.hpp>

#include <algorithm>
#include <vector>

using namespace SushiTrack;

// Helpers

/**
 * @brief Constructs an n×n identity-like cost matrix:
 *        diagonal = 0 (perfect match), off-diagonal = 1 (no match).
 */
static CostMatrix makeIdentityCost(int n)
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
TEST(LAPJVSolverTest, EmptyMatrixAllUnmatched)
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
TEST(LAPJVSolverTest, OneByOneMatchBelow)
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
TEST(LAPJVSolverTest, OneByOneMatchAboveThreshold)
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
TEST(LAPJVSolverTest, IdentityMatrixPerfectDiagonalMatch)
{
    const int n = 4;
    LAPJVSolver solver;
    CostMatrix cost = makeIdentityCost(n);

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
 * @brief In a 4×2 identity-like matrix, rows 2 and 3 are unmatched;
 *        rows 0 and 1 match their diagonal column.
 */
TEST(LAPJVSolverTest, MoreRowsThanColsSurplusRowsUnmatched)
{
    LAPJVSolver solver;
    CostMatrix cost;
    cost.resize(4, 2, 1.0f);
    cost(0, 0) = 0.0f;
    cost(1, 1) = 0.0f;

    std::vector<std::vector<int>> matches;
    std::vector<int> a_unm, b_unm;
    solver.solve(cost, 4, 2, 0.5f, matches, a_unm, b_unm);

    EXPECT_EQ(matches.size(), 2u);
    EXPECT_EQ(a_unm.size(), 2u);
    EXPECT_TRUE(b_unm.empty());

    // Both columns must be matched.
    std::vector<int> matched_cols;
    for (const auto& p : matches) matched_cols.push_back(p[1]);
    std::sort(matched_cols.begin(), matched_cols.end());
    EXPECT_EQ(matched_cols, (std::vector<int>{0, 1}));
}

// Rectangular: more columns than rows

/**
 * @brief In a 2×4 identity-like matrix, columns 2 and 3 are unmatched;
 *        rows 0 and 1 match their diagonal column.
 */
TEST(LAPJVSolverTest, MoreColsThanRowsSurplusColsUnmatched)
{
    LAPJVSolver solver;
    CostMatrix cost;
    cost.resize(2, 4, 1.0f);
    cost(0, 0) = 0.0f;
    cost(1, 1) = 0.0f;

    std::vector<std::vector<int>> matches;
    std::vector<int> a_unm, b_unm;
    solver.solve(cost, 2, 4, 0.5f, matches, a_unm, b_unm);

    EXPECT_EQ(matches.size(), 2u);
    EXPECT_TRUE(a_unm.empty());
    EXPECT_EQ(b_unm.size(), 2u);

    // Matched columns must be 0 and 1.
    std::vector<int> matched_cols;
    for (const auto& p : matches) matched_cols.push_back(p[1]);
    std::sort(matched_cols.begin(), matched_cols.end());
    EXPECT_EQ(matched_cols, (std::vector<int>{0, 1}));
}

// Threshold sensitivity

/**
 * @brief Tightening the threshold from 0.9 to 0.3 suppresses matches whose
 *        cost exceeds the new limit.
 */
TEST(LAPJVSolverTest, ThresholdControlsAcceptedAssignments)
{
    LAPJVSolver solver;
    CostMatrix cost;
    cost.resize(2, 2);
    cost(0, 0) = 0.2f;  // good match
    cost(0, 1) = 0.9f;
    cost(1, 0) = 0.9f;
    cost(1, 1) = 0.7f;  // borderline match

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

/**
 * @brief Each row and each column appears at most once across all returned
 *        match pairs (bijectivity constraint).
 */
TEST(LAPJVSolverTest, NoDoubleAssignments)
{
    const int n = 5;
    LAPJVSolver solver;
    CostMatrix cost = makeIdentityCost(n);

    std::vector<std::vector<int>> matches;
    std::vector<int> a_unm, b_unm;
    solver.solve(cost, n, n, 0.5f, matches, a_unm, b_unm);

    std::vector<bool> rows_seen(n, false), cols_seen(n, false);
    for (const auto& p : matches)
    {
        EXPECT_FALSE(rows_seen[p[0]]) << "row " << p[0] << " assigned twice";
        EXPECT_FALSE(cols_seen[p[1]]) << "col " << p[1] << " assigned twice";
        rows_seen[p[0]] = true;
        cols_seen[p[1]] = true;
    }
}

// Repeated calls

/**
 * @brief Two consecutive calls on the same solver instance produce identical
 *        results (internal workspace does not corrupt state).
 */
TEST(LAPJVSolverTest, RepeatedCallsProduceConsistentResults)
{
    LAPJVSolver solver;
    CostMatrix cost = makeIdentityCost(3);

    auto run = [&]() {
        std::vector<std::vector<int>> matches;
        std::vector<int> a_unm, b_unm;
        solver.solve(cost, 3, 3, 0.5f, matches, a_unm, b_unm);
        return matches.size();
    };

    std::size_t first  = run();
    std::size_t second = run();
    EXPECT_EQ(first, second);
    EXPECT_EQ(first, 3u);
}

// Tier 3 edge cases

/**
 * @brief A matrix where every cell equals the sentinel value (1e4f) produces
 *        no matches; all rows and columns are reported as unmatched.
 */
TEST(LAPJVSolverTest, AllSentinelMatrixProducesNoMatches)
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
TEST(LAPJVSolverTest, OneRowAllAboveThresholdNoMatch)
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
TEST(LAPJVSolverTest, PermutationMatrixMatchesOptimally)
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
