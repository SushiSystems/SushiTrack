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
 * @file kalman_filter_registry.hpp
 * @brief Dynamic registry for Kalman filter implementations.
 * @author Mustafa Garip
 */

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include <SushiTrack/filtering/filtering_log_category.hpp>
#include <SushiTrack/filtering/gated_state_predictor.hpp>
#include <SushiTrack/logging/logger.hpp>

namespace SushiTrack
{
    /**
     * @brief Signature for Kalman filter instantiation functions.
     */
    using KalmanCreator = std::function<std::unique_ptr<GatedStatePredictor>(bool)>;

    /**
     * @brief Maps a Kalman filter type to the creator that instantiates it.
     *
     * The filters of the library register into get_instance() at static initialisation; a
     * caller that wants its own set owns an instance and passes it in.
     */
    class KalmanFilterRegistry final
    {
        public:
            /// @brief Builds a registry that holds no creator.
            KalmanFilterRegistry() = default;

            /**
             * @brief Retrieves the global registry instance.
             * @return Reference to the registry the library's filters register into.
             */
            [[nodiscard]] static KalmanFilterRegistry& get_instance()
            {
                static KalmanFilterRegistry instance;
                return instance;
            }

            /**
             * @brief Registers a new Kalman filter creator.
             * @param type Identifier for the Kalman filter type.
             * @param creator Factory function to instantiate the filter.
             */
            void register_creator(int type, KalmanCreator creator)
            {
                if (creators_.count(type) != 0U)
                {
                    SUSHITRACK_LOG_WARN(K_FILTERING_LOG_CATEGORY,
                                        "Kalman type {} registered twice; the later creator "
                                        "replaces the first.",
                                        type);
                }
                creators_[type] = std::move(creator);
            }

            /**
             * @brief Instantiates a Kalman filter based on its registered type.
             * @param type Identifier for the desired Kalman filter type.
             * @param enable_xyah Flag for coordinate system selection.
             * @return Unique pointer to newly instantiated filter, or nullptr.
             */
            [[nodiscard]] std::unique_ptr<GatedStatePredictor> create(int type,
                                                                      bool enable_xyah) const
            {
                auto it = creators_.find(type);
                if (it != creators_.end())
                {
                    return it->second(enable_xyah);
                }
                SUSHITRACK_LOG_ERROR(K_FILTERING_LOG_CATEGORY, "Unregistered Kalman type requested: {}", type);
                return nullptr;
            }

        private:
            /// @brief The creator of each registered type.
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
                KalmanFilterRegistry::get_instance().register_creator(
                    type,
                    [](bool enable_xyah)
                    {
                        return std::make_unique<T>(enable_xyah);
                    });
            }
    };

} // namespace SushiTrack
