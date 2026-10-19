/**************************************************************************/
/* generic_kalman_factory.hpp                                             */
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
 * @file generic_kalman_factory.hpp
 * @brief Declares the factory that creates a state predictor from the Kalman filter registry.
 * @author Mustafa Garip
 */

#pragma once

#include <memory>

#include <SushiTrack/filtering/kalman_factory.hpp>
#include <SushiTrack/filtering/state_predictor.hpp>

namespace SushiTrack
{
    /**
     * @brief Generic factory for instantiating state predictors via a dynamic registry.
     */
    class GenericKalmanFactory final : public KalmanFactory
    {
        public:
            /**
             * @brief Initializes factory with Kalman type and coordinate parameters.
             * @param kalman_type Identifier for the Kalman filter type.
             * @param enable_xyah Flag for coordinate system selection.
             */
            explicit GenericKalmanFactory(int kalman_type, bool enable_xyah = false);

            /**
             * @brief Generates a new state predictor instance from registry.
             * @return Unique pointer to initialized state predictor.
             */
            [[nodiscard]] std::unique_ptr<StatePredictor> create() const override;

        private:
            int kalman_type_;
            bool enable_xyah_;
    };
} // namespace SushiTrack
