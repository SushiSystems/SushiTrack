/**************************************************************************/
/* lapjv.hpp                                                              */
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
 * @file lapjv.hpp
 * @brief Implements Jonker-Volgenant linear assignment algorithm.
 * @author Mustafa Garip
 */

#pragma once

#include <cstddef>
#include <vector>

namespace SushiTrack
{
    /**
     * @brief Workspace management structure for the LAPJV solver.
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
     * @brief Solves linear assignment problem using Jonker-Volgenant algorithm.
     * @param ws Calculation workspace buffer; must not be null.
     * @return Zero on success, non-zero on error.
     */
    int lapjv_internal(const size_t n, double* __restrict cost[], int* __restrict x, int* __restrict y, LAPJVWorkspace* __restrict ws);

} // namespace SushiTrack
