/****************************************************************/
/* kalman_filter_registry.hpp                                   */
/* SushiTrack - https://github.com/SushiSystems/SushiTrack      */
/* Copyright (c) 2026 Sushi Systems                             */
/* Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.    */
/* Commercial use requires a licence from Sushi Systems.        */
/****************************************************************/

/**
 * @file kalman_filter_registry.hpp
 * @brief Dynamic registry for Kalman filter implementations.
 * @author Mustafa Garip
 */

#pragma once

#include <SushiTrack/i_kalman_filter.hpp>
#include <SushiTrack/logger.hpp>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

namespace SushiTrack
{
    /**
     * @brief Signature for Kalman filter instantiation functions.
     */
    using KalmanCreator = std::function<std::unique_ptr<IKalmanFilter>(bool)>;

    /**
     * @brief Singleton registry for managing and instantiating Kalman filters.
     */
    class KalmanFilterRegistry
    {
    public:
        /**
         * @brief Retrieves the global registry instance.
         * @return Reference to the singleton registry.
         */
        static KalmanFilterRegistry& getInstance()
        {
            static KalmanFilterRegistry instance;
            return instance;
        }

        /**
         * @brief Registers a new Kalman filter creator.
         * @param type Identifier for the Kalman filter type.
         * @param creator Factory function to instantiate the filter.
         */
        void registerCreator(int type, KalmanCreator creator)
        {
            creators_[type] = std::move(creator);
        }

        /**
         * @brief Instantiates a Kalman filter based on its registered type.
         * @param type Identifier for the desired Kalman filter type.
         * @param enable_xyah Flag for coordinate system selection.
         * @return Unique pointer to newly instantiated filter, or nullptr.
         */
        std::unique_ptr<IKalmanFilter> create(int type, bool enable_xyah) const
        {
            auto it = creators_.find(type);
            if (it != creators_.end())
            {
                return it->second(enable_xyah);
            }
            SUSHITRACK_LOG_ERROR(("[KalmanFilterRegistry] Unregistered Kalman type requested: " + std::to_string(type)).c_str());
            return nullptr;
        }

    private:
        KalmanFilterRegistry() = default;
        std::unordered_map<int, KalmanCreator> creators_;
    };

    /**
     * @brief Helper structure for automatic registration of Kalman filters.
     * @tparam T The Kalman filter class to register.
     */
    template <typename T>
    struct KalmanFilterRegistrar
    {
        /**
         * @brief Registers the given class to the specified type identifier.
         * @param type Identifier for the Kalman filter type.
         */
        explicit KalmanFilterRegistrar(int type)
        {
            KalmanFilterRegistry::getInstance().registerCreator(type,
                [](bool enable_xyah) {
                    return std::make_unique<T>(enable_xyah);
                });
        }
    };

} // namespace SushiTrack
