/**
 * @file      ConfigLoader.cpp
 * @brief     JSON configuration loading for the SushiTrack C API.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      23.06.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 *
 * @details   Binds a runtime JSON configuration file to the library itself,
 *            independently of any test harness. A deployed build can be retuned
 *            by editing sushitrack.json with no recompilation. The loader overlays
 *            file values onto an existing SushiTrack_Params structure, so any key
 *            that is absent keeps the compiled default the caller passed in.
 */

#include "SushiTrackC.h"
#include "SushiTrack/Logger.h"

#include <nlohmann/json.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

namespace
{
    namespace fs = std::filesystem;

    /** @brief Canonical file name searched for when no explicit path is given. */
    constexpr const char* kDefaultConfigName = "sushitrack.json";

    /** @brief Maximum number of parent directories walked during the upward search. */
    constexpr int kMaxSearchDepth = 8;

    /**
     * @brief Resolves the configuration file to load.
     *
     * Resolution order:
     *   1. An explicit, non-empty @p path that exists on disk.
     *   2. The SUSHITRACK_CONFIG environment variable, when it points at an
     *      existing file.
     *   3. An upward walk from the current working directory looking for
     *      "sushitrack.json" (so a binary launched from build/bin still finds the
     *      file at the repository or deployment root).
     *
     * @param path Optional explicit path (may be null or empty).
     * @return Resolved path, or an empty string when nothing was found.
     */
    std::string resolveConfigPath(const char* path)
    {
        std::error_code ec;

        if (path && path[0] != '\0')
        {
            if (fs::exists(path, ec)) return std::string(path);
            return std::string();
        }

        if (const char* env = std::getenv("SUSHITRACK_CONFIG"))
        {
            if (env[0] != '\0' && fs::exists(env, ec))
                return std::string(env);
        }

        fs::path dir = fs::current_path(ec);
        for (int depth = 0; depth <= kMaxSearchDepth && !dir.empty(); ++depth)
        {
            fs::path candidate = dir / kDefaultConfigName;
            if (fs::exists(candidate, ec))
                return candidate.string();

            if (!dir.has_parent_path()) break;
            fs::path parent = dir.parent_path();
            if (parent == dir) break;
            dir = parent;
        }

        return std::string();
    }

    /**
     * @brief Reads a float-convertible value, falling back to a default.
     *
     * @param sec Containing JSON object.
     * @param key Field name to read.
     * @param def Default returned when the key is missing or non-numeric.
     * @return Parsed value or @p def.
     */
    float getFloat(const nlohmann::json& sec, const char* key, float def)
    {
        auto it = sec.find(key);
        if (it == sec.end() || !it->is_number()) return def;
        return it->get<float>();
    }

    /**
     * @brief Reads an integer value, falling back to a default.
     *
     * @param sec Containing JSON object.
     * @param key Field name to read.
     * @param def Default returned when the key is missing or non-numeric.
     * @return Parsed value or @p def.
     */
    int getInt(const nlohmann::json& sec, const char* key, int def)
    {
        auto it = sec.find(key);
        if (it == sec.end() || !it->is_number()) return def;
        return static_cast<int>(it->get<double>());
    }

