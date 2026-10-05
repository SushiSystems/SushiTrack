/****************************************************************/
/* i_assignment_solver.hpp                                      */
/* SushiTrack - https://github.com/SushiSystems/SushiTrack      */
/* Copyright (c) 2026 Sushi Systems                             */
/* Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.    */
/* Commercial use requires a licence from Sushi Systems.        */
/****************************************************************/

/**
 * @file i_assignment_solver.hpp
 * @brief Interface definition for linear assignment problem solvers.
 * @author Mustafa Garip
 */

#pragma once

#include <SushiTrack/cost_matrix.hpp>

#include <vector>

namespace SushiTrack
{
    /**
     * @brief Interface for solving linear assignment problems.
     */
    class IAssignmentSolver
    {
    public:
        /**
         * @brief Virtual destructor for the interface.
         */
        virtual ~IAssignmentSolver() = default;

        /**
         * @brief Computes optimal assignments from a cost matrix.
         * @param cost Input matrix containing cost values.
         * @param thresh Cost threshold for assignment validity.
         */
        virtual void solve(const CostMatrix& __restrict cost,
                           int n_rows, int n_cols, float thresh,
                           std::vector<std::vector<int>>& __restrict matches,
                           std::vector<int>& __restrict a_unmatched,
                           std::vector<int>& __restrict b_unmatched) = 0;
    };

} // namespace SushiTrack
