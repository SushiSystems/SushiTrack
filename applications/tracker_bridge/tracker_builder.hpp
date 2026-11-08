/**************************************************************************/
/* tracker_builder.hpp                                                    */
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
 * @file tracker_builder.hpp
 * @brief Declares build_tracker, which builds the adapter a registry holds under a name and
 *        logs a refusal.
 * @author Mustafa Garip
 */

#pragma once

#include <exception>
#include <memory>
#include <string>

#include <SushiTrack/logging/logger.hpp>

#include <tracker_adapter.hpp>
#include <tracker_bridge_log_category.hpp>
#include <tracker_registry.hpp>

namespace TrackerBridge
{
    /**
     * @brief Returns the names @p registry holds, joined by commas.
     */
    [[nodiscard]] inline std::string registered_list(const TrackerRegistry& registry)
    {
        std::string list;
        for (const std::string& name : registry.registered_names())
            list += (list.empty() ? "" : ", ") + name;
        return list;
    }

    /**
     * @brief Returns a new adapter @p registry builds for @p name, or nullptr after one ERROR
     *        when the name is null or unknown or the adapter throws.
     * @param name The tracker's name in any case; may be null.
     */
    [[nodiscard]] inline std::unique_ptr<TrackerAdapter> build_tracker(
        const TrackerRegistry& registry, const char* name)
    {
        if (name == nullptr)
        {
            SUSHITRACK_LOG_ERROR(K_TRACKER_BRIDGE_LOG_CATEGORY,
                                 "create_tracker refused: the name is null; no tracker was "
                                 "built.");
            return nullptr;
        }
        try
        {
            std::unique_ptr<TrackerAdapter> tracker = registry.create(name);
            if (tracker == nullptr)
            {
                SUSHITRACK_LOG_ERROR(K_TRACKER_BRIDGE_LOG_CATEGORY,
                                     "create_tracker refused: {} is not registered (known: {}); "
                                     "no tracker was built.",
                                     name, registered_list(registry));
            }
            return tracker;
        }
        catch (const std::exception& error)
        {
            SUSHITRACK_LOG_ERROR(K_TRACKER_BRIDGE_LOG_CATEGORY,
                                 "create_tracker failed for {}: {}; no tracker was built.", name,
                                 error.what());
            return nullptr;
        }
        catch (...)
        {
            SUSHITRACK_LOG_ERROR(K_TRACKER_BRIDGE_LOG_CATEGORY,
                                 "create_tracker failed for {}: unknown exception; no tracker "
                                 "was built.",
                                 name);
            return nullptr;
        }
    }
} // namespace TrackerBridge
