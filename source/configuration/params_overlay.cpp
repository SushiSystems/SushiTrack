/**************************************************************************/
/* params_overlay.cpp                                                     */
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
 * @file params_overlay.cpp
 * @brief Implements overlay_params over a table of the sections and fields the format holds.
 * @author Mustafa Garip
 */

#include <configuration/params_overlay.hpp>

#include <cmath>
#include <limits>
#include <string>
#include <vector>

namespace SushiTrack::Detail
{
    namespace
    {
        /// @brief Names an integer field of sushitrack_params_t by its JSON key.
        struct IntField
        {
                const char* key;                 /**< The JSON key. */
                int sushitrack_params_t::* slot; /**< The field it fills. */
        };

        /// @brief Names a float field of sushitrack_params_t by its JSON key.
        struct FloatField
        {
                const char* key;                   /**< The JSON key. */
                float sushitrack_params_t::* slot; /**< The field it fills. */
        };

        /// @brief Holds the fields one section of the document may carry.
        struct Section
        {
                std::string name;              /**< The section's key. */
                std::vector<IntField> ints;    /**< Its integer and flag fields. */
                std::vector<FloatField> floats; /**< Its real fields. */
        };

        /// @brief The `basic` section's key.
        constexpr char K_BASIC[] = "basic";
        /// @brief The `advanced` object's key; it holds the other sections.
        constexpr char K_ADVANCED[] = "advanced";
        /// @brief The root key that may wrap the whole document.
        constexpr char K_ROOT[] = "sushitrack";

        /// @brief Returns the fields of `basic`.
        const Section& basic_section()
        {
            static const Section section{
                K_BASIC,
                {{"time_aware", &sushitrack_params_t::time_aware},
                 {"frame_rate", &sushitrack_params_t::frame_rate},
                 {"track_buffer", &sushitrack_params_t::track_buffer},
                 {"enable_tentative", &sushitrack_params_t::enable_tentative},
                 {"enable_reid", &sushitrack_params_t::enable_reid},
                 {"enable_mahalanobis", &sushitrack_params_t::enable_mahalanobis},
                 {"enable_ocm", &sushitrack_params_t::enable_ocm},
                 {"enable_oru", &sushitrack_params_t::enable_oru}},
                {{"track_thresh", &sushitrack_params_t::track_thresh},
                 {"high_thresh", &sushitrack_params_t::high_thresh},
                 {"match_thresh", &sushitrack_params_t::match_thresh}}};
            return section;
        }

        /// @brief Returns the sections under `advanced`.
        const std::vector<Section>& advanced_sections()
        {
            static const std::vector<Section> sections = {
                {"association",
                 {{"primary_assoc_mode", &sushitrack_params_t::primary_assoc_mode},
                  {"fuse_score", &sushitrack_params_t::fuse_score}},
                 {{"iou_match_thresh", &sushitrack_params_t::iou_match_thresh},
                  {"unconfirmed_iou_thresh", &sushitrack_params_t::unconfirmed_iou_thresh},
                  {"duplicate_iou_thresh", &sushitrack_params_t::duplicate_iou_thresh}}},
                {"coordinate_system",
                 {{"enable_xyah", &sushitrack_params_t::enable_xyah},
                  {"iou_type", &sushitrack_params_t::iou_type},
                  {"iou_plus_one", &sushitrack_params_t::iou_plus_one},
                  {"kalman_type", &sushitrack_params_t::kalman_type}},
                 {}},
                {"reid_params",
                 {{"max_feature_history", &sushitrack_params_t::max_feature_history}},
                 {{"feature_momentum", &sushitrack_params_t::feature_momentum},
                  {"reid_high_thresh", &sushitrack_params_t::reid_high_thresh},
                  {"reid_high_iou_weight", &sushitrack_params_t::reid_high_iou_weight},
                  {"reid_low_thresh", &sushitrack_params_t::reid_low_thresh},
                  {"reid_low_iou_weight", &sushitrack_params_t::reid_low_iou_weight},
                  {"reid_recovery_thresh", &sushitrack_params_t::reid_recovery_thresh},
                  {"reid_direct_assignment_weight",
                   &sushitrack_params_t::reid_direct_assignment_weight}}},
                {"tentative_logic",
                 {{"tentative_confirm_hits", &sushitrack_params_t::tentative_confirm_hits},
                  {"tentative_max_miss_frames", &sushitrack_params_t::tentative_max_miss_frames},
                  {"tentative_fast_hits", &sushitrack_params_t::tentative_fast_hits},
                  {"tentative_slow_hits_offset",
                   &sushitrack_params_t::tentative_slow_hits_offset},
                  {"tentative_vslow_hits_offset",
                   &sushitrack_params_t::tentative_vslow_hits_offset}},
                 {{"tentative_match_thresh", &sushitrack_params_t::tentative_match_thresh},
                  {"tentative_max_age_seconds", &sushitrack_params_t::tentative_max_age_seconds},
                  {"tentative_init_suppress_iou",
                   &sushitrack_params_t::tentative_init_suppress_iou},
                  {"tentative_fast_thresh", &sushitrack_params_t::tentative_fast_thresh},
                  {"tentative_slow_thresh", &sushitrack_params_t::tentative_slow_thresh}}},
                {"ocm",
                 {{"ocm_delta_t", &sushitrack_params_t::ocm_delta_t},
                  {"ocm_max_age", &sushitrack_params_t::ocm_max_age}},
                 {{"ocm_vdc_weight", &sushitrack_params_t::ocm_vdc_weight},
                  {"ocm_min_velocity", &sushitrack_params_t::ocm_min_velocity},
                  {"ocm_maneuver_weight_scale",
                   &sushitrack_params_t::ocm_maneuver_weight_scale}}},
                {"mahalanobis_gating",
                 {},
                 {{"gating_thresh", &sushitrack_params_t::gating_thresh},
                  {"motion_cost_weight", &sushitrack_params_t::motion_cost_weight},
                  {"max_area_ratio", &sushitrack_params_t::max_area_ratio}}}};
            return sections;
        }

