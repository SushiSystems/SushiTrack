/**************************************************************************/
/* config_loader.hpp                                                      */
/**************************************************************************/
/*                          This file is part of:                         */
/*                               SushiTrack                               */
/*              https://github.com/SushiSystems/SushiTrack                */
/*                          https://sushisystems.io                       */
/**************************************************************************/
/* Copyright (c) 2026-present Mustafa Garip & Sushi Systems               */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License");        */
/* you may not use this file except in compliance with the License.       */
/* You may obtain a copy of the License at                                */
/*                                                                        */
/*     http://www.apache.org/licenses/LICENSE-2.0                         */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                          */
/**************************************************************************/

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
