/**************************************************************************/
/* lapjv.cpp                                                              */
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
 * @file lapjv.cpp
 * @brief Implements the Jonker-Volgenant linear assignment algorithm.
 * @author Mustafa Garip
 */

#include <SushiTrack/lapjv.hpp>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <vector>

#include <SushiTrack/logger.hpp>

namespace SushiTrack
{
    namespace
    {
        constexpr size_t LARGE = 1000000;

        /**
         * @brief Performs column-reduction and reduction transfer for a dense cost matrix.
         * @return Number of free rows after reduction.
         */
        int _ccrrt_dense(const size_t n, double* __restrict cost[],
            int* __restrict free_rows, int* __restrict x, int* __restrict y, double* __restrict v)
        {
            int n_free_rows;

            for (size_t i = 0; i < n; i++) 
            {
                x[i] = -1;
                v[i] = LARGE;
                y[i] = 0;
            }
            for (size_t i = 0; i < n; i++) 
            {
                for (size_t j = 0; j < n; j++) 
                {
                    const double c = cost[i][j];
                    if (c < v[j]) {
                        v[j] = c;
                        y[j] = (int)i;
                    }
                }
            }

            std::vector<bool> unique(n, true);
            {
                int j = (int)n;
                do 
                {
                    j--;
                    const int i = y[j];
                    if (x[i] < 0) 
                    {
                        x[i] = j;
                    }
                    else 
                    {
                        unique[i] = false;
                        y[j] = -1;
                    }
                } 
                while (j > 0);
            }
            n_free_rows = 0;
            for (size_t i = 0; i < n; i++) 
            {
                if (x[i] < 0) 
                {
                    free_rows[n_free_rows++] = (int)i;
                }
                else if (unique[i]) 
                {
                    const int j = x[i];
                    double min = LARGE;
                    for (size_t j2 = 0; j2 < n; j2++) 
                    {
                        if (j2 == (size_t)j) 
                        {
                            continue;
                        }
                        const double c = cost[i][j2] - v[j2];
                        if (c < min) 
                        {
                            min = c;
                        }
                    }
                    v[j] -= min;
                }
            }
            return n_free_rows;
        }

        /**
         * @brief Performs augmenting row reduction for a dense cost matrix.
         * @return Number of free rows remaining after augmentation.
         */
        int _carr_dense(
            const size_t n, double* __restrict cost[],
            const size_t n_free_rows,
            int* __restrict free_rows, int* __restrict x, int* __restrict y, double* __restrict v)
        {
            size_t current = 0;
            int new_free_rows = 0;
            size_t rr_cnt = 0;
            while (current < n_free_rows) {
                int i0;
                int j1, j2;
                double v1, v2, v1_new;
                bool v1_lowers;

                rr_cnt++;
                const int free_i = free_rows[current++];
                j1 = 0;
                v1 = cost[free_i][0] - v[0];
                j2 = -1;
                v2 = LARGE;
                for (size_t j = 1; j < n; j++) {
                    const double c = cost[free_i][j] - v[j];
                    if (c < v2) {
                        if (c >= v1) {
                            v2 = c;
                            j2 = (int)j;
                        }
                        else {
                            v2 = v1;
                            v1 = c;
                            j2 = j1;
                            j1 = (int)j;
                        }
                    }
                }
                i0 = y[j1];
                v1_new = v[j1] - (v2 - v1);
                v1_lowers = v1_new < v[j1];
                if (rr_cnt < current * n) {
                    if (v1_lowers) {
                        v[j1] = v1_new;
                    }
                    else if (i0 >= 0 && j2 >= 0) {
                        j1 = j2;
                        i0 = y[j2];
                    }
                    if (i0 >= 0) {
                        if (v1_lowers) {
                            free_rows[--current] = i0;
                        }
                        else {
                            free_rows[new_free_rows++] = i0;
                        }
                    }
                }
                else {
                    if (i0 >= 0) {
                        free_rows[new_free_rows++] = i0;
                    }
                }
                x[free_i] = j1;
                y[j1] = free_i;
            }
            return new_free_rows;
        }

        /**
         * @brief Identifies columns with minimum dual costs and updates the scan list.
         * @return Index of the first column beyond the identified minimums.
         */
        size_t _find_dense(const size_t n, size_t lo, double* __restrict d, int* __restrict cols, int* __restrict y)
        {
            size_t hi = lo + 1;
            double mind = d[cols[lo]];
            for (size_t k = hi; k < n; k++) {
                int j = cols[k];
                if (d[j] <= mind) {
                    if (d[j] < mind) {
                        hi = lo;
                        mind = d[j];
                    }
                    std::swap(cols[k], cols[hi]);
                    hi++;
                }
            }
            return hi;
        }

