/**************************************************************************/
/* i_assignment_solver.hpp                                                */
/**************************************************************************/
/*                          This file is part of:                         */
/*                               SushiTrack                               */
/*              https://github.com/SushiSystems/SushiTrack                */
/*                          https://sushisystems.io                       */
/**************************************************************************/
/* Copyright (c) 2026-present Mustafa Garip & Sushi Systems               */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License");        */
/* you may not use this file except in compliance with the License.       */
/* You may obtain a copy of the License at                                */
/*                                                                        */
/*     http://www.apache.org/licenses/LICENSE-2.0                         */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                          */
/**************************************************************************/

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
