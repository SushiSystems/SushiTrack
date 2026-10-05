/****************************************************************/
/* config_loader.hpp                                            */
/* SushiTrack - https://github.com/SushiSystems/SushiTrack      */
/* Copyright (c) 2026 Sushi Systems                             */
/* Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.    */
/* Commercial use requires a licence from Sushi Systems.        */
/****************************************************************/

/**
 * @file config_loader.hpp
 * @brief Declares configuration file loading helpers for tracker parameters.
 * @author Mustafa Garip
 */

#pragma once

#include <SushiTrack/sushitrack_c.h>
#include <SushiTrack/sushitrack_export.hpp>
#include <SushiTrack/tracker_config.hpp>

#include <string>

namespace SushiTrack
{
    /**
     * @brief Resolves configuration file path from explicit path, environment, or search.
     * @param path Explicit path or null/empty to use resolution order.
     * @return Resolved path string, or empty string when no file found.
     */
    SUSHITRACK_API std::string resolveConfigPath(const char* path);

    /**
     * @brief Loads tracker configuration parameters from a JSON file.
     * @param path File path or null for auto-discovery.
     * @param config Target TrackerConfig structure updated in place.
     * @return True when file was found and parsed successfully, false otherwise.
     */
    SUSHITRACK_API bool loadConfigFromJson(const char* path, TrackerConfig& config);
} // namespace SushiTrack
