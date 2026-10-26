/**************************************************************************/
/* tracker_registry.hpp                                                   */
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
 * @file tracker_registry.hpp
 * @brief Declares TrackerRegistry, which builds a tracker adapter from its name.
 * @author Mustafa Garip
 */

#pragma once

#include <algorithm>
#include <cctype>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <SushiTrack/logging/logger.hpp>

#include <tracker_adapter.hpp>
#include <tracker_bridge_log_category.hpp>

namespace TrackerBridge
{
    /**
     * @brief Maps a tracker name, in any case, to the function that builds its adapter.
     */
    class TrackerRegistry final
    {
        public:
            /// @brief Builds one adapter.
            using Creator = std::function<std::unique_ptr<TrackerAdapter>()>;

            /// @brief Returns the registry the adapters register into at static initialisation.
            [[nodiscard]] static TrackerRegistry& get_instance()
            {
                static TrackerRegistry instance;
                return instance;
            }

            /// @brief Registers @p creator under @p name, folded to lower case.
            void register_tracker(const std::string& name, Creator creator)
            {
                const std::string key = folded(name);
                if (creators_.count(key) != 0U)
                {
                    SUSHITRACK_LOG_WARN(K_TRACKER_BRIDGE_LOG_CATEGORY,
                                        "Tracker {} registered twice; the later creator replaces "
                                        "the first.",
                                        key);
                }
                creators_[key] = std::move(creator);
            }

            /// @brief Returns the registered names, sorted.
            [[nodiscard]] std::vector<std::string> registered_names() const
            {
                std::vector<std::string> names;
                names.reserve(creators_.size());
                for (const auto& entry : creators_)
                    names.push_back(entry.first);
                std::sort(names.begin(), names.end());
                return names;
            }

            /// @brief Returns a new adapter for @p name, or nullptr when none is registered.
            [[nodiscard]] std::unique_ptr<TrackerAdapter> create(const std::string& name) const
            {
                const auto found = creators_.find(folded(name));
                return found == creators_.end() ? nullptr : found->second();
            }

        private:
            TrackerRegistry() = default;

            /// @brief Returns @p name in lower case.
            [[nodiscard]] static std::string folded(std::string name)
            {
                std::transform(name.begin(), name.end(), name.begin(),
                               [](unsigned char c)
                               {
                                   return static_cast<char>(std::tolower(c));
                               });
                return name;
            }

            /// @brief The creator of each registered name.
            std::unordered_map<std::string, Creator> creators_;
    };
} // namespace TrackerBridge
