/**************************************************************************/
/* config_loader.cpp                                                      */
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
 * @file config_loader.cpp
 * @brief Implements configuration path resolution, the document reader and the C entry points
 *        that load a file.
 * @author Mustafa Garip
 */

#include <SushiTrack/configuration/config_loader.hpp>

#include <exception>
#include <filesystem>
#include <istream>
#include <memory>
#include <string>
#include <system_error>
#include <utility>

#include <nlohmann/json.hpp>

#include <SushiTrack/c_api/sushitrack_c.h>
#include <SushiTrack/configuration/config_host.hpp>
#include <SushiTrack/configuration/configuration_log_category.hpp>
#include <SushiTrack/logging/logger.hpp>

#include <configuration/params_overlay.hpp>

namespace fs = std::filesystem;

using SushiTrack::K_CONFIGURATION_LOG_CATEGORY;

namespace
{
    constexpr const char* K_DEFAULT_CONFIG_NAME = "sushitrack.json";
    constexpr const char* K_CONFIG_VARIABLE = "SUSHITRACK_CONFIG";
    constexpr int K_MAX_SEARCH_DEPTH = 8;

    /**
     * @brief Holds a file the caller asked for by name, and where the name came from.
     */
    struct RequestedFile
    {
            std::string path;   /**< The file named, or empty when the caller named none. */
            const char* origin; /**< What named it: the path argument or the variable. */
    };

    /**
     * @brief Returns the file the caller named: the path argument first, then SUSHITRACK_CONFIG.
     */
    RequestedFile requested_file(const char* path, const SushiTrack::ConfigHost& host)
    {
        if (path && path[0] != '\0')
            return {path, "the path argument"};
        std::string env = host.variable(K_CONFIG_VARIABLE);
        if (!env.empty())
            return {std::move(env), K_CONFIG_VARIABLE};
        return {std::string(), ""};
    }
} // namespace

namespace SushiTrack
{
    /**
     * @brief Resolves configuration file path from explicit path, environment, or search.
     * @return Resolved path string, or empty string when no file found.
     */
    std::string resolve_config_path(const char* path, const ConfigHost& host)
    {
        if (path && path[0] != '\0')
        {
            if (host.exists(path))
            {
                return std::string(path);
            }
            return std::string();
        }

        const std::string env = host.variable(K_CONFIG_VARIABLE);
        if (!env.empty() && host.exists(env))
        {
            return env;
        }

        std::error_code ec;
        fs::path dir = host.working_directory(ec);
        if (ec)
        {
            SUSHITRACK_LOG_DEBUG(K_CONFIGURATION_LOG_CATEGORY,
                                 "The current directory is unreadable ({}); the search for {} "
                                 "ends empty.",
                                 ec.message(), K_DEFAULT_CONFIG_NAME);
        }
        for (int depth = 0; depth <= K_MAX_SEARCH_DEPTH && !dir.empty(); ++depth)
        {
            fs::path candidate = dir / K_DEFAULT_CONFIG_NAME;
            if (host.exists(candidate.string()))
            {
                return candidate.string();
            }

            if (!dir.has_parent_path())
            {
                break;
            }
            fs::path parent = dir.parent_path();
            if (parent == dir)
            {
                break;
            }
            dir = parent;
        }

        return std::string();
    }

    std::string resolve_config_path(const char* path)
    {
        return resolve_config_path(path, SystemConfigHost());
    }

    /**
     * @brief Overlays the JSON configuration read from @p document onto @p params.
     * @return SUCCESS, INVALID_ARGUMENT for a refused document, RUNTIME_ERROR for an unreadable
     *         one.
     */
    sushitrack_status_t load_params(std::istream& document, const std::string& origin,
                                    sushitrack_params_t& params)
    {
        try
        {
            const nlohmann::json parsed = nlohmann::json::parse(document, nullptr, true, true);
            const std::string refusal = Detail::overlay_params(parsed, params);
            if (!refusal.empty())
            {
                SUSHITRACK_LOG_ERROR(K_CONFIGURATION_LOG_CATEGORY,
                                     "Configuration {} refused: {}; params is unchanged.", origin,
                                     refusal);
                return SUSHITRACK_INVALID_ARGUMENT;
            }

            SUSHITRACK_LOG_INFO(K_CONFIGURATION_LOG_CATEGORY, "Configuration loaded from {}",
                                origin);
            return SUSHITRACK_SUCCESS;
        }
        catch (const std::exception& e)
        {
            SUSHITRACK_LOG_ERROR(K_CONFIGURATION_LOG_CATEGORY,
                                 "Failed to parse configuration {}: {}", origin, e.what());
            return SUSHITRACK_RUNTIME_ERROR;
        }
        catch (...)
        {
            SUSHITRACK_LOG_ERROR(K_CONFIGURATION_LOG_CATEGORY,
                                 "Failed to load configuration {}: unknown exception.", origin);
            return SUSHITRACK_RUNTIME_ERROR;
        }
    }

