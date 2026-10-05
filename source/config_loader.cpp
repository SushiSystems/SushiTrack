/****************************************************************/
/* config_loader.cpp                                            */
/* SushiTrack - https://github.com/SushiSystems/SushiTrack      */
/* Copyright (c) 2026 Sushi Systems                             */
/* Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.    */
/* Commercial use requires a licence from Sushi Systems.        */
/****************************************************************/

/**
 * @file config_loader.cpp
 * @brief Implements JSON configuration loading and path resolution helpers.
 * @author Mustafa Garip
 */

#include <SushiTrack/config_loader.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

#include <nlohmann/json.hpp>

#include <SushiTrack/logger.hpp>
#include <SushiTrack/sushitrack_c.h>

namespace fs = std::filesystem;

namespace
{
    constexpr const char* kDefaultConfigName = "sushitrack.json";
    constexpr int kMaxSearchDepth = 8;

    template <typename T>
    T getInt(const nlohmann::json& sec, const char* key, T def)
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

    float getFloat(const nlohmann::json& sec, const char* key, float def)
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
        const nlohmann::json& st =
            doc.contains("sushitrack") ? doc.at("sushitrack") : doc;

        if (st.contains("basic"))
        {
            const auto& b = st.at("basic");
            p.time_aware                 = getInt(b, "time_aware",                 p.time_aware);
            p.frame_rate                 = getInt(b, "frame_rate",                 p.frame_rate);
            p.track_buffer               = getInt(b, "track_buffer",               p.track_buffer);
            p.track_thresh               = getFloat(b, "track_thresh",             p.track_thresh);
            p.high_thresh                = getFloat(b, "high_thresh",              p.high_thresh);
            p.match_thresh               = getFloat(b, "match_thresh",             p.match_thresh);
            p.enable_tentative           = getInt(b, "enable_tentative",           p.enable_tentative);
            p.enable_reid                = getInt(b, "enable_reid",                p.enable_reid);
            p.enable_mahalanobis         = getInt(b, "enable_mahalanobis",         p.enable_mahalanobis);
            p.enable_ocm                 = getInt(b, "enable_ocm",                 p.enable_ocm);
            p.enable_oru                 = getInt(b, "enable_oru",                 p.enable_oru);
        }

        if (!st.contains("advanced"))
        {
            return;
        }
        const auto& adv = st.at("advanced");

        if (adv.contains("association"))
        {
            const auto& s = adv.at("association");
            p.primary_assoc_mode     = getInt(s, "primary_assoc_mode",     p.primary_assoc_mode);
            p.fuse_score             = getInt(s, "fuse_score",             p.fuse_score);
            p.iou_match_thresh       = getFloat(s, "iou_match_thresh",       p.iou_match_thresh);
            p.unconfirmed_iou_thresh = getFloat(s, "unconfirmed_iou_thresh", p.unconfirmed_iou_thresh);
            p.duplicate_iou_thresh   = getFloat(s, "duplicate_iou_thresh",   p.duplicate_iou_thresh);
        }

        if (adv.contains("coordinate_system"))
        {
            const auto& s = adv.at("coordinate_system");
            p.enable_xyah  = getInt(s, "enable_xyah",  p.enable_xyah);
            p.iou_type     = getInt(s, "iou_type",     p.iou_type);
            p.iou_plus_one = getInt(s, "iou_plus_one", p.iou_plus_one);
            p.kalman_type  = getInt(s, "kalman_type",  p.kalman_type);
        }

        if (adv.contains("reid_params"))
        {
            const auto& s = adv.at("reid_params");
            p.feature_momentum     = getFloat(s, "feature_momentum",     p.feature_momentum);
            p.reid_high_thresh     = getFloat(s, "reid_high_thresh",     p.reid_high_thresh);
            p.reid_high_iou_weight = getFloat(s, "reid_high_iou_weight", p.reid_high_iou_weight);
            p.reid_low_thresh      = getFloat(s, "reid_low_thresh",      p.reid_low_thresh);
            p.reid_low_iou_weight  = getFloat(s, "reid_low_iou_weight",  p.reid_low_iou_weight);
            p.reid_recovery_thresh = getFloat(s, "reid_recovery_thresh", p.reid_recovery_thresh);
            p.max_feature_history  = getInt(s, "max_feature_history",  p.max_feature_history);
            p.reid_direct_assignment_weight = getFloat(s, "reid_direct_assignment_weight", p.reid_direct_assignment_weight);
        }

