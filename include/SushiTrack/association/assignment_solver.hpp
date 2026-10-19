/**************************************************************************/
/* assignment_solver.hpp                                                  */
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
 * @file assignment_solver.hpp
 * @brief Interface definition for linear assignment problem solvers.
 * @author Mustafa Garip
 */

#pragma once

#include <vector>

#include <SushiTrack/association/cost_matrix.hpp>

namespace SushiTrack
{
    /**
     * @brief Interface for solving linear assignment problems.
     */
    class AssignmentSolver
    {
        public:
            /**
             * @brief Virtual destructor for the interface.
             */
            virtual ~AssignmentSolver() = default;

            /**
             * @brief Computes optimal assignments from a cost matrix.
             * @param cost Input matrix containing cost values.
             * @param thresh Cost threshold for assignment validity.
             */
            virtual void solve(const CostMatrix& __restrict cost, int n_rows, int n_cols,
                               float thresh, std::vector<std::vector<int>>& __restrict matches,
                               std::vector<int>& __restrict a_unmatched,
                               std::vector<int>& __restrict b_unmatched) = 0;
    };

} // namespace SushiTrack
