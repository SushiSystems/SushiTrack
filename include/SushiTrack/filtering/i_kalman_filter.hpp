/**************************************************************************/
/* i_kalman_filter.hpp                                                    */
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
 * @file i_kalman_filter.hpp
 * @brief Unified interface for Kalman filter variants.
 * @author Mustafa Garip
 */

#pragma once

#include <SushiTrack/filtering/i_gate_validator.hpp>
#include <SushiTrack/filtering/i_state_predictor.hpp>

namespace SushiTrack
{
    /**
     * @brief Unified interface combining state prediction and gate validation.
     */
    class IKalmanFilter : public IStatePredictor, public IGateValidator
    {
    public:
        /**
         * @brief Virtual destructor for the interface.
         */
        virtual ~IKalmanFilter() = default;
    };

} // namespace SushiTrack
