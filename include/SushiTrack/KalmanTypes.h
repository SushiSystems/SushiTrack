/**
 * @file      KalmanTypes.h
 * @brief     Common matrix and vector types for Kalman filter operations.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      28.04.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */

#pragma once

#include <vector>
#include <Eigen/Dense>

namespace SushiTrack
{

    /**
    * @brief Common matrix types for Kalman filter operations.
    */
    struct KalmanTypes
    {
        using DetectBox = Eigen::Matrix<float, 1, 4, Eigen::RowMajor>; ///< 1x4 vector for observation coordinates ([x, y, w, h] or [cx, cy, a, h]).
        using StateMean = Eigen::Matrix<float, 1, 8, Eigen::RowMajor>; ///< 1x8 vector for state mean components ([x, y, w, h, vx, vy, vw, vh] or [cx, cy, a, h, vcx, vcy, va, vh]).
        using StateCov  = Eigen::Matrix<float, 8, 8, Eigen::RowMajor>; ///< 8x8 matrix for state covariance distribution.

        /** @brief Aligned vector for observation coordinates. */
        using DetectBoxVector = std::vector<DetectBox, Eigen::aligned_allocator<DetectBox>>;
    };

} // namespace SushiTrack
