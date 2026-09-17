/**************************************************************************/
/* object.hpp                                                             */
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
