/**************************************************************************/
/* object.cpp                                                             */
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
/*                                                                        */
/* Holds code ported from ByteTrack-cpp (MIT), see NOTICE.md              */
/* Copyright (c) 2022 Vertical Beach (lp6m, medalotte)                    */
/* Copyright (c) 2021 Yifu Zhang                                          */
/**************************************************************************/

/**
 * @file object.cpp
 * @brief Implements object detection data structure constructors.
 * @author Mustafa Garip
 */

#include <SushiTrack/detection/object.hpp>

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
    Object::Object(const Rect<float>& _rect, const int& _label, const float& _prob,
                   std::vector<float> _feature)
        : rect(_rect), label(_label), prob(_prob), feature(std::move(_feature))
    {
    }

} // namespace SushiTrack
