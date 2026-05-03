/**
 * @file      Object.cpp
 * @brief     Implementation of the Object data structure for detections.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      28.04.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */

#include "SushiTrack/Object.h"

namespace SushiTrack
{
    /**
     * @brief Constructs an Object instance representing a raw detection.
     * 
     * @param _rect Bounding box of the detection.
     * @param _label Class identifier for the detected object.
     * @param _prob Confidence score of the detection.
     * @param _feature Visual feature vector (optional).
     */
    Object::Object(const Rect<float> &_rect,
                   const int &_label,
                   const float &_prob,
                   std::vector<float> _feature) 
        : rect(_rect), label(_label), prob(_prob), feature(std::move(_feature))
    { }

} // namespace SushiTrack
