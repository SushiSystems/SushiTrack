/**************************************************************************/
/* kalman_filter_registry.hpp                                             */
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
 * @file kalman_filter_registry.hpp
 * @brief Dynamic registry for Kalman filter implementations.
 * @author Mustafa Garip
 */

#pragma once

#include <SushiTrack/filtering/i_kalman_filter.hpp>
#include <SushiTrack/logging/logger.hpp>

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
            SUSHITRACK_LOG_ERROR("[KalmanFilterRegistry] Unregistered Kalman type requested: %d", type);
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
