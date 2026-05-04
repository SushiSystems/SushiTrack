/**
 * @file      IKalmanFactory.h
 * @brief     Interface definition for state predictor instantiation factories.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      28.04.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */

#pragma once

#include "SushiTrack/IStatePredictor.h"
#include <memory>

namespace SushiTrack
{
    /**
     * @brief Interface for instantiating IStatePredictor objects.
     * 
     * Defines a factory for generating unique state predictors for individual 
     * tracklets, decoupling predictor configuration from the main tracker logic.
     */
    class IKalmanFactory
    {
        public:
            virtual ~IKalmanFactory() = default;

            /**
             * @brief Creates and returns a new state predictor instance.
             * 
             * @return Unique pointer to the newly initialized state predictor.
             */
            virtual std::unique_ptr<IStatePredictor> create() const = 0;
    };

    /**
     * @brief Generic factory for instantiating state predictors via a dynamic registry.
     * 
     * Stores and applies baseline noise parameters and the selected Kalman type 
     * to each generated predictor instance.
     */
    class GenericKalmanFactory : public IKalmanFactory
    {
        public:
            GenericKalmanFactory(int kalman_type, bool enable_xyah = false);

            /**
             * @brief Generates a new state predictor instance from the registry.
             * 
             * @return Unique pointer to the initialized state predictor.
             */
            std::unique_ptr<IStatePredictor> create() const override;

        private:
            int kalman_type_;
            bool enable_xyah_;
    };

} // namespace SushiTrack