    /**
     * @brief Finds the file through @p host and overlays it onto @p params.
     * @return SUCCESS when the file applied or none was asked for and none was found,
     *         INVALID_ARGUMENT for a named file that does not exist or a document the reader
     *         refuses, RUNTIME_ERROR for a file that cannot be read or parsed.
     */
    sushitrack_status_t load_params_from_path(const char* path, const ConfigHost& host,
                                              sushitrack_params_t& params)
    {
        std::string resolved;

        try
        {
            const RequestedFile requested = requested_file(path, host);
            if (!requested.path.empty() && !host.exists(requested.path))
            {
                SUSHITRACK_LOG_ERROR(K_CONFIGURATION_LOG_CATEGORY,
                                     "Configuration load refused: {} names {}, which does not "
                                     "exist; params is unchanged.",
                                     requested.origin, requested.path);
                return SUSHITRACK_INVALID_ARGUMENT;
            }

            resolved = resolve_config_path(path, host);
            if (resolved.empty())
            {
                SUSHITRACK_LOG_INFO(K_CONFIGURATION_LOG_CATEGORY,
                                    "No configuration file found -- using provided parameters.");
                return SUSHITRACK_SUCCESS;
            }

            const std::unique_ptr<std::istream> file = host.open(resolved);
            if (!file)
            {
                SUSHITRACK_LOG_ERROR(K_CONFIGURATION_LOG_CATEGORY,
                                     "Configuration load failed: {} exists but cannot be opened; "
                                     "params is unchanged.",
                                     resolved);
                return SUSHITRACK_RUNTIME_ERROR;
            }

            return load_params(*file, resolved, params);
        }
        catch (const std::exception& e)
        {
            SUSHITRACK_LOG_ERROR(K_CONFIGURATION_LOG_CATEGORY,
                                 "Failed to parse configuration {}: {}", resolved, e.what());
            return SUSHITRACK_RUNTIME_ERROR;
        }
        catch (...)
        {
            SUSHITRACK_LOG_ERROR(K_CONFIGURATION_LOG_CATEGORY,
                                 "Failed to load configuration {}: unknown exception.", resolved);
            return SUSHITRACK_RUNTIME_ERROR;
        }
    }
} // namespace SushiTrack

extern "C"
{
    /**
     * @brief Overlays a JSON configuration file onto a parameters structure.
     * @param path Explicit file path, or NULL to use standard resolution.
     * @param params Parameters structure updated in place; untouched on any failure.
     * @return SUCCESS when the file applied or none was asked for and none was found,
     *         INVALID_ARGUMENT for a null params, a named file that does not exist or a
     *         document the reader refuses, RUNTIME_ERROR for a file that cannot be read or parsed.
     */
    sushitrack_status_t sushitrack_load_params_from_json(const char* path,
                                                         sushitrack_params_t* params)
    {
        if (!params)
        {
            SUSHITRACK_LOG_ERROR(K_CONFIGURATION_LOG_CATEGORY,
                                 "Configuration load refused: params is null; nothing was read.");
            return SUSHITRACK_INVALID_ARGUMENT;
        }

        return SushiTrack::load_params_from_path(path, SushiTrack::SystemConfigHost(), *params);
    }

    /**
     * @brief Instantiates a tracker directly from a JSON configuration file.
     * @param path Explicit file path, or NULL to use standard resolution order.
     * @return Handle to initialized tracker instance, or NULL when the file is refused or the
     *         tracker cannot be built; the reason is logged.
     */
    sushitrack_tracker_t sushitrack_create_from_config(const char* path)
    {
        sushitrack_params_t params;
        sushitrack_get_default_params(&params);
        if (sushitrack_load_params_from_json(path, &params) != SUSHITRACK_SUCCESS)
            return nullptr;
        return sushitrack_create(&params);
    }

    /**
     * @brief Instantiates a tracker directly from a configuration file.
     * @param config_path Path to JSON file, or NULL to use default resolution.
     * @return Handle to initialized tracker instance, or NULL on failure.
     */
    sushitrack_tracker_t sushitrack_create_tracker(const char* config_path)
    {
        return sushitrack_create_from_config(config_path);
    }

} // extern "C"
