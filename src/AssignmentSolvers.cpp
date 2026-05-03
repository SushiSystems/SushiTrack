/**
 * @file      AssignmentSolvers.cpp
 * @brief     Implementation of linear assignment problem solvers for object tracking.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      28.04.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */

#include "SushiTrack/AssignmentSolvers.h"
#include <algorithm>
#include "SushiTrack/Logger.h"
#include <stdexcept>

namespace SushiTrack
{
    /**
     * @brief Solves the assignment problem for a given cost matrix and threshold.
     * 
     * Identifies active rows and columns that contain at least one cost value 
     * below the specified threshold. Constructs a reduced sub-matrix containing 
     * only these active elements, solves the assignment problem on this 
     * sub-matrix, and maps the results back to the original indices.
     * 
     * @param cost Input cost matrix representing association weights.
     * @param n_rows Total number of source entities (rows).
     * @param n_cols Total number of candidate entities (columns).
     * @param thresh Cost threshold for qualifying an assignment.
     * @param matches Output collection of matched index pairs.
     * @param a_unmatched Output collection of unmatched row indices.
     * @param b_unmatched Output collection of unmatched column indices.
     */
    void LAPJVSolver::solve(const CostMatrix& __restrict cost,
                            int n_rows, int n_cols, float thresh,
                            std::vector<std::vector<int>>& __restrict matches,
                            std::vector<int>& __restrict a_unmatched,
                            std::vector<int>& __restrict b_unmatched)
    {
        if (n_rows == 0 || n_cols == 0 || cost.empty())
        {
            for (int i = 0; i < n_rows; ++i) a_unmatched.push_back(i);
            for (int j = 0; j < n_cols; ++j) b_unmatched.push_back(j);
            return;
        }

        ws_.resize_mappings(n_rows, n_cols);

        std::fill(ws_._row_active.begin(), ws_._row_active.begin() + n_rows, 0);
        std::fill(ws_._col_active.begin(), ws_._col_active.begin() + n_cols, 0);

        int num_active_rows = 0;
        int num_active_cols = 0;

        for (int i = 0; i < n_rows; ++i) 
        {
            for (int j = 0; j < n_cols; ++j) 
            {
                if (cost(i, j) <= thresh) {

                    if (!ws_._row_active[i]) { ws_._row_active[i] = 1; num_active_rows++; }
                    if (!ws_._col_active[j]) { ws_._col_active[j] = 1; num_active_cols++; }
                }
            }
        }

        SUSHITRACK_LOG_DEBUG("[Solver] Problem size: %d x %d, active: %d x %d", n_rows, n_cols, num_active_rows, num_active_cols);

        if (num_active_rows == 0 || num_active_cols == 0) 
        {
            for (int i = 0; i < n_rows; ++i) a_unmatched.push_back(i);
            for (int j = 0; j < n_cols; ++j) b_unmatched.push_back(j);
            return;
        }

        int r_idx = 0;
        for (int i = 0; i < n_rows; ++i) 
        {
            if (ws_._row_active[i]) 
                ws_._sparse_to_real_row[r_idx++] = i; 
            else 
                a_unmatched.push_back(i);
        }

        int c_idx = 0;
        for (int j = 0; j < n_cols; ++j) 
        {
            if (ws_._col_active[j])
                ws_._sparse_to_real_col[c_idx++] = j; 
        }

        ws_._sparse_cost.resize(num_active_rows, num_active_cols, 1e4f);
        for (int sr = 0; sr < num_active_rows; ++sr) 
        {
            int rr = ws_._sparse_to_real_row[sr];
            for (int sc = 0; sc < num_active_cols; ++sc) 
            {
                int rc = ws_._sparse_to_real_col[sc];
                ws_._sparse_cost(sr, sc) = cost(rr, rc);
            }
        }

        execLAPJV(ws_._sparse_cost, ws_._sparse_rowsol, ws_._sparse_colsol, true, thresh);

        std::fill(ws_._matched_cols.begin(), ws_._matched_cols.begin() + n_cols, 0);

        for (int sr = 0; sr < num_active_rows; ++sr) 
        {
            int rr = ws_._sparse_to_real_row[sr];
            int sc = ws_._sparse_rowsol[sr];

            if (sc >= 0 && sc < num_active_cols) 
            {
                int rc = ws_._sparse_to_real_col[sc];
                if (ws_._sparse_cost(sr, sc) <= thresh) 
                {
                    matches.push_back({rr, rc});
                    ws_._matched_cols[rc] = 1;
                } 
                else
                    a_unmatched.push_back(rr);
            }
            else 
                a_unmatched.push_back(rr);
        }

        for (int j = 0; j < n_cols; ++j) 
        {
            if (!ws_._matched_cols[j])
                b_unmatched.push_back(j);
        }

        SUSHITRACK_LOG_DEBUG("[Solver] Assignments found: %zu", matches.size());
    }

