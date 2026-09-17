/**************************************************************************/
/* object.cpp                                                             */
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
