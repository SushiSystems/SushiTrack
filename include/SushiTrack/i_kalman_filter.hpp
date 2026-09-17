/**************************************************************************/
/* i_kalman_filter.hpp                                                    */
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
 * @file i_kalman_filter.hpp
 * @brief Unified interface for Kalman filter variants.
 * @author Mustafa Garip
 */

#pragma once

#include <SushiTrack/i_gate_validator.hpp>
#include <SushiTrack/i_state_predictor.hpp>

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
