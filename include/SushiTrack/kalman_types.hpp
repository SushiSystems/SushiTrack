/**************************************************************************/
/* kalman_types.hpp                                                       */
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
 * @file kalman_types.hpp
 * @brief Defines vector and matrix types for Kalman filter operations.
 * @author Mustafa Garip
 */

#pragma once

#include <Eigen/Dense>
#include <vector>

namespace SushiTrack
{
    /**
     * @brief Common matrix types for Kalman filter operations.
     */
    struct KalmanTypes
    {
        using DetectBox = Eigen::Matrix<float, 1, 4, Eigen::RowMajor>; ///< 1x4 vector for observation coordinates.
        using StateMean = Eigen::Matrix<float, 1, 8, Eigen::RowMajor>; ///< 1x8 vector for state mean components.
        using StateCov  = Eigen::Matrix<float, 8, 8, Eigen::RowMajor>; ///< 8x8 matrix for state covariance.

        /** @brief Aligned vector for observation coordinates. */
        using DetectBoxVector = std::vector<DetectBox, Eigen::aligned_allocator<DetectBox>>;
    };

} // namespace SushiTrack
