/**************************************************************************/
/* kalman_factory.hpp                                                     */
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
 * @file kalman_factory.hpp
 * @brief Declares the interface that creates state predictors.
 * @author Mustafa Garip
 */

#pragma once

#include <memory>

#include <SushiTrack/filtering/state_predictor.hpp>

namespace SushiTrack
{
    /**
     * @brief Interface for instantiating StatePredictor objects.
     */
    class KalmanFactory
    {
        public:
            /**
             * @brief Virtual destructor for the interface.
             */
            virtual ~KalmanFactory() = default;

            /**
             * @brief Creates and returns a new state predictor instance.
             * @return Unique pointer to the newly initialized state predictor.
             */
            [[nodiscard]] virtual std::unique_ptr<StatePredictor> create() const = 0;
    };
} // namespace SushiTrack
