/****************************************************************/
/* i_kalman_filter.hpp                                          */
/* SushiTrack - https://github.com/SushiSystems/SushiTrack      */
/* Copyright (c) 2026 Sushi Systems                             */
/* Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.    */
/* Commercial use requires a licence from Sushi Systems.        */
/****************************************************************/

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
