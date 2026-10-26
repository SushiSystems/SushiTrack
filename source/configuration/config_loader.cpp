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
 * @file config_loader.cpp
 * @brief Implements configuration path resolution and the C entry points that load a file.
 * @author Mustafa Garip
 */

#include <SushiTrack/configuration/config_loader.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

#include <nlohmann/json.hpp>

#include <SushiTrack/c_api/sushitrack_c.h>
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
    RequestedFile requested_file(const char* path)
    {
        if (path && path[0] != '\0')
            return {path, "the path argument"};
        if (const char* env = std::getenv(K_CONFIG_VARIABLE))
        {
            if (env[0] != '\0')
                return {env, K_CONFIG_VARIABLE};
        }
        return {std::string(), ""};
    }
} // namespace

namespace SushiTrack
{
    /**
     * @brief Resolves configuration file path from explicit path, environment, or search.
     * @return Resolved path string, or empty string when no file found.
     */
    std::string resolve_config_path(const char* path)
    {
        std::error_code ec;

        if (path && path[0] != '\0')
        {
            if (fs::exists(path, ec))
            {
                return std::string(path);
            }
            return std::string();
        }

        if (const char* env = std::getenv(K_CONFIG_VARIABLE))
        {
            if (env[0] != '\0' && fs::exists(env, ec))
            {
                return std::string(env);
            }
        }

        fs::path dir = fs::current_path(ec);
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
            if (fs::exists(candidate, ec))
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

        std::string resolved;

        try
        {
            const RequestedFile requested = requested_file(path);
            std::error_code ec;
            if (!requested.path.empty() && !fs::exists(requested.path, ec))
            {
                SUSHITRACK_LOG_ERROR(K_CONFIGURATION_LOG_CATEGORY,
                                     "Configuration load refused: {} names {}, which does not "
                                     "exist; params is unchanged.",
                                     requested.origin, requested.path);
                return SUSHITRACK_INVALID_ARGUMENT;
            }

            resolved = SushiTrack::resolve_config_path(path);
            if (resolved.empty())
            {
                SUSHITRACK_LOG_INFO(K_CONFIGURATION_LOG_CATEGORY,
                                    "No configuration file found -- using provided parameters.");
                return SUSHITRACK_SUCCESS;
            }

            std::ifstream file(resolved);
            if (!file.is_open())
            {
                SUSHITRACK_LOG_ERROR(K_CONFIGURATION_LOG_CATEGORY,
                                     "Configuration load failed: {} exists but cannot be opened; "
                                     "params is unchanged.",
                                     resolved);
                return SUSHITRACK_RUNTIME_ERROR;
            }

            const nlohmann::json document = nlohmann::json::parse(file, nullptr, true, true);
            const std::string refusal = SushiTrack::Detail::overlay_params(document, *params);
            if (!refusal.empty())
            {
                SUSHITRACK_LOG_ERROR(K_CONFIGURATION_LOG_CATEGORY,
                                     "Configuration {} refused: {}; params is unchanged.",
                                     resolved, refusal);
                return SUSHITRACK_INVALID_ARGUMENT;
            }

            SUSHITRACK_LOG_INFO(K_CONFIGURATION_LOG_CATEGORY, "Configuration loaded from {}",
                                resolved);
            return SUSHITRACK_SUCCESS;
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