    /**
     * @brief Overlays the parsed document onto a parameters structure.
     *
     * Accepts either a document wrapped in a top-level "sushitrack" object or a
     * document whose root already holds the "basic"/"advanced" sections, so a
     * dedicated sushitrack.json and a shared multi-tracker file are both valid.
     *
     * @param doc Parsed JSON document.
     * @param p   Parameters structure updated in place.
     */
    void overlay(const nlohmann::json& doc, SushiTrack_Params& p)
    {
        const nlohmann::json& ds =
            doc.contains("sushitrack") ? doc.at("sushitrack") : doc;

        if (ds.contains("basic"))
        {
            const auto& b = ds.at("basic");
            p.time_aware                 = getInt(b, "time_aware",                 p.time_aware);
            p.frame_rate                 = getInt(b, "frame_rate",                 p.frame_rate);
            p.track_buffer               = getInt(b, "track_buffer",               p.track_buffer);
            p.track_thresh               = getFloat(b, "track_thresh",             p.track_thresh);
            p.high_thresh                = getFloat(b, "high_thresh",              p.high_thresh);
            p.match_thresh               = getFloat(b, "match_thresh",             p.match_thresh);
            p.enable_tentative           = getInt(b, "enable_tentative",           p.enable_tentative);
            p.enable_fsm                 = getInt(b, "enable_fsm",                 p.enable_fsm);
            p.enable_reid                = getInt(b, "enable_reid",                p.enable_reid);
            p.enable_mahalanobis         = getInt(b, "enable_mahalanobis",         p.enable_mahalanobis);
        }

        if (!ds.contains("advanced")) return;
        const auto& adv = ds.at("advanced");

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

        if (adv.contains("fsm_params"))
        {
            const auto& s = adv.at("fsm_params");
            p.fsm_stationary_vel_thresh = getFloat(s, "fsm_stationary_vel_thresh", p.fsm_stationary_vel_thresh);
            p.fsm_maneuver_accel_thresh = getFloat(s, "fsm_maneuver_accel_thresh", p.fsm_maneuver_accel_thresh);
            p.fsm_reference_height      = getFloat(s, "fsm_reference_height",      p.fsm_reference_height);
        }

        if (adv.contains("tentative_logic"))
        {
            const auto& s = adv.at("tentative_logic");
            p.tentative_match_thresh    = getFloat(s, "tentative_match_thresh",    p.tentative_match_thresh);
            p.tentative_confirm_hits    = getInt(s, "tentative_confirm_hits",    p.tentative_confirm_hits);
            p.tentative_max_age_seconds = getFloat(s, "tentative_max_age_seconds", p.tentative_max_age_seconds);
            p.tentative_fast_thresh       = getFloat(s, "tentative_fast_thresh",       p.tentative_fast_thresh);
            p.tentative_fast_hits         = getInt(s, "tentative_fast_hits",         p.tentative_fast_hits);
            p.tentative_slow_thresh       = getFloat(s, "tentative_slow_thresh",       p.tentative_slow_thresh);
            p.tentative_slow_hits_offset  = getInt(s, "tentative_slow_hits_offset",  p.tentative_slow_hits_offset);
            p.tentative_vslow_hits_offset = getInt(s, "tentative_vslow_hits_offset", p.tentative_vslow_hits_offset);
        }

        if (adv.contains("mahalanobis_gating"))
        {
            const auto& s = adv.at("mahalanobis_gating");
            p.proximity_thresh   = getFloat(s, "proximity_thresh",   p.proximity_thresh);
            p.gating_thresh      = getFloat(s, "gating_thresh",      p.gating_thresh);
            p.motion_cost_weight = getFloat(s, "motion_cost_weight", p.motion_cost_weight);
            p.max_area_ratio     = getFloat(s, "max_area_ratio",     p.max_area_ratio);
        }
    }
} // namespace

extern "C"
{
    /**
     * @brief Overlays a JSON configuration file onto a parameters structure.
     *
     * The caller is expected to seed @p params first (typically via
     * SushiTrack_GetDefaultParams), then call this to apply any file overrides.
     * A missing file is not an error: the parameters are left untouched and
     * SUSHITRACK_SUCCESS is returned, so callers can treat configuration as
     * optional. A malformed file returns SUSHITRACK_RUNTIME_ERROR with the
     * parameters left untouched.
     *
     * @param path   Explicit file path, or NULL to use the resolution order
     *               (SUSHITRACK_CONFIG env var, then an upward search for
     *               sushitrack.json from the working directory).
     * @param params Parameters structure updated in place.
     * @return Status code; SUSHITRACK_SUCCESS also covers the no-file case.
     */
    SushiTrack_Status SushiTrack_LoadParamsFromJson(const char* path, SushiTrack_Params* params)
    {
        if (!params) return SUSHITRACK_INVALID_ARGUMENT;

        const std::string resolved = resolveConfigPath(path);
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

            // Permit comments and trailing commas so operators can annotate the file.
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
     * @brief Instantiates a tracker from a JSON configuration file.
     *
     * Convenience wrapper equivalent to seeding defaults, overlaying the file,
     * and calling SushiTrack_Create. When the file is absent the tracker is still
     * created on compiled defaults.
     *
     * @param path Explicit file path, or NULL to use the standard resolution
     *             order (see SushiTrack_LoadParamsFromJson).
     * @return Tracker handle, or NULL on failure.
     */
    SushiTrackHdl SushiTrack_CreateFromConfig(const char* path)
    {
        SushiTrack_Params params;
        SushiTrack_GetDefaultParams(&params);
        SushiTrack_LoadParamsFromJson(path, &params);
        return SushiTrack_Create(&params);
    }
} // extern "C"
