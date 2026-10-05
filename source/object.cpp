/****************************************************************/
/* object.cpp                                                   */
/* SushiTrack - https://github.com/SushiSystems/SushiTrack      */
/* Copyright (c) 2026 Sushi Systems                             */
/* Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.    */
/* Commercial use requires a licence from Sushi Systems.        */
/* Holds code ported from ByteTrack-cpp (MIT), see NOTICE.md    */
/* Copyright (c) 2022 Vertical Beach (lp6m, medalotte)          */
/* Copyright (c) 2021 Yifu Zhang                                */
/****************************************************************/

/**
 * @file object.cpp
 * @brief Implements object detection data structure constructors.
 * @author Mustafa Garip
 */

#include <SushiTrack/object.hpp>

#include <utility>

namespace SushiTrack
{
    /**
     * @brief Constructs an Object instance representing a raw detection.
     * @param _rect Bounding box of the detection.
     * @param _label Class identifier for the detected object.
     * @param _prob Confidence score of the detection.
     * @param _feature Visual feature vector (optional).
     */
    Object::Object(const Rect<float>& _rect,
                   const int& _label,
                   const float& _prob,
                   std::vector<float> _feature)
        : rect(_rect), label(_label), prob(_prob), feature(std::move(_feature))
    {
    }

} // namespace SushiTrack
