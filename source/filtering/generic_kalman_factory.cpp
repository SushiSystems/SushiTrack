/**************************************************************************/
/* generic_kalman_factory.cpp                                             */
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
 * @file generic_kalman_factory.cpp
 * @brief Defines the factory that creates a state predictor from the Kalman filter registry.
 * @author Mustafa Garip
 */

#include <SushiTrack/filtering/generic_kalman_factory.hpp>

#include <memory>

#include <SushiTrack/filtering/kalman_filter_registry.hpp>

namespace SushiTrack
{
    /**
     * @brief Initializes the generic Kalman factory.
     * @param kalman_type Identifier for the Kalman filter implementation.
     * @param enable_xyah Flag selecting the xyah coordinate parameterization.
     */
    GenericKalmanFactory::GenericKalmanFactory(int kalman_type, bool enable_xyah)
        : kalman_type_(kalman_type), enable_xyah_(enable_xyah)
    {
    }

    /**
     * @brief Instantiates a state predictor through the registry.
     * @return Unique pointer to the constructed state predictor.
     */
    std::unique_ptr<IStatePredictor> GenericKalmanFactory::create() const
    {
        return KalmanFilterRegistry::getInstance().create(
            kalman_type_, enable_xyah_);
    }
} // namespace SushiTrack
