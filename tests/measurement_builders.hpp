/**************************************************************************/
/* measurement_builders.hpp                                               */
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
/**************************************************************************/

/**
 * @file measurement_builders.hpp
 * @brief Declares make_box, which builds the measurements the Kalman filter tests feed.
 * @author Mustafa Garip
 */

#pragma once

#include <SushiTrack/filtering/kalman_types.hpp>

namespace SushiTrack
{
    /** @brief Returns the measurement [x, y, w, h]. */
    inline KalmanTypes::DetectBox make_box(float x, float y, float w, float h)
    {
        KalmanTypes::DetectBox box;
        box << x, y, w, h;
        return box;
    }
} // namespace SushiTrack
