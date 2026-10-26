/**************************************************************************/
/* json_settings.hpp                                                      */
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
 * @file json_settings.hpp
 * @brief Declares read_json_settings, json_section and json_number, which read a reference
 *        tracker's settings and log what they cannot use.
 * @author Mustafa Garip
 */

#pragma once

#include <exception>
#include <fstream>
#include <string>

#include <nlohmann/json.hpp>

#include <SushiTrack/logging/logger.hpp>

#include <tracker_bridge_log_category.hpp>

namespace TrackerBridge
{
    /**
     * @brief Returns the JSON document in @p path, or an empty object when the file cannot be
     *        opened or parsed, logging why.
     */
    inline nlohmann::json read_json_settings(const std::string& path)
    {
        nlohmann::json result = nlohmann::json::object();
        std::ifstream file(path);
        if (!file.is_open())
        {
            SUSHITRACK_LOG_WARN(K_TRACKER_BRIDGE_LOG_CATEGORY,
                                "Settings file {} cannot be opened; the tracker keeps its "
                                "defaults.",
                                path);
            return result;
        }
        try
        {
            result = nlohmann::json::parse(file, nullptr, true, true);
        }
        catch (const std::exception& error)
        {
            SUSHITRACK_LOG_ERROR(K_TRACKER_BRIDGE_LOG_CATEGORY,
                                 "Settings file {} is not valid JSON ({}); the tracker keeps its "
                                 "defaults.",
                                 path, error.what());
        }
        return result;
    }

    /**
     * @brief Returns the section @p key of @p document read from @p path, or an empty object
     *        after logging that the file has none.
     */
    inline const nlohmann::json& json_section(const nlohmann::json& document,
                                              const std::string& key, const std::string& path)
    {
        static const nlohmann::json empty = nlohmann::json::object();
        const auto found = document.find(key);
        if (found == document.end())
        {
            SUSHITRACK_LOG_WARN(K_TRACKER_BRIDGE_LOG_CATEGORY,
                                "Settings file {} has no \"{}\" section; the tracker keeps its "
                                "defaults.",
                                path, key);
            return empty;
        }
        return found.value();
    }

    /**
     * @brief Returns the number @p section holds under @p key, or @p fallback when it holds none.
     *
     * An absent key gives the fallback silently; a key that holds something else than a number
     * gives it after a WARN.
     */
    inline float json_number(const nlohmann::json& section, const std::string& key, float fallback)
    {
        if (!section.is_object() || !section.contains(key))
            return fallback;
        const nlohmann::json& value = section[key];
        if (!value.is_number())
        {
            SUSHITRACK_LOG_WARN(K_TRACKER_BRIDGE_LOG_CATEGORY,
                                "Setting {} is not a number; keeping {}.", key, fallback);
            return fallback;
        }
        return value.get<float>();
    }
} // namespace TrackerBridge
