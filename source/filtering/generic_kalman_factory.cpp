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
/* Licensed under the Apache License, Version 2.0 (the "License"); you    */
/* may not use this file except in compliance with the License. You may   */
/* obtain a copy of the License at                                        */
/*                                                                        */
/*     https://www.apache.org/licenses/LICENSE-2.0                        */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                         */
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
        : GenericKalmanFactory(kalman_type, enable_xyah, KalmanFilterRegistry::get_instance())
    {
    }

    /**
     * @brief Initializes the generic Kalman factory over a registry the caller owns.
     * @param kalman_type Identifier for the Kalman filter implementation.
     * @param enable_xyah Flag selecting the xyah coordinate parameterization.
     * @param registry The registry create() asks; it must outlive the factory.
     */
    GenericKalmanFactory::GenericKalmanFactory(int kalman_type, bool enable_xyah,
                                               const KalmanFilterRegistry& registry)
        : kalman_type_(kalman_type), enable_xyah_(enable_xyah), registry_(&registry)
    {
    }

    /**
     * @brief Instantiates a state predictor through the registry.
     * @return Unique pointer to the constructed state predictor.
     */
    std::unique_ptr<StatePredictor> GenericKalmanFactory::create() const
    {
        return registry_->create(kalman_type_, enable_xyah_);
    }
} // namespace SushiTrack
