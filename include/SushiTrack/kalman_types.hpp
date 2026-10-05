/****************************************************************/
/* kalman_types.hpp                                             */
/* SushiTrack - https://github.com/SushiSystems/SushiTrack      */
/* Copyright (c) 2026 Sushi Systems                             */
/* Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.    */
/* Commercial use requires a licence from Sushi Systems.        */
/* Holds code ported from ByteTrack-cpp (MIT), see NOTICE.md    */
/* Copyright (c) 2022 Vertical Beach (lp6m, medalotte)          */
/* Copyright (c) 2021 Yifu Zhang                                */
/****************************************************************/

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
