/**************************************************************************/
/* config_loader.hpp                                                      */
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
 * @file config_loader.hpp
 * @brief Declares the helpers that find the tracker configuration file and read a document.
 * @author Mustafa Garip
 */

#pragma once

#include <istream>
#include <string>

#include <SushiTrack/c_api/sushitrack_c.h>
#include <SushiTrack/c_api/sushitrack_export.hpp>

namespace SushiTrack
{
    /**
     * @brief Resolves configuration file path from explicit path, environment, or search.
     * @param path Explicit path or null/empty to use resolution order.
     * @return Resolved path string, or empty string when no file found.
     */
    SUSHITRACK_API std::string resolve_config_path(const char* path);

    /**
     * @brief Overlays the JSON configuration read from @p document onto @p params, as
     *        sushitrack_load_params_from_json does with the file it opens.
     * @param origin The name the log lines give the document, such as its path.
     * @param params Updated in place; untouched on any failure.
     * @return SUCCESS when it applied, INVALID_ARGUMENT for a document the reader refuses,
     *         RUNTIME_ERROR for one that cannot be read or parsed.
     */
    [[nodiscard]] SUSHITRACK_API sushitrack_status_t load_params(std::istream& document,
                                                                 const std::string& origin,
                                                                 sushitrack_params_t& params);
} // namespace SushiTrack
