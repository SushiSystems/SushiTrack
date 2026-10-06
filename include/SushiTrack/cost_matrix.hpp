/**************************************************************************/
/* cost_matrix.hpp                                                        */
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
/**************************************************************************/

/**
 * @file cost_matrix.hpp
 * @brief Provides 2D matrix storage for bipartite assignment cost calculations.
 * @author Mustafa Garip
 */

#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <vector>

namespace SushiTrack
{
    /**
     * @brief Contiguous 2D matrix storage with reallocatable buffer.
     */
    class CostMatrix
    {
        public:
            /**
             * @brief Initializes an empty cost matrix with zero dimensions.
             */
            CostMatrix() : rows_(0), cols_(0) {}

            /**
             * @brief Updates matrix dimensions and initializes elements.
             * @param rows Row dimension.
             * @param cols Column dimension.
             * @param initial_val Value assigned to all elements.
             */
            void resize(std::size_t rows, std::size_t cols, float initial_val = 1e4f)
            {
                rows_ = rows;
                cols_ = cols;
                std::size_t total = rows * cols;

                if (data_.size() < total)
                    data_.assign(total, initial_val);
                else
                    std::fill(data_.begin(), data_.begin() + total, initial_val);
            }

            /**
             * @brief Accesses element at specified indices.
             * @param r Row index.
             * @param c Column index.
             * @return Reference to the element.
             */
            float& operator()(std::size_t r, std::size_t c)
            {
                assert(r < rows_ && c < cols_);
                return data_[r * cols_ + c];
            }

            /**
             * @brief Accesses element at specified indices.
             * @param r Row index.
             * @param c Column index.
             * @return Constant reference to the element.
             */
            const float& operator()(std::size_t r, std::size_t c) const
            {
                assert(r < rows_ && c < cols_);
                return data_[r * cols_ + c];
            }

            std::size_t rows() const    { return rows_; }        ///< Row count.
            std::size_t cols() const    { return cols_; }        ///< Column count.
            float* data()               { return data_.data(); } ///< Pointer to internal buffer.
            const float* data() const   { return data_.data(); } ///< Constant pointer to internal buffer.

            /**
             * @brief Checks if the matrix has zero dimensions.
             * @return True if either rows or columns is zero.
             */
            bool empty() const          { return rows_ == 0 || cols_ == 0; }

        private:
            std::vector<float> data_;
            std::size_t rows_;
            std::size_t cols_;
    };

} // namespace SushiTrack
