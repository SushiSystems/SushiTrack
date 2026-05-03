/**
 * @file      IKalmanFilter.h
 * @brief     Unified interface for Kalman filter variants.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      30.04.2026
 * @version   v1.1.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */

#pragma once

#include "SushiTrack/IStatePredictor.h"
#include "SushiTrack/IGateValidator.h"

namespace SushiTrack
{
    /**
     * @brief Unified interface encompassing state prediction and gate validation.
     * 
     * Combines the IStatePredictor and IGateValidator interfaces into a single 
     * type, allowing a single Kalman Filter instance to be used for both motion 
     * prediction and search space validation. This conforms to the Interface 
     * Segregation Principle while providing a cohesive type for the tracking engine.
     */
    class IKalmanFilter : public IStatePredictor, public IGateValidator
    {
        public:
            virtual ~IKalmanFilter() = default;
    };

} // namespace SushiTrack
