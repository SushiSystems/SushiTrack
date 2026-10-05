/****************************************************************/
/* object.hpp                                                   */
/* SushiTrack - https://github.com/SushiSystems/SushiTrack      */
/* Copyright (c) 2026 Sushi Systems                             */
/* Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.    */
/* Commercial use requires a licence from Sushi Systems.        */
/* Holds code ported from ByteTrack-cpp (MIT), see NOTICE.md    */
/* Copyright (c) 2022 Vertical Beach (lp6m, medalotte)          */
/* Copyright (c) 2021 Yifu Zhang                                */
/****************************************************************/

/**
 * @file object.hpp
 * @brief Represents observed object detection data and visual features.
 * @author Mustafa Garip
 */

#pragma once

#include <SushiTrack/rect.hpp>

#include <vector>

namespace SushiTrack
{
    /**
     * @brief Data structure for object detection information.
     */
    struct alignas(16) Object
    {
        EIGEN_MAKE_ALIGNED_OPERATOR_NEW
        Rect<float> rect;           ///< Bounding box coordinates.
        int label;                  ///< Classification index.
        float prob;                 ///< Confidence probability score.
        std::vector<float> feature; ///< High-dimensional feature vector.

        /**
         * @brief Initializes an Object instance with detection data.
         * @param _rect Geometric bounding box of the detection.
         * @param _label Class identifier for the detected entity.
         * @param _prob Confidence probability score.
         * @param _feature Visual feature vector (optional).
         */
        Object(const Rect<float>& _rect,
               const int& _label,
               const float& _prob,
               std::vector<float> _feature = {});
    };

    /** @brief Aligned vector for Object instances. */
    using ObjectVector = std::vector<Object, Eigen::aligned_allocator<Object>>;

} // namespace SushiTrack