        if (adv.contains("tentative_logic"))
        {
            const auto& s = adv.at("tentative_logic");
            p.tentative_match_thresh    = getFloat(s, "tentative_match_thresh",    p.tentative_match_thresh);
            p.tentative_confirm_hits    = getInt(s, "tentative_confirm_hits",    p.tentative_confirm_hits);
            p.tentative_max_age_seconds = getFloat(s, "tentative_max_age_seconds", p.tentative_max_age_seconds);
            p.tentative_max_miss_frames = getInt(s, "tentative_max_miss_frames", p.tentative_max_miss_frames);
            p.tentative_init_suppress_iou = getFloat(s, "tentative_init_suppress_iou", p.tentative_init_suppress_iou);
            p.tentative_fast_thresh       = getFloat(s, "tentative_fast_thresh",       p.tentative_fast_thresh);
            p.tentative_fast_hits         = getInt(s, "tentative_fast_hits",         p.tentative_fast_hits);
            p.tentative_slow_thresh       = getFloat(s, "tentative_slow_thresh",       p.tentative_slow_thresh);
            p.tentative_slow_hits_offset  = getInt(s, "tentative_slow_hits_offset",  p.tentative_slow_hits_offset);
            p.tentative_vslow_hits_offset = getInt(s, "tentative_vslow_hits_offset", p.tentative_vslow_hits_offset);
        }

        if (adv.contains("ocm"))
        {
            const auto& s = adv.at("ocm");
            p.ocm_vdc_weight  = getFloat(s, "ocm_vdc_weight",  p.ocm_vdc_weight);
            p.ocm_delta_t     = getInt(s, "ocm_delta_t",     p.ocm_delta_t);
            p.ocm_min_velocity = getFloat(s, "ocm_min_velocity", p.ocm_min_velocity);
            p.ocm_max_age     = getInt(s, "ocm_max_age",     p.ocm_max_age);
            p.ocm_maneuver_weight_scale = getFloat(s, "ocm_maneuver_weight_scale", p.ocm_maneuver_weight_scale);
        }

        if (adv.contains("mahalanobis_gating"))
        {
            const auto& s = adv.at("mahalanobis_gating");
            p.gating_thresh      = getFloat(s, "gating_thresh",      p.gating_thresh);
            p.motion_cost_weight = getFloat(s, "motion_cost_weight", p.motion_cost_weight);
            p.max_area_ratio     = getFloat(s, "max_area_ratio",     p.max_area_ratio);
        }
    }
} // namespace

namespace SushiTrack
{
    /**
     * @brief Resolves configuration file path from explicit path, environment, or search.
     * @return Resolved path string, or empty string when no file found.
     */
    std::string resolveConfigPath(const char* path)
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
        for (int depth = 0; depth <= kMaxSearchDepth && !dir.empty(); ++depth)
        {
            fs::path candidate = dir / kDefaultConfigName;
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
    bool loadConfigFromJson(const char* path, TrackerConfig& config)
    {
        const std::string resolved = resolveConfigPath(path);
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
    sushitrack_status_t sushitrack_load_params_from_json(const char* path, sushitrack_params_t* params)
    {
        if (!params)
        {
            return SUSHITRACK_INVALID_ARGUMENT;
        }

        const std::string resolved = SushiTrack::resolveConfigPath(path);
        if (resolved.empty())
        {
            SUSHITRACK_LOG_INFO("[SushiTrack] No configuration file found -- using provided parameters.");
            return SUSHITRACK_SUCCESS;
        }

        try
        {
            std::ifstream file(resolved);
            if (!file.is_open())
            {
                SUSHITRACK_LOG_WARN("[SushiTrack] Configuration file could not be opened: %s", resolved.c_str());
                return SUSHITRACK_SUCCESS;
            }

            nlohmann::json doc = nlohmann::json::parse(file, nullptr, true, true);
            overlay(doc, *params);

            SUSHITRACK_LOG_INFO("[SushiTrack] Configuration loaded from %s", resolved.c_str());
            return SUSHITRACK_SUCCESS;
        }
        catch (const std::exception& e)
        {
            SUSHITRACK_LOG_ERROR("[SushiTrack] Failed to parse configuration %s: %s", resolved.c_str(), e.what());
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
