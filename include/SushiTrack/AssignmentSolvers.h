/**
 * @file      AssignmentSolvers.h
 * @brief     Linear assignment problem solvers implementation for object tracking.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      28.04.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */

#pragma once

#include "SushiTrack/IAssignmentSolver.h"
#include "SushiTrack/LAPJV.h"
#include <limits>
#include <vector>
#include <cstdint>

namespace SushiTrack
{
    /**
     * @brief Implementation of the Jonker-Volgenant (LAPJV) assignment algorithm.
     * 
     * Solves the linear assignment problem using the LAPJV algorithm with an 
     * integrated gating mechanism for matrix reduction. Reuses internal memory 
     * buffers to minimize heap allocations during repetitive execution.
     */
    class LAPJVSolver : public IAssignmentSolver
    {
        public:
            /**
             * @brief Solves the linear assignment problem for a given cost matrix.
             * 
             * @param cost Input matrix containing assignment costs.
             * @param n_rows Number of rows in the cost matrix.
             * @param n_cols Number of columns in the cost matrix.
             * @param thresh Maximum cost threshold for a valid assignment.
             * @param matches Output collection of row-to-column assignment pairs.
             * @param a_unmatched Output collection of unassigned row indices.
             * @param b_unmatched Output collection of unassigned column indices.
             */
            void solve(const CostMatrix& cost,
                    int n_rows, int n_cols, float thresh,
                    std::vector<std::vector<int>>& matches,
                    std::vector<int>& a_unmatched,
                    std::vector<int>& b_unmatched) override;

        private:
            /**
             * @brief Executes the core Jonker-Volgenant algorithm.
             * 
             * @param cost Cost matrix to be processed.
             * @param rowsol Output mapping from rows to columns.
             * @param colsol Output mapping from columns to rows.
             * @param extend_cost Flag to pad the matrix to a square representation.
             * @param cost_limit Threshold for considering costs as infinity.
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
                 * 
                 * @param n Required dimension for the buffers.
                 */
                void resize(size_t n) 
                {
                    _x_c.resize(n);
                    _y_c.resize(n);
                    if (_cost_data.size() < n * n)
                        _cost_data.resize(n * n);

                    _cost_ptr.resize(n);
                    for(size_t i = 0; i < n; ++i) 
                        _cost_ptr[i] = &_cost_data[i * n];

                    _jv_ws.resize(n);
                }

                /**
                 * @brief Allocates or resizes mapping buffers for matrix reduction.
                 * 
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