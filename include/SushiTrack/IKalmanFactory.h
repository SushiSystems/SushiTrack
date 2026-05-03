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
            /**
             * @brief Initializes the factory with dynamic type and baseline noise parameters.
             * 
             * @param kalman_type Identifier for the Kalman filter type.
             * @param std_weight_pos Baseline uncertainty for position.
             * @param std_weight_vel Baseline uncertainty for velocity.
             * @param kf_pos_weight Scaling factor for position uncertainty.
             * @param kf_size_weight Scaling factor for size uncertainty.
             * @param kf_vel_weight Scaling factor for velocity uncertainty.
             * @param enable_xyah Coordinate system selection flag.
             */
            GenericKalmanFactory(int kalman_type, float std_weight_pos = 0.05f, float std_weight_vel = 0.00625f,
                                 float kf_pos_weight = 2.0f, float kf_size_weight = 2.0f, float kf_vel_weight = 10.0f,
                                 bool enable_xyah = false);

            /**
             * @brief Generates a new state predictor instance from the registry.
             * 
             * @return Unique pointer to the initialized state predictor.
             */
            std::unique_ptr<IStatePredictor> create() const override;

        private:
            int kalman_type_;
            float std_weight_pos_, std_weight_vel_, kf_pos_weight_, kf_size_weight_, kf_vel_weight_;
            bool enable_xyah_;
    };

} // namespace SushiTrack
