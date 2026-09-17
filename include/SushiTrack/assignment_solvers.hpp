/**************************************************************************/
/* assignment_solvers.hpp                                                 */
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
 * @file assignment_solvers.hpp
 * @brief Linear assignment problem solvers for object tracking association.
 * @author Mustafa Garip
 */

#pragma once

#include <SushiTrack/cost_matrix.hpp>
#include <SushiTrack/i_assignment_solver.hpp>
#include <SushiTrack/lapjv.hpp>

#include <cstdint>
#include <limits>
#include <vector>

namespace SushiTrack
{
    /**
     * @brief Implementation of Jonker-Volgenant (LAPJV) assignment algorithm.
     */
    class LAPJVSolver : public IAssignmentSolver
    {
    public:
        /**
         * @brief Solves linear assignment problem for a given cost matrix.
         * @param cost Input matrix containing assignment costs.
         * @param thresh Maximum cost threshold for a valid assignment.
         */
        void solve(const CostMatrix& cost,
                   int n_rows, int n_cols, float thresh,
                   std::vector<std::vector<int>>& matches,
                   std::vector<int>& a_unmatched,
                   std::vector<int>& b_unmatched) override;

    private:
        /**
         * @brief Executes core Jonker-Volgenant algorithm.
         * @param cost Cost matrix to be processed.
         * @return Total assignment cost.
         */
        double execLAPJV(const CostMatrix& cost,
                         std::vector<int>& rowsol, std::vector<int>& colsol,
                         bool extend_cost = true,
                         float cost_limit = std::numeric_limits<float>::max());

        /**
         * @brief Internal memory workspace for algorithm execution.
         */
        struct InternalWorkspace
        {
            std::vector<double>  _cost_data;
            std::vector<double*> _cost_ptr;
            std::vector<int>     _x_c;
            std::vector<int>     _y_c;
            LAPJVWorkspace       _jv_ws;

            std::vector<uint8_t> _row_active;
            std::vector<uint8_t> _col_active;
            std::vector<uint8_t> _matched_cols;
            std::vector<int>     _sparse_to_real_row;
            std::vector<int>     _sparse_to_real_col;
            std::vector<int>     _sparse_rowsol;
            std::vector<int>     _sparse_colsol;
            CostMatrix           _sparse_cost;

            /**
             * @brief Allocates or resizes core algorithm buffers.
             * @param n Required dimension for the buffers.
             */
            void resize(size_t n)
            {
                _x_c.resize(n);
                _y_c.resize(n);
                if (_cost_data.size() < n * n)
                    _cost_data.resize(n * n);

                _cost_ptr.resize(n);
                for (size_t i = 0; i < n; ++i)
                    _cost_ptr[i] = &_cost_data[i * n];

                _jv_ws.resize(n);
            }

            /**
             * @brief Allocates or resizes mapping buffers for matrix reduction.
             * @param nr Required number of rows.
             * @param nc Required number of columns.
             */
            void resize_mappings(size_t nr, size_t nc)
            {
                if (_row_active.size() < nr) _row_active.resize(nr);
                if (_col_active.size() < nc) _col_active.resize(nc);
                if (_matched_cols.size() < nc) _matched_cols.resize(nc);
                if (_sparse_to_real_row.size() < nr) _sparse_to_real_row.resize(nr);
                if (_sparse_to_real_col.size() < nc) _sparse_to_real_col.resize(nc);
                if (_sparse_rowsol.size() < nr) _sparse_rowsol.resize(nr);
                if (_sparse_colsol.size() < nc) _sparse_colsol.resize(nc);
            }
        };

        InternalWorkspace ws_;
    };

} // namespace SushiTrack