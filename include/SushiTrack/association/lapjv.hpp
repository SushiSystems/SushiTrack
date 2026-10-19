/**************************************************************************/
/* lapjv.hpp                                                              */
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
/* Holds code ported from gatagat/lap (BSD-2-Clause), see NOTICE.md       */
/* Copyright (c) 2012-2025, Tomas Kazmar                                  */
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
