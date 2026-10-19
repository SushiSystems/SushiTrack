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
 * @brief Implements JSON configuration loading and path resolution helpers.
 * @author Mustafa Garip
 */

#include <SushiTrack/configuration/config_loader.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

#include <nlohmann/json.hpp>

#include <SushiTrack/c_api/sushitrack_c.h>
#include <SushiTrack/logging/logger.hpp>

namespace fs = std::filesystem;

namespace
{
    constexpr const char* K_DEFAULT_CONFIG_NAME = "sushitrack.json";
    constexpr int K_MAX_SEARCH_DEPTH = 8;

    template <typename T>
    T get_int(const nlohmann::json& sec, const char* key, T def)
    {
        auto it = sec.find(key);
        if (it == sec.end())
        {
            return def;
        }
        if (it->is_boolean())
        {
            return static_cast<T>(it->get<bool>() ? 1 : 0);
        }
        if (it->is_number())
        {
            return static_cast<T>(it->get<double>());
        }
        return def;
    }

    float get_float(const nlohmann::json& sec, const char* key, float def)
    {
        auto it = sec.find(key);
        if (it == sec.end() || !it->is_number())
        {
            return def;
        }
        return it->get<float>();
    }

    template <typename ParamsT>
    void overlay(const nlohmann::json& doc, ParamsT& p)
    {
        const nlohmann::json& st = doc.contains("sushitrack") ? doc.at("sushitrack") : doc;

        if (st.contains("basic"))
        {
            const auto& b = st.at("basic");
            p.time_aware = get_int(b, "time_aware", p.time_aware);
            p.frame_rate = get_int(b, "frame_rate", p.frame_rate);
            p.track_buffer = get_int(b, "track_buffer", p.track_buffer);
            p.track_thresh = get_float(b, "track_thresh", p.track_thresh);
            p.high_thresh = get_float(b, "high_thresh", p.high_thresh);
            p.match_thresh = get_float(b, "match_thresh", p.match_thresh);
            p.enable_tentative = get_int(b, "enable_tentative", p.enable_tentative);
            p.enable_reid = get_int(b, "enable_reid", p.enable_reid);
            p.enable_mahalanobis = get_int(b, "enable_mahalanobis", p.enable_mahalanobis);
            p.enable_ocm = get_int(b, "enable_ocm", p.enable_ocm);
            p.enable_oru = get_int(b, "enable_oru", p.enable_oru);
        }

        if (!st.contains("advanced"))
        {
            return;
        }
        const auto& adv = st.at("advanced");

        if (adv.contains("association"))
        {
            const auto& s = adv.at("association");
            p.primary_assoc_mode = get_int(s, "primary_assoc_mode", p.primary_assoc_mode);
            p.fuse_score = get_int(s, "fuse_score", p.fuse_score);
            p.iou_match_thresh = get_float(s, "iou_match_thresh", p.iou_match_thresh);
            p.unconfirmed_iou_thresh =
                get_float(s, "unconfirmed_iou_thresh", p.unconfirmed_iou_thresh);
            p.duplicate_iou_thresh = get_float(s, "duplicate_iou_thresh", p.duplicate_iou_thresh);
        }

        if (adv.contains("coordinate_system"))
        {
            const auto& s = adv.at("coordinate_system");
            p.enable_xyah = get_int(s, "enable_xyah", p.enable_xyah);
            p.iou_type = get_int(s, "iou_type", p.iou_type);
            p.iou_plus_one = get_int(s, "iou_plus_one", p.iou_plus_one);
            p.kalman_type = get_int(s, "kalman_type", p.kalman_type);
        }

        if (adv.contains("reid_params"))
        {
            const auto& s = adv.at("reid_params");
            p.feature_momentum = get_float(s, "feature_momentum", p.feature_momentum);
            p.reid_high_thresh = get_float(s, "reid_high_thresh", p.reid_high_thresh);
            p.reid_high_iou_weight = get_float(s, "reid_high_iou_weight", p.reid_high_iou_weight);
            p.reid_low_thresh = get_float(s, "reid_low_thresh", p.reid_low_thresh);
            p.reid_low_iou_weight = get_float(s, "reid_low_iou_weight", p.reid_low_iou_weight);
            p.reid_recovery_thresh = get_float(s, "reid_recovery_thresh", p.reid_recovery_thresh);
            p.max_feature_history = get_int(s, "max_feature_history", p.max_feature_history);
            p.reid_direct_assignment_weight =
                get_float(s, "reid_direct_assignment_weight", p.reid_direct_assignment_weight);
        }

        if (adv.contains("tentative_logic"))
        {
            const auto& s = adv.at("tentative_logic");
            p.tentative_match_thresh =
                get_float(s, "tentative_match_thresh", p.tentative_match_thresh);
            p.tentative_confirm_hits =
                get_int(s, "tentative_confirm_hits", p.tentative_confirm_hits);
            p.tentative_max_age_seconds =
                get_float(s, "tentative_max_age_seconds", p.tentative_max_age_seconds);
            p.tentative_max_miss_frames =
                get_int(s, "tentative_max_miss_frames", p.tentative_max_miss_frames);
            p.tentative_init_suppress_iou =
                get_float(s, "tentative_init_suppress_iou", p.tentative_init_suppress_iou);
            p.tentative_fast_thresh =
                get_float(s, "tentative_fast_thresh", p.tentative_fast_thresh);
            p.tentative_fast_hits = get_int(s, "tentative_fast_hits", p.tentative_fast_hits);
            p.tentative_slow_thresh =
                get_float(s, "tentative_slow_thresh", p.tentative_slow_thresh);
            p.tentative_slow_hits_offset =
                get_int(s, "tentative_slow_hits_offset", p.tentative_slow_hits_offset);
            p.tentative_vslow_hits_offset =
                get_int(s, "tentative_vslow_hits_offset", p.tentative_vslow_hits_offset);
        }

        if (adv.contains("ocm"))
        {
            const auto& s = adv.at("ocm");
            p.ocm_vdc_weight = get_float(s, "ocm_vdc_weight", p.ocm_vdc_weight);
            p.ocm_delta_t = get_int(s, "ocm_delta_t", p.ocm_delta_t);
            p.ocm_min_velocity = get_float(s, "ocm_min_velocity", p.ocm_min_velocity);
            p.ocm_max_age = get_int(s, "ocm_max_age", p.ocm_max_age);
            p.ocm_maneuver_weight_scale =
                get_float(s, "ocm_maneuver_weight_scale", p.ocm_maneuver_weight_scale);
        }

        if (adv.contains("mahalanobis_gating"))
        {
            const auto& s = adv.at("mahalanobis_gating");
            p.gating_thresh = get_float(s, "gating_thresh", p.gating_thresh);
            p.motion_cost_weight = get_float(s, "motion_cost_weight", p.motion_cost_weight);
            p.max_area_ratio = get_float(s, "max_area_ratio", p.max_area_ratio);
        }
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

        if (const char* env = std::getenv("SUSHITRACK_CONFIG"))
        {
            if (env[0] != '\0' && fs::exists(env, ec))
            {
                return std::string(env);
            }
        }

        fs::path dir = fs::current_path(ec);
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

    /**
     * @brief Loads tracker configuration parameters from a JSON file.
     * @return True when file was found and parsed successfully, false otherwise.
     */
    bool load_config_from_json(const char* path, TrackerConfig& config)
    {
        const std::string resolved = resolve_config_path(path);
        if (resolved.empty())
        {
            return false;
        }

        try
        {
            std::ifstream file(resolved);
            if (!file.is_open())
            {
                return false;
            }
            nlohmann::json doc = nlohmann::json::parse(file, nullptr, true, true);
            overlay(doc, config);
            return true;
        }
        catch (const std::exception&)
        {
            return false;
        }
    }

} // namespace SushiTrack

extern "C"
{
    /**
     * @brief Overlays a JSON configuration file onto a parameters structure.
     * @param path Explicit file path, or NULL to use standard resolution.
     * @param params Parameters structure updated in place.
     * @return Status code indicating success or failure reason.
     */
    sushitrack_status_t sushitrack_load_params_from_json(const char* path,
                                                         sushitrack_params_t* params)
    {
        if (!params)
        {
            return SUSHITRACK_INVALID_ARGUMENT;
        }

        std::string resolved;

        try
        {
            resolved = SushiTrack::resolve_config_path(path);
            if (resolved.empty())
            {
                SUSHITRACK_LOG_INFO(
                    "[SushiTrack] No configuration file found -- using provided parameters.");
                return SUSHITRACK_SUCCESS;
            }

            std::ifstream file(resolved);
            if (!file.is_open())
            {
                SUSHITRACK_LOG_WARN("[SushiTrack] Configuration file could not be opened: %s",
                                    resolved.c_str());
                return SUSHITRACK_SUCCESS;
            }

            nlohmann::json doc = nlohmann::json::parse(file, nullptr, true, true);
            overlay(doc, *params);

            SUSHITRACK_LOG_INFO("[SushiTrack] Configuration loaded from %s", resolved.c_str());
            return SUSHITRACK_SUCCESS;
        }
        catch (const std::exception& e)
        {
            SUSHITRACK_LOG_ERROR("[SushiTrack] Failed to parse configuration %s: %s",
                                 resolved.c_str(), e.what());
            return SUSHITRACK_RUNTIME_ERROR;
        }
        catch (...)
        {
            SUSHITRACK_LOG_ERROR("[SushiTrack] Failed to load configuration %s: unknown exception.",
                                 resolved.c_str());
            return SUSHITRACK_RUNTIME_ERROR;
        }
    }

    /**
     * @brief Instantiates a tracker directly from a JSON configuration file.
     * @param path Explicit file path, or NULL to use standard resolution order.
     * @return Handle to initialized tracker instance, or NULL on failure.
     */
    sushitrack_tracker_t sushitrack_create_from_config(const char* path)
    {
        sushitrack_params_t params;
        sushitrack_get_default_params(&params);
        sushitrack_load_params_from_json(path, &params);
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
