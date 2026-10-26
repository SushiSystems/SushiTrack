/**************************************************************************/
/* lapjv_solver.cpp                                                       */
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
 * @file lapjv_solver.cpp
 * @brief Implements linear assignment problem solvers for object tracking.
 * @author Mustafa Garip
 */

#include <SushiTrack/association/lapjv_solver.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <vector>

#include <SushiTrack/association/association_log_category.hpp>
#include <SushiTrack/association/lapjv.hpp>
#include <SushiTrack/logging/log_throttle.hpp>
#include <SushiTrack/logging/logger.hpp>
#include <SushiTrack/profiling/profiler_keys.hpp>
#include <SushiTrack/profiling/profiling.hpp>

namespace SushiTrack
{
    namespace
    {
        /// @brief Returns whether @p value is NaN or infinite, read from its bits so -ffast-math
        ///        cannot fold the test away.
        bool is_non_finite(float value) noexcept
        {
            std::uint32_t bits = 0U;
            std::memcpy(&bits, &value, sizeof(bits));
            return (bits & 0x7F800000U) == 0x7F800000U;
        }
    } // namespace

    /**
     * @brief Solves the assignment problem for a given cost matrix and threshold.
     * @param cost Input cost matrix representing association weights.
     * @param thresh Cost threshold for qualifying an assignment.
     */
    void LAPJVSolver::solve(const CostMatrix& __restrict cost, int n_rows, int n_cols, float thresh,
                            std::vector<std::vector<int>>& __restrict matches,
                            std::vector<int>& __restrict a_unmatched,
                            std::vector<int>& __restrict b_unmatched)
    {
        SUSHITRACK_ZONE_FINE(ProfilerKeys::K_ZONE_SOLVE, ProfilerKeys::K_GROUP_ASSOCIATE);
        if (n_rows == 0 || n_cols == 0 || cost.empty())
        {
            if (n_rows > 0 && n_cols > 0)
            {
                SUSHITRACK_LOG_ERROR(K_ASSOCIATION_LOG_CATEGORY,
                                     "Cost matrix is empty for {} x {}; every row and column "
                                     "stays unmatched.",
                                     n_rows, n_cols);
            }
            for (int i = 0; i < n_rows; ++i)
            {
                a_unmatched.push_back(i);
            }
            for (int j = 0; j < n_cols; ++j)
            {
                b_unmatched.push_back(j);
            }
            return;
        }

        ws_.resize_mappings(n_rows, n_cols);

        std::fill(ws_.row_active.begin(), ws_.row_active.begin() + n_rows, 0);
        std::fill(ws_.col_active.begin(), ws_.col_active.begin() + n_cols, 0);

        int num_active_rows = 0;
        int num_active_cols = 0;
        int non_finite_costs = 0;

        for (int i = 0; i < n_rows; ++i)
        {
            for (int j = 0; j < n_cols; ++j)
            {
                if (cost(i, j) <= thresh)
                {
                    if (!ws_.row_active[i])
                    {
                        ws_.row_active[i] = 1;
                        num_active_rows++;
                    }
                    if (!ws_.col_active[j])
                    {
                        ws_.col_active[j] = 1;
                        num_active_cols++;
                    }
                }
                else if (is_non_finite(cost(i, j)))
                {
                    ++non_finite_costs;
                }
            }
        }

        if (non_finite_costs > 0)
        {
            static LogThrottle non_finite_throttle;
            SUSHITRACK_LOG_WARN_LIMITED(K_ASSOCIATION_LOG_CATEGORY, non_finite_throttle,
                                        "Cost matrix of {} x {} holds {} non-finite costs; those "
                                        "pairs stay unmatched.",
                                        n_rows, n_cols, non_finite_costs);
        }

        SUSHITRACK_LOG_TRACE(K_ASSOCIATION_LOG_CATEGORY, "Problem size: {} x {}, active: {} x {}",
                             n_rows, n_cols, num_active_rows, num_active_cols);

        if (num_active_rows == 0 || num_active_cols == 0)
        {
            for (int i = 0; i < n_rows; ++i)
            {
                a_unmatched.push_back(i);
            }
            for (int j = 0; j < n_cols; ++j)
            {
                b_unmatched.push_back(j);
            }
            return;
        }

        int r_idx = 0;
        for (int i = 0; i < n_rows; ++i)
        {
            if (ws_.row_active[i])
            {
                ws_.sparse_to_real_row[r_idx++] = i;
            }
            else
            {
                a_unmatched.push_back(i);
            }
        }

        int c_idx = 0;
        for (int j = 0; j < n_cols; ++j)
        {
            if (ws_.col_active[j])
            {
                ws_.sparse_to_real_col[c_idx++] = j;
            }
        }

        ws_.sparse_cost.resize(num_active_rows, num_active_cols, 1e4f);
        for (int sr = 0; sr < num_active_rows; ++sr)
        {
            int rr = ws_.sparse_to_real_row[sr];
            for (int sc = 0; sc < num_active_cols; ++sc)
            {
                int rc = ws_.sparse_to_real_col[sc];
                ws_.sparse_cost(sr, sc) = cost(rr, rc);
            }
        }

        exec_lapjv(ws_.sparse_cost, ws_.sparse_rowsol, ws_.sparse_colsol, true, thresh);

        std::fill(ws_.matched_cols.begin(), ws_.matched_cols.begin() + n_cols, 0);

        for (int sr = 0; sr < num_active_rows; ++sr)
        {
            int rr = ws_.sparse_to_real_row[sr];
            int sc = ws_.sparse_rowsol[sr];

            if (sc >= 0 && sc < num_active_cols)
            {
                int rc = ws_.sparse_to_real_col[sc];
                if (ws_.sparse_cost(sr, sc) <= thresh)
                {
                    matches.push_back({rr, rc});
                    ws_.matched_cols[rc] = 1;
                }
                else
                {
                    a_unmatched.push_back(rr);
                }
            }
            else
            {
                a_unmatched.push_back(rr);
            }
        }

        for (int j = 0; j < n_cols; ++j)
        {
            if (!ws_.matched_cols[j])
            {
                b_unmatched.push_back(j);
            }
        }

        SUSHITRACK_LOG_TRACE(K_ASSOCIATION_LOG_CATEGORY, "Assignments found: {}", matches.size());
    }

