/**************************************************************************/
/* lapjv_solver.hpp                                                       */
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
/*                                                                        */
/* Holds code ported from ByteTrack-cpp (MIT), see NOTICE.md              */
/* Copyright (c) 2022 Vertical Beach (lp6m, medalotte)                    */
/* Copyright (c) 2021 Yifu Zhang                                          */
/**************************************************************************/

/**
 * @file lapjv_solver.hpp
 * @brief Linear assignment problem solvers for object tracking association.
 * @author Mustafa Garip
 */

#pragma once

#include <cstdint>
#include <limits>
#include <vector>

#include <SushiTrack/association/assignment_solver.hpp>
#include <SushiTrack/association/cost_matrix.hpp>
#include <SushiTrack/association/lapjv.hpp>

namespace SushiTrack
{
    /**
     * @brief Implementation of Jonker-Volgenant (LAPJV) assignment algorithm.
     */
    class LAPJVSolver final : public AssignmentSolver
    {
        public:
            /**
             * @brief Solves linear assignment problem for a given cost matrix.
             * @param cost Input matrix containing assignment costs.
             * @param thresh Maximum cost threshold for a valid assignment.
             */
            void solve(const CostMatrix& cost, int n_rows, int n_cols, float thresh,
                       std::vector<std::vector<int>>& matches, std::vector<int>& a_unmatched,
                       std::vector<int>& b_unmatched) override;

        private:
            /**
             * @brief Executes core Jonker-Volgenant algorithm.
             * @param cost Cost matrix to be processed.
             */
            void exec_lapjv(const CostMatrix& cost, std::vector<int>& rowsol,
                            std::vector<int>& colsol, bool extend_cost = true,
                            float cost_limit = std::numeric_limits<float>::max());

            /**
             * @brief Internal memory workspace for algorithm execution.
             */
            struct InternalWorkspace
            {
                    std::vector<double> cost_data;
                    std::vector<double*> cost_ptr;
                    std::vector<int> x_c;
                    std::vector<int> y_c;
                    LAPJVWorkspace jv_ws;

                    std::vector<uint8_t> row_active;
                    std::vector<uint8_t> col_active;
                    std::vector<uint8_t> matched_cols;
                    std::vector<int> sparse_to_real_row;
                    std::vector<int> sparse_to_real_col;
                    std::vector<int> sparse_rowsol;
                    std::vector<int> sparse_colsol;
                    CostMatrix sparse_cost;

                    /**
                     * @brief Allocates or resizes core algorithm buffers.
                     * @param n Required dimension for the buffers.
                     */
                    void resize(size_t n)
                    {
                        x_c.resize(n);
                        y_c.resize(n);
                        if (cost_data.size() < n * n)
                            cost_data.resize(n * n);

                        cost_ptr.resize(n);
                        for (size_t i = 0; i < n; ++i)
                            cost_ptr[i] = &cost_data[i * n];

                        jv_ws.resize(n);
                    }

                    /**
                     * @brief Allocates or resizes mapping buffers for matrix reduction.
                     * @param nr Required number of rows.
                     * @param nc Required number of columns.
                     */
                    void resize_mappings(size_t nr, size_t nc)
                    {
                        if (row_active.size() < nr)
                            row_active.resize(nr);
                        if (col_active.size() < nc)
                            col_active.resize(nc);
                        if (matched_cols.size() < nc)
                            matched_cols.resize(nc);
                        if (sparse_to_real_row.size() < nr)
                            sparse_to_real_row.resize(nr);
                        if (sparse_to_real_col.size() < nc)
                            sparse_to_real_col.resize(nc);
                        if (sparse_rowsol.size() < nr)
                            sparse_rowsol.resize(nr);
                        if (sparse_colsol.size() < nc)
                            sparse_colsol.resize(nc);
                    }
            };

            InternalWorkspace ws_;
    };

} // namespace SushiTrack