    /**
     * @brief Executes the Jonker-Volgenant algorithm on a cost matrix.
     * 
     * Pads the input matrix to a square format if required. Handles rectangular 
     * matrices or matching limits using high-cost augmentation. Utilizes 
     * pre-allocated memory buffers in the workspace.
     * 
     * @param cost Input cost matrix.
     * @param rowsol Output assignment of rows to columns.
     * @param colsol Output assignment of columns to rows.
     * @param extend_cost Flag indicating whether to pad the matrix to a square format.
     * @param cost_limit Threshold for limiting maximum assignment cost.
     * @return Total optimized assignment cost.
     */
    double LAPJVSolver::execLAPJV(const CostMatrix& cost,
                                std::vector<int>& rowsol, std::vector<int>& colsol,
                                bool extend_cost, float cost_limit)
    {
        int nr = static_cast<int>(cost.rows());
        int nc = static_cast<int>(cost.cols());
        
        rowsol.assign(nr, -1);
        colsol.assign(nc, -1);

        int n = 0;
        if (extend_cost || cost_limit < std::numeric_limits<float>::max())
        {
            n = nr + nc;
            ws_.resize(n);

            float fill_val = (cost_limit < std::numeric_limits<float>::max()) ? (cost_limit * 1.1f) : 1e6f;
            
            std::fill(ws_._cost_data.begin(), ws_._cost_data.begin() + (static_cast<size_t>(n) * n), static_cast<double>(fill_val));
            
            for (int i = nr; i < n; ++i)
            {
                for (int j = nc; j < n; ++j) ws_._cost_ptr[i][j] = 0.0;
            }
            for (int i = 0; i < nr; ++i)
            {
                for (int j = 0; j < nc; ++j)
                    ws_._cost_ptr[i][j] = static_cast<double>(cost(i, j));
            }
        }
        else
        {
            if (nr != nc) {
                SUSHITRACK_RUNTIME_ERROR("[Solver] LAPJV requires square matrix if not extending (nr=%d, nc=%d)", nr, nc);
            }
           
            n = nr;
            ws_.resize(n);
            for (int i = 0; i < n; ++i)
            {
                for (int j = 0; j < n; ++j)
                    ws_._cost_ptr[i][j] = static_cast<double>(cost(i, j));
            }
        }

        int result = lapjv_internal(static_cast<size_t>(n),
                                    ws_._cost_ptr.data(),
                                    ws_._x_c.data(),
                                    ws_._y_c.data(),
                                    &ws_._jv_ws);
        
        if (result != 0)
        {
            SUSHITRACK_RUNTIME_ERROR("[Solver] LAPJV assignment failed - cost matrix may be numerically unstable (result=%d)", result);
        }

        for (int i = 0; i < nr; ++i)
        {
            int j = ws_._x_c[i];
            if (j < nc) rowsol[i] = j;
        }
        for (int j = 0; j < nc; ++j)
        {
            int i = ws_._y_c[j];
            if (i < nr) colsol[j] = i;
        }

        double opt = 0.0;
        for (int i = 0; i < nr; ++i)
        {
            if (rowsol[i] != -1) opt += static_cast<double>(cost(i, rowsol[i]));
        }
        return opt;
    }

} // namespace SushiTrack
