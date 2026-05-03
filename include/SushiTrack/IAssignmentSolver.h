/**
 * @file      IAssignmentSolver.h
 * @brief     Interface definition for linear assignment problem solvers.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      28.04.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */

#pragma once

#include "SushiTrack/CostMatrix.h"
#include <vector>

namespace SushiTrack
{
    /**
     * @brief Interface for solving linear assignment problems.
     * 
     * Provides the functional requirements for algorithms performing bipartite 
     * matching between sets based on a provided cost matrix.
     */
    class IAssignmentSolver
    {
        public:
            virtual ~IAssignmentSolver() = default;

            /**
             * @brief Computes optimal assignments from a cost matrix.
             * 
             * @param cost The input matrix containing cost values.
             * @param n_rows Size of the first set.
             * @param n_cols Size of the second set.
             * @param thresh Cost threshold for assignment validity.
             * @param matches Output storage for assigned index pairs.
             * @param a_unmatched Output storage for unassigned row indices.
             * @param b_unmatched Output storage for unassigned column indices.
             */
            virtual void solve(const CostMatrix& __restrict cost,
                            int n_rows, int n_cols, float thresh,
                            std::vector<std::vector<int>>& __restrict matches,
                            std::vector<int>& __restrict a_unmatched,
                            std::vector<int>& __restrict b_unmatched) = 0;
    };

} // namespace SushiTrack