        /**
         * @brief Scans columns to update dual distances and find augmenting paths.
         * @return Index of the unassigned column found, or -1 if none identified.
         */
        int _scan_dense(const size_t n, double* __restrict cost[],
            size_t* __restrict plo, size_t* __restrict phi,
            double* __restrict d, int* __restrict cols, int* __restrict pred,
            int* __restrict y, double* __restrict v)
        {
            size_t lo = *plo;
            size_t hi = *phi;
            double h, cred_ij;

            while (lo != hi) {
                int j = cols[lo++];
                const int i = y[j];
                if (i < 0) continue;
                const double mind = d[j];
                h = cost[i][j] - v[j] - mind;
                for (size_t k = hi; k < n; k++) {
                    int jj = cols[k];
                    cred_ij = cost[i][jj] - v[jj] - h;
                    if (cred_ij < d[jj]) {
                        d[jj] = cred_ij;
                        pred[jj] = i;
                        if (cred_ij == mind) {
                            if (y[jj] < 0) {
                                return jj;
                            }
                            std::swap(cols[k], cols[hi]);
                            hi++;
                        }
                    }
                }
            }
            *plo = lo;
            *phi = hi;
            return -1;
        }

        /**
         * @brief Executes a single iteration of the shortest path search.
         * @return Index of the unassigned column terminating the shortest path.
         */
        int find_path_dense(
            const size_t n, double* __restrict cost[],
            const int start_i,
            int* __restrict y, double* __restrict v,
            int* __restrict pred, int* __restrict cols, double* __restrict d)
        {
            size_t lo = 0, hi = 0;
            int final_j = -1;
            size_t n_ready = 0;

            for (size_t i = 0; i < n; i++) {
                cols[i] = (int)i;
                pred[i] = start_i;
                d[i] = cost[start_i][i] - v[i];
            }

            while (final_j == -1) {
                if (lo == hi) {
                    n_ready = lo;
                    hi = _find_dense(n, lo, d, cols, y);
                    for (size_t k = lo; k < hi; k++) {
                        const int j = cols[k];
                        if (y[j] < 0) {
                            final_j = j;
                        }
                    }
                }
                if (final_j == -1) {
                    final_j = _scan_dense(
                        n, cost, &lo, &hi, d, cols, pred, y, v);
                }
            }

            {
                const double mind = d[cols[lo]];
                for (size_t k = 0; k < n_ready; k++) {
                    const int j = cols[k];
                    v[j] += d[j] - mind;
                }
            }

            return final_j;
        }

        /**
         * @brief Performs augmentation using identified shortest paths.
         * @return Zero on successful completion.
         */
        int _ca_dense(
            const size_t n, double* __restrict cost[],
            const size_t n_free_rows,
            int* __restrict free_rows, int* __restrict x, int* __restrict y, double* __restrict v, LAPJVWorkspace& ws)
        {
            int* __restrict pred = ws.pred.data();
            int* __restrict cols = ws.cols.data();
            double* __restrict d = ws.d.data();

            for (int* pfree_i = free_rows; pfree_i < free_rows + n_free_rows; pfree_i++) {
                int i = -1, j;
                size_t k = 0;

                j = find_path_dense(n, cost, *pfree_i, y, v, pred, cols, d);
                if (j < 0)
                {
                    SUSHITRACK_RUNTIME_ERROR("[LAPJV] Error occurred in _ca_dense(): j < 0");
                }
                if (j >= static_cast<int>(n))
                {
                    SUSHITRACK_RUNTIME_ERROR("[LAPJV] Error occurred in _ca_dense(): j >= n (j=%d, n=%zu)", j, n);
                }
                while (i != *pfree_i) {
                    i = pred[j];
                    y[j] = i;
                    std::swap(j, x[i]);
                    k++;
                    if (k >= n) {
                        SUSHITRACK_RUNTIME_ERROR("[LAPJV] Error occurred in _ca_dense(): k >= n (k=%zu, n=%zu)", k, n);
                    }
                }
            }
            return 0;
        }

    } // namespace

    /**
     * @brief Solves a dense linear assignment problem using the Jonker-Volgenant algorithm.
     * @return Zero on success, or non-zero status code on failure.
     */
    int lapjv_internal(
        const size_t n, double* __restrict cost[],
        int* __restrict x, int* __restrict y, LAPJVWorkspace* __restrict ws)
    {
        if (n == 0) return 0;
        if (!ws) throw std::invalid_argument("LAPJVWorkspace is null");

        ws->resize(n);

        int* free_rows = ws->free_rows.data();
        double* v = ws->v.data();

        int ret = _ccrrt_dense(n, cost, free_rows, x, y, v);
        int i = 0;
        while (ret > 0 && i < 2) {
            ret = _carr_dense(n, cost, (size_t)ret, free_rows, x, y, v);
            i++;
        }
        if (ret > 0) {
            ret = _ca_dense(n, cost, (size_t)ret, free_rows, x, y, v, *ws);
        }
        return ret;
    }

} // namespace SushiTrack
