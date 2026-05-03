/**
 * @file      Object.h
 * @brief     Data structure for representing object detections.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      28.04.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */

#pragma once

#include "SushiTrack/Rect.h"
#include <vector>

namespace SushiTrack
{
    /**
     * @brief Data structure for object detection information.
     * 
     * Encapsulates spatial coordinates, classification labels, confidence 
     * probabilities, and visual feature vectors for a single observation.
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
         * 
         * @param _rect Geometric bounding box of the detection.
         * @param _label Class identifier for the detected entity.
         * @param _prob Confidence probability score.
         * @param _feature Visual feature vector (optional).
         */
        Object(const Rect<float> &_rect,
            const int &_label,
            const float &_prob,
            std::vector<float> _feature = {});
    };
    
    /** @brief Aligned vector for Object instances to prevent memory alignment issues. */
    using ObjectVector = std::vector<Object, Eigen::aligned_allocator<Object>>;

} // namespace SushiTrack