    /**
     * @brief Executes the Jonker-Volgenant algorithm on a cost matrix.
     * @param cost Input cost matrix.
     * @param extend_cost Flag indicating whether to pad matrix to square format.
     */
    void LAPJVSolver::exec_lapjv(const CostMatrix& cost, std::vector<int>& rowsol,
                                 std::vector<int>& colsol, bool extend_cost, float cost_limit)
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

            float fill_val =
                (cost_limit < std::numeric_limits<float>::max()) ? (cost_limit * 1.1f) : 1e6f;

            std::fill(ws_.cost_data.begin(), ws_.cost_data.begin() + (static_cast<size_t>(n) * n),
                      static_cast<double>(fill_val));

            for (int i = nr; i < n; ++i)
            {
                for (int j = nc; j < n; ++j)
                {
                    ws_.cost_ptr[i][j] = 0.0;
                }
            }
            for (int i = 0; i < nr; ++i)
            {
                for (int j = 0; j < nc; ++j)
                {
                    ws_.cost_ptr[i][j] = static_cast<double>(cost(i, j));
                }
            }
        }
        else
        {
            if (nr != nc)
            {
                SUSHITRACK_THROW("LAPJV requires square matrix if not extending (nr={}, nc={})", nr,
                                 nc);
            }

            n = nr;
            ws_.resize(n);
            for (int i = 0; i < n; ++i)
            {
                for (int j = 0; j < n; ++j)
                {
                    ws_.cost_ptr[i][j] = static_cast<double>(cost(i, j));
                }
            }
        }

        int result = lapjv_internal(static_cast<size_t>(n), ws_.cost_ptr.data(), ws_.x_c.data(),
                                    ws_.y_c.data(), &ws_.jv_ws);

        if (result != 0)
        {
            SUSHITRACK_THROW(
                "LAPJV assignment failed - cost matrix may be numerically unstable (result={})",
                result);
        }

        for (int i = 0; i < nr; ++i)
        {
            int j = ws_.x_c[i];
            if (j < nc)
            {
                rowsol[i] = j;
            }
        }
        for (int j = 0; j < nc; ++j)
        {
            int i = ws_.y_c[j];
            if (i < nr)
            {
                colsol[j] = i;
            }
        }
    }

} // namespace SushiTrack
