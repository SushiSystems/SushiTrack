/**
 * @file      lapjv.h
 * @brief     Jonker-Volgenant (LAPJV) algorithm implementation for linear assignment.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      28.04.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */

#pragma once

#include <cstddef>
#include <vector>

namespace SushiTrack
{
    /**
     * @brief Workspace management structure for the LAPJV solver.
     * 
     * Maintains pre-allocated memory buffers for internal Jonker-Volgenant 
     * calculations to minimize heap reallocations during recurring 
     * association cycles.
     */
    struct LAPJVWorkspace
    {
        std::vector<int>    free_rows; ///< Row indices available for assignment.
        std::vector<double> v;         ///< Dual variables for columns.
        std::vector<int>    cols;      ///< Current column assignments.
        std::vector<double> d;         ///< Minimum reduced costs.
        std::vector<int>    pred;      ///< Predecessor indices for augmenting paths.

        /**
         * @brief Updates buffer sizes to accommodate specified element counts.
         * 
         * @param n Required element capacity.
         */
        void resize(size_t n) 
        {
            if (free_rows.size() < n) 
            {
                free_rows.resize(n);
                v.resize(n);
                cols.resize(n);
                d.resize(n);
                pred.resize(n);
            }
        }
    };

    /**
     * @brief Low-level implementation of the linear assignment problem solver.
     * 
     * Calculates the optimal bipartite matching by minimizing the aggregate 
     * association cost across the provided matrix.
     * 
     * @param n Square matrix dimension.
     * @param cost Pointer to the 2D cost buffer.
     * @param x Output assignment for row indices.
     * @param y Output assignment for column indices.
     * @param ws Pointer to the calculation workspace.
     * @return Zero on success, or non-zero status code on failure.
     */
    int lapjv_internal(const size_t n, double * __restrict cost[], int * __restrict x, int * __restrict y, LAPJVWorkspace* __restrict ws);

} // namespace SushiTrack