        /// @brief Returns how a JSON value reads in a refusal: "a string", "null" and so on.
        std::string kind_of(const nlohmann::json& value)
        {
            if (value.is_string())
                return "a string";
            if (value.is_boolean())
                return "a boolean";
            if (value.is_null())
                return "null";
            if (value.is_array())
                return "an array";
            if (value.is_object())
                return "an object";
            return "a number";
        }

        /// @brief Returns @p path joined with @p key.
        std::string join(const std::string& path, const std::string& key)
        {
            return path.empty() ? key : path + "." + key;
        }

        /// @brief Returns `<path>: <reason>`.
        std::string refusal(const std::string& path, const std::string& reason)
        {
            return path + ": " + reason;
        }

        /// @brief Reads @p value as an integer, or a flag written true or false.
        std::string read_int(const nlohmann::json& value, const std::string& path, int& out)
        {
            if (value.is_boolean())
            {
                out = value.get<bool>() ? 1 : 0;
                return {};
            }
            if (!value.is_number())
                return refusal(path, "expected an integer, got " + kind_of(value));
            const double number = value.get<double>();
            const bool whole = std::nearbyint(number) == number &&
                               number >= static_cast<double>(std::numeric_limits<int>::min()) &&
                               number <= static_cast<double>(std::numeric_limits<int>::max());
            if (!whole)
                return refusal(path, "expected an integer, got " + value.dump());
            out = static_cast<int>(number);
            return {};
        }

        /// @brief Reads @p value as a real.
        std::string read_float(const nlohmann::json& value, const std::string& path, float& out)
        {
            if (!value.is_number())
                return refusal(path, "expected a number, got " + kind_of(value));
            out = value.get<float>();
            return {};
        }

        /// @brief Applies one section object to @p params, or returns its first refusal.
        std::string apply_section(const nlohmann::json& node, const std::string& path,
                                  const Section& section, sushitrack_params_t& params)
        {
            if (!node.is_object())
                return refusal(path, "expected an object, got " + kind_of(node));
            for (auto entry = node.begin(); entry != node.end(); ++entry)
            {
                const std::string key_path = join(path, entry.key());
                bool known = false;
                std::string error;
                for (const IntField& field : section.ints)
                {
                    if (entry.key() == field.key)
                    {
                        known = true;
                        error = read_int(entry.value(), key_path, params.*field.slot);
                    }
                }
                for (const FloatField& field : section.floats)
                {
                    if (entry.key() == field.key)
                    {
                        known = true;
                        error = read_float(entry.value(), key_path, params.*field.slot);
                    }
                }
                if (!known)
                    return refusal(key_path, "unknown key");
                if (!error.empty())
                    return error;
            }
            return {};
        }

        /// @brief Applies the `advanced` object, which holds only the named sections.
        std::string apply_advanced(const nlohmann::json& node, const std::string& path,
                                   sushitrack_params_t& params)
        {
            if (!node.is_object())
                return refusal(path, "expected an object, got " + kind_of(node));
            for (auto entry = node.begin(); entry != node.end(); ++entry)
            {
                const std::string key_path = join(path, entry.key());
                const Section* found = nullptr;
                for (const Section& section : advanced_sections())
                {
                    if (entry.key() == section.name)
                        found = &section;
                }
                if (found == nullptr)
                    return refusal(key_path, "unknown key");
                std::string error = apply_section(entry.value(), key_path, *found, params);
                if (!error.empty())
                    return error;
            }
            return {};
        }

        /// @brief Applies the `sushitrack` object: `basic` and `advanced` and nothing else.
        std::string apply_root(const nlohmann::json& node, const std::string& path,
                               sushitrack_params_t& params)
        {
            if (!node.is_object())
                return refusal(K_ROOT, "expected an object, got " + kind_of(node));
            for (auto entry = node.begin(); entry != node.end(); ++entry)
            {
                const std::string key_path = join(path, entry.key());
                std::string error;
                if (entry.key() == K_BASIC)
                    error = apply_section(entry.value(), key_path, basic_section(), params);
                else if (entry.key() == K_ADVANCED)
                    error = apply_advanced(entry.value(), key_path, params);
                else
                    error = refusal(key_path, "unknown key");
                if (!error.empty())
                    return error;
            }
            return {};
        }
    } // namespace

    std::string overlay_params(const nlohmann::json& document, sushitrack_params_t& params)
    {
        if (!document.is_object())
            return refusal("document", "expected an object, got " + kind_of(document));

        sushitrack_params_t candidate = params;
        std::string error;
        if (document.contains(K_ROOT))
        {
            for (auto entry = document.begin(); entry != document.end() && error.empty(); ++entry)
            {
                if (entry.key() != K_ROOT)
                    error = refusal(entry.key(), "unknown key");
            }
            if (error.empty())
                error = apply_root(document.at(K_ROOT), K_ROOT, candidate);
        }
        else
        {
            error = apply_root(document, "", candidate);
        }

        if (error.empty())
            params = candidate;
        return error;
    }
} // namespace SushiTrack::Detail
