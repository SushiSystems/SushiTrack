/**************************************************************************/
/* test_config_loader.cpp                                                 */
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
 * @file test_config_loader.cpp
 * @brief Tests the loading of sushitrack.json into the tracker parameters.
 * @author Mustafa Garip
 */

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <SushiTrack/c_api/sushitrack_c.h>

#include <c_detection_builders.hpp>
#include <captured_log.hpp>

namespace
{

    using Json = nlohmann::json;

    /**
     * @brief Resolves the path to sushitrack.json relative to the source tree.
     *
     * GoogleTest's working directory is set to ${CMAKE_SOURCE_DIR} via the
     * gtest_discover_tests WORKING_DIRECTORY argument (see tests/unit/CMakeLists.txt
     * and tests/regression/CMakeLists.txt for precedent), so the relative path
     */
    std::string config_path()
    {
        const char* env = std::getenv("SUSHITRACK_CONFIG");
        if (env != nullptr && env[0] != '\0')
            return std::string(env);
        return std::string("assets/sushitrack.json");
    }

    /**
     * @brief Safe numeric accessor: returns the JSON value or a default.
     */
    template <typename T>
    T json_get(const Json& sec, const char* key, T def)
    {
        if (sec.is_object() && sec.contains(key) && sec[key].is_number())
            return sec[key].get<T>();
        return def;
    }

    /**
     * @brief Applies a parsed JSON document to a sushitrack_params_t struct.
     *
     * Fields absent from the JSON are left untouched, so the caller is responsible
     * for invoking sushitrack_get_default_params first to establish a baseline.
     *
     */
    void apply_config_to_params(const Json& doc, sushitrack_params_t& params)
    {
        if (!doc.is_object() || !doc.contains("sushitrack"))
            return;
        const Json& root = doc["sushitrack"];
        if (!root.is_object())
            return;

        if (root.contains("basic") && root["basic"].is_object())
        {
            const Json& b = root["basic"];
            params.time_aware = json_get<int>(b, "time_aware", params.time_aware);
            params.frame_rate = json_get<int>(b, "frame_rate", params.frame_rate);
            params.track_buffer = json_get<int>(b, "track_buffer", params.track_buffer);
            params.track_thresh = json_get<float>(b, "track_thresh", params.track_thresh);
            params.high_thresh = json_get<float>(b, "high_thresh", params.high_thresh);
            params.match_thresh = json_get<float>(b, "match_thresh", params.match_thresh);
            params.enable_tentative = json_get<int>(b, "enable_tentative", params.enable_tentative);
            params.enable_reid = json_get<int>(b, "enable_reid", params.enable_reid);
            params.enable_mahalanobis =
                json_get<int>(b, "enable_mahalanobis", params.enable_mahalanobis);
        }

        if (!root.contains("advanced") || !root["advanced"].is_object())
            return;
        const Json& adv = root["advanced"];

        if (adv.contains("association") && adv["association"].is_object())
        {
            const Json& s = adv["association"];
            params.primary_assoc_mode =
                json_get<int>(s, "primary_assoc_mode", params.primary_assoc_mode);
            params.fuse_score = json_get<int>(s, "fuse_score", params.fuse_score);
            params.iou_match_thresh =
                json_get<float>(s, "iou_match_thresh", params.iou_match_thresh);
            params.unconfirmed_iou_thresh =
                json_get<float>(s, "unconfirmed_iou_thresh", params.unconfirmed_iou_thresh);
            params.duplicate_iou_thresh =
                json_get<float>(s, "duplicate_iou_thresh", params.duplicate_iou_thresh);
        }

        if (adv.contains("coordinate_system") && adv["coordinate_system"].is_object())
        {
            const Json& s = adv["coordinate_system"];
            params.enable_xyah = json_get<int>(s, "enable_xyah", params.enable_xyah);
            params.iou_type = json_get<int>(s, "iou_type", params.iou_type);
            params.iou_plus_one = json_get<int>(s, "iou_plus_one", params.iou_plus_one);
            params.kalman_type = json_get<int>(s, "kalman_type", params.kalman_type);
        }

        if (adv.contains("reid_params") && adv["reid_params"].is_object())
        {
            const Json& s = adv["reid_params"];
            params.feature_momentum =
                json_get<float>(s, "feature_momentum", params.feature_momentum);
            params.reid_high_thresh =
                json_get<float>(s, "reid_high_thresh", params.reid_high_thresh);
            params.reid_high_iou_weight =
                json_get<float>(s, "reid_high_iou_weight", params.reid_high_iou_weight);
            params.reid_low_thresh = json_get<float>(s, "reid_low_thresh", params.reid_low_thresh);
            params.reid_low_iou_weight =
                json_get<float>(s, "reid_low_iou_weight", params.reid_low_iou_weight);
            params.reid_recovery_thresh =
                json_get<float>(s, "reid_recovery_thresh", params.reid_recovery_thresh);
            params.max_feature_history =
                json_get<int>(s, "max_feature_history", params.max_feature_history);
            params.reid_direct_assignment_weight = json_get<float>(
                s, "reid_direct_assignment_weight", params.reid_direct_assignment_weight);
        }

        if (adv.contains("tentative_logic") && adv["tentative_logic"].is_object())
        {
            const Json& s = adv["tentative_logic"];
            params.tentative_match_thresh =
                json_get<float>(s, "tentative_match_thresh", params.tentative_match_thresh);
            params.tentative_confirm_hits =
                json_get<int>(s, "tentative_confirm_hits", params.tentative_confirm_hits);
            params.tentative_max_age_seconds =
                json_get<float>(s, "tentative_max_age_seconds", params.tentative_max_age_seconds);
            params.tentative_max_miss_frames =
                json_get<int>(s, "tentative_max_miss_frames", params.tentative_max_miss_frames);
            params.tentative_init_suppress_iou = json_get<float>(
                s, "tentative_init_suppress_iou", params.tentative_init_suppress_iou);
            params.tentative_fast_thresh =
                json_get<float>(s, "tentative_fast_thresh", params.tentative_fast_thresh);
            params.tentative_fast_hits =
                json_get<int>(s, "tentative_fast_hits", params.tentative_fast_hits);
            params.tentative_slow_thresh =
                json_get<float>(s, "tentative_slow_thresh", params.tentative_slow_thresh);
            params.tentative_slow_hits_offset =
                json_get<int>(s, "tentative_slow_hits_offset", params.tentative_slow_hits_offset);
            params.tentative_vslow_hits_offset =
                json_get<int>(s, "tentative_vslow_hits_offset", params.tentative_vslow_hits_offset);
        }

        if (adv.contains("mahalanobis_gating") && adv["mahalanobis_gating"].is_object())
        {
            const Json& s = adv["mahalanobis_gating"];
            params.gating_thresh = json_get<float>(s, "gating_thresh", params.gating_thresh);
            params.motion_cost_weight =
                json_get<float>(s, "motion_cost_weight", params.motion_cost_weight);
            params.max_area_ratio = json_get<float>(s, "max_area_ratio", params.max_area_ratio);
        }
    }

    /**
     * @brief Reads sushitrack.json into an nlohmann::json document.
     *
     * Returns an empty object on failure (mirrors TestInfra::parse_json
     * semantics in the regression harness).
     */
    Json load_config_json()
    {
        Json doc = Json::object();
        std::ifstream file(config_path());
        if (!file.is_open())
            return doc;
        try
        {
            doc = Json::parse(file, /*cb*/ nullptr, /*allow_exceptions*/ true,
                              /*ignore_comments*/ true);
        }
        catch (...)
        {
            return Json::object();
        }
        return doc;
    }

} // namespace

// Medium_ConfigLoaderBaselineTest

/**
 * @brief sushitrack.json must parse and contain the expected root object.
 */
TEST(Medium_ConfigLoaderBaselineTest, ConfigFileParsesAndContainsRootObject)
{
    Json doc = load_config_json();
    ASSERT_FALSE(doc.empty()) << "Could not load " << config_path()
                              << " — set SUSHITRACK_CONFIG or run tests from the source root.";
    ASSERT_TRUE(doc.contains("sushitrack"));
    EXPECT_TRUE(doc["sushitrack"].is_object());
}

/**
 * @brief The basic section enumerates every documented switch.
 */
TEST(Medium_ConfigLoaderBaselineTest, BasicSectionContainsAllDocumentedSwitches)
{
    Json doc = load_config_json();
    ASSERT_FALSE(doc.empty());
    ASSERT_TRUE(doc.contains("sushitrack"));
    ASSERT_TRUE(doc["sushitrack"].contains("basic"));
    const Json& b = doc["sushitrack"]["basic"];

    for (const char* key :
         {"time_aware", "frame_rate", "track_buffer", "track_thresh", "high_thresh", "match_thresh",
          "enable_tentative", "enable_reid", "enable_mahalanobis"})
    {
        EXPECT_TRUE(b.contains(key)) << "missing key: " << key;
    }
}

// Medium_ConfigLoaderBasicFieldsTest

/**
 * @brief Round-trip of the five numeric basic fields into Params.
 */
TEST(Medium_ConfigLoaderBasicFieldsTest, BasicNumericFieldsRoundTripIntoParams)
{
    Json doc = load_config_json();
    ASSERT_FALSE(doc.empty());

    sushitrack_params_t p{};
    sushitrack_get_default_params(&p);
    apply_config_to_params(doc, p);

    const Json& b = doc["sushitrack"]["basic"];
    EXPECT_EQ(p.frame_rate, b["frame_rate"].get<int>());
    EXPECT_EQ(p.track_buffer, b["track_buffer"].get<int>());
    EXPECT_FLOAT_EQ(p.track_thresh, b["track_thresh"].get<float>());
    EXPECT_FLOAT_EQ(p.high_thresh, b["high_thresh"].get<float>());
    EXPECT_FLOAT_EQ(p.match_thresh, b["match_thresh"].get<float>());
}

/**
 * @brief All five enable_* switches round-trip with integer type.
 */
TEST(Medium_ConfigLoaderBasicFieldsTest, EnableFlagsRoundTripIntoParams)
{
    Json doc = load_config_json();
    ASSERT_FALSE(doc.empty());

    sushitrack_params_t p{};
    sushitrack_get_default_params(&p);
    apply_config_to_params(doc, p);

    const Json& b = doc["sushitrack"]["basic"];
    EXPECT_EQ(p.enable_tentative, b["enable_tentative"].get<int>());
    EXPECT_EQ(p.enable_reid, b["enable_reid"].get<int>());
    EXPECT_EQ(p.enable_mahalanobis, b["enable_mahalanobis"].get<int>());
}

// Medium_ConfigLoaderAdvancedFieldsTest

/**
 * @brief Fixture providing a loader-populated Params instance.
 */
class Medium_ConfigLoaderAdvancedFieldsTest : public ::testing::Test
{
    protected:
        Json doc_;
        sushitrack_params_t params_{};

        void SetUp() override
        {
            doc_ = load_config_json();
            ASSERT_FALSE(doc_.empty());
            sushitrack_get_default_params(&params_);
            apply_config_to_params(doc_, params_);
        }
};

/**
 * @brief Association sub-section fields are applied to Params.
 */
TEST_F(Medium_ConfigLoaderAdvancedFieldsTest, AssociationFieldsAreApplied)
{
    const Json& s = doc_["sushitrack"]["advanced"]["association"];
    EXPECT_EQ(params_.primary_assoc_mode, s["primary_assoc_mode"].get<int>());
    EXPECT_EQ(params_.fuse_score, s["fuse_score"].get<int>());
    EXPECT_FLOAT_EQ(params_.iou_match_thresh, s["iou_match_thresh"].get<float>());
    EXPECT_FLOAT_EQ(params_.unconfirmed_iou_thresh, s["unconfirmed_iou_thresh"].get<float>());
    EXPECT_FLOAT_EQ(params_.duplicate_iou_thresh, s["duplicate_iou_thresh"].get<float>());
}

/**
 * @brief Coordinate-system fields are applied to Params.
 */
TEST_F(Medium_ConfigLoaderAdvancedFieldsTest, CoordinateSystemFieldsAreApplied)
{
    const Json& s = doc_["sushitrack"]["advanced"]["coordinate_system"];
    EXPECT_EQ(params_.enable_xyah, s["enable_xyah"].get<int>());
    EXPECT_EQ(params_.iou_type, s["iou_type"].get<int>());
    EXPECT_EQ(params_.iou_plus_one, s["iou_plus_one"].get<int>());
    EXPECT_EQ(params_.kalman_type, s["kalman_type"].get<int>());
}

/**
 * @brief ReID fields, including the documented but in past tests-untested
 *        reid_direct_assignment_weight (Verified Omission #2), are applied.
 */
TEST_F(Medium_ConfigLoaderAdvancedFieldsTest, ReIDFieldsIncludingDirectAssignmentWeightAreApplied)
{
    const Json& s = doc_["sushitrack"]["advanced"]["reid_params"];
    EXPECT_FLOAT_EQ(params_.feature_momentum, s["feature_momentum"].get<float>());
    EXPECT_FLOAT_EQ(params_.reid_high_thresh, s["reid_high_thresh"].get<float>());
    EXPECT_FLOAT_EQ(params_.reid_high_iou_weight, s["reid_high_iou_weight"].get<float>());
    EXPECT_FLOAT_EQ(params_.reid_low_thresh, s["reid_low_thresh"].get<float>());
    EXPECT_FLOAT_EQ(params_.reid_low_iou_weight, s["reid_low_iou_weight"].get<float>());
    EXPECT_FLOAT_EQ(params_.reid_recovery_thresh, s["reid_recovery_thresh"].get<float>());
    EXPECT_EQ(params_.max_feature_history, s["max_feature_history"].get<int>());
    EXPECT_FLOAT_EQ(params_.reid_direct_assignment_weight,
                    s["reid_direct_assignment_weight"].get<float>());
}

/**
 * @brief Tentative-logic fast/slow/vslow tiers are applied — the in past tests
 *        untested fields called out in Omission #2.
 */
TEST_F(Medium_ConfigLoaderAdvancedFieldsTest, TentativeLogicTiersAreApplied)
{
    const Json& s = doc_["sushitrack"]["advanced"]["tentative_logic"];
    EXPECT_FLOAT_EQ(params_.tentative_match_thresh, s["tentative_match_thresh"].get<float>());
    EXPECT_EQ(params_.tentative_confirm_hits, s["tentative_confirm_hits"].get<int>());
    EXPECT_FLOAT_EQ(params_.tentative_max_age_seconds, s["tentative_max_age_seconds"].get<float>());
    EXPECT_EQ(params_.tentative_max_miss_frames, s["tentative_max_miss_frames"].get<int>());
    EXPECT_FLOAT_EQ(params_.tentative_init_suppress_iou,
                    s["tentative_init_suppress_iou"].get<float>());
    EXPECT_FLOAT_EQ(params_.tentative_fast_thresh, s["tentative_fast_thresh"].get<float>());
    EXPECT_EQ(params_.tentative_fast_hits, s["tentative_fast_hits"].get<int>());
    EXPECT_FLOAT_EQ(params_.tentative_slow_thresh, s["tentative_slow_thresh"].get<float>());
    EXPECT_EQ(params_.tentative_slow_hits_offset, s["tentative_slow_hits_offset"].get<int>());
    EXPECT_EQ(params_.tentative_vslow_hits_offset, s["tentative_vslow_hits_offset"].get<int>());
}

/**
 * @brief Mahalanobis-gating fields are applied to Params.
 */
TEST_F(Medium_ConfigLoaderAdvancedFieldsTest, MahalanobisGatingFieldsAreApplied)
{
    const Json& s = doc_["sushitrack"]["advanced"]["mahalanobis_gating"];
    EXPECT_FLOAT_EQ(params_.gating_thresh, s["gating_thresh"].get<float>());
    EXPECT_FLOAT_EQ(params_.motion_cost_weight, s["motion_cost_weight"].get<float>());
    EXPECT_FLOAT_EQ(params_.max_area_ratio, s["max_area_ratio"].get<float>());
}

// Medium_ConfigLoaderDefaultsTest

/**
 * @brief A document missing the "basic" section must not perturb the basic
 *        fields of Params — they should remain at the GetDefaultParams baseline.
 */
TEST(Medium_ConfigLoaderDefaultsTest, MissingBasicSectionLeavesDefaults)
{
    Json doc = Json::object();
    doc["sushitrack"]["advanced"]["association"]["primary_assoc_mode"] = 0;

    sushitrack_params_t p{};
    sushitrack_get_default_params(&p);
    const int default_frame_rate = p.frame_rate;
    apply_config_to_params(doc, p);

    EXPECT_EQ(p.frame_rate, default_frame_rate);
    EXPECT_EQ(p.primary_assoc_mode, 0);
}

/**
 * @brief A completely empty document must leave Params equal to defaults.
 */
TEST(Medium_ConfigLoaderDefaultsTest, EmptyDocumentLeavesAllDefaults)
{
    Json doc = Json::object();

    sushitrack_params_t before{};
    sushitrack_get_default_params(&before);
    sushitrack_params_t after = before;
    apply_config_to_params(doc, after);

    EXPECT_EQ(after.frame_rate, before.frame_rate);
    EXPECT_EQ(after.track_buffer, before.track_buffer);
    EXPECT_FLOAT_EQ(after.match_thresh, before.match_thresh);
    EXPECT_EQ(after.enable_reid, before.enable_reid);
}

// Medium_ConfigLoaderTrackerSmokeTest

/**
 * @brief Params produced by the loader must build a tracker successfully.
 */
TEST(Medium_ConfigLoaderTrackerSmokeTest, LoadedParamsBuildTrackerSuccessfully)
{
    Json doc = load_config_json();
    ASSERT_FALSE(doc.empty());

    sushitrack_params_t p{};
    sushitrack_get_default_params(&p);
    apply_config_to_params(doc, p);

    sushitrack_tracker_t hdl = sushitrack_create(&p);
    ASSERT_NE(hdl, nullptr);
    sushitrack_destroy(hdl);
}

/**
 * @brief Params produced by the loader must survive a one-detection Update.
 */
TEST(Medium_ConfigLoaderTrackerSmokeTest, LoadedParamsSurviveSingleDetectionUpdate)
{
    Json doc = load_config_json();
    ASSERT_FALSE(doc.empty());

    sushitrack_params_t p{};
    sushitrack_get_default_params(&p);
    apply_config_to_params(doc, p);
    p.time_aware = 1; // ensure dt argument is honoured

    sushitrack_tracker_t hdl = sushitrack_create(&p);
    ASSERT_NE(hdl, nullptr);

    sushitrack_object_t d{};
    d.x = 100.0f;
    d.y = 200.0f;
    d.width = 50.0f;
    d.height = 100.0f;
    d.score = 0.9f;
    d.label = 1;
    std::vector<sushitrack_track_t> out(4);
    int n = CDetections::update_frame(hdl, &d, 1, out.data(), 4, 0.04f);
    EXPECT_GE(n, 0);

    sushitrack_destroy(hdl);
}

// Medium_ConfigLoaderLibraryApiTest

/**
 * @brief The library-owned loader (sushitrack_load_params_from_json) must produce
 *        the same Params as the in-test reference parser for the shipped file.
 *
 * This is the parity guard for the production code path: sushitrack.json is now
 * parsed inside the library, not by the harness, so the two must agree field by
 * field. The reference parser above stays as an independent oracle.
 */
TEST(Medium_ConfigLoaderLibraryApiTest, LibraryLoaderMatchesReferenceParser)
{
    Json doc = load_config_json();
    ASSERT_FALSE(doc.empty());

    sushitrack_params_t reference{};
    sushitrack_get_default_params(&reference);
    apply_config_to_params(doc, reference);

    sushitrack_params_t loaded{};
    sushitrack_get_default_params(&loaded);
    ASSERT_EQ(sushitrack_load_params_from_json(config_path().c_str(), &loaded), SUSHITRACK_SUCCESS);

    EXPECT_EQ(loaded.enable_reid, reference.enable_reid);
    EXPECT_EQ(loaded.frame_rate, reference.frame_rate);
    EXPECT_EQ(loaded.primary_assoc_mode, reference.primary_assoc_mode);
    EXPECT_FLOAT_EQ(loaded.match_thresh, reference.match_thresh);
    EXPECT_FLOAT_EQ(loaded.ocm_vdc_weight, reference.ocm_vdc_weight);
}

/**
 * @brief A NULL params pointer must be rejected without dereferencing.
 */
TEST(Medium_ConfigLoaderLibraryApiTest, NullParamsAreRejected)
{
    EXPECT_EQ(sushitrack_load_params_from_json(config_path().c_str(), nullptr),
              SUSHITRACK_INVALID_ARGUMENT);
}

/**
 * @brief A named file that does not exist is refused: the status says so, params stays at its
 *        defaults and one ERROR names the file.
 */
TEST(Medium_ConfigLoaderLibraryApiTest, MissingNamedFileIsRefusedAndLogged)
{
    SushiTrack::CapturedLog log;
    sushitrack_params_t before{};
    sushitrack_get_default_params(&before);
    sushitrack_params_t after = before;

    EXPECT_EQ(sushitrack_load_params_from_json("this/path/does/not/exist.json", &after),
              SUSHITRACK_INVALID_ARGUMENT);
    EXPECT_EQ(after.frame_rate, before.frame_rate);
    EXPECT_EQ(after.enable_reid, before.enable_reid);
    EXPECT_EQ(log.count("this/path/does/not/exist.json, which does not exist"), 1);
    EXPECT_EQ(log.level_of("does not exist"), SushiLog::LogLevel::Error);
}

namespace
{
    /**
     * @brief Sets an environment variable for one scope and puts the old value back.
     */
    class ScopedEnvironment final
    {
        public:
            /// @brief Sets @p name to @p value.
            ScopedEnvironment(const char* name, const std::string& value) : name_(name)
            {
                const char* old = std::getenv(name);
                had_old_ = old != nullptr;
                if (had_old_)
                    old_ = old;
                set(value);
            }

            ScopedEnvironment(const ScopedEnvironment&) = delete;
            ScopedEnvironment& operator=(const ScopedEnvironment&) = delete;
            ScopedEnvironment(ScopedEnvironment&&) = delete;
            ScopedEnvironment& operator=(ScopedEnvironment&&) = delete;

            /// @brief Restores the value the variable had.
            ~ScopedEnvironment()
            {
                set(old_);
            }

        private:
            /// @brief Sets the variable, or clears it when it had no value and @p value is empty.
            void set(const std::string& value) const
            {
#ifdef _WIN32
                _putenv_s(name_.c_str(), value.c_str());
#else
                if (value.empty() && !had_old_)
                    unsetenv(name_.c_str());
                else
                    setenv(name_.c_str(), value.c_str(), 1);
#endif
            }

            /// @brief The variable's name.
            std::string name_;
            /// @brief Whether the variable had a value before.
            bool had_old_ = false;
            /// @brief The value it had.
            std::string old_;
    };

    /**
     * @brief Writes @p text to a file named @p name in a temporary folder and returns its path.
     */
    std::string write_temp_config(const std::string& name, const std::string& text)
    {
        const std::filesystem::path folder =
            std::filesystem::temp_directory_path() / "sushitrack_config_loader_tests";
        std::filesystem::create_directories(folder);
        const std::filesystem::path path = folder / name;
        std::ofstream(path) << text;
        return path.string();
    }
} // namespace

/**
 * @brief SUSHITRACK_CONFIG naming a file that does not exist is refused like a named path, so a
 *        typo in the variable does not fall back to a search silently.
 */
TEST(Medium_ConfigLoaderLibraryApiTest, MissingFileNamedByTheEnvironmentIsRefusedAndLogged)
{
    const ScopedEnvironment environment("SUSHITRACK_CONFIG", "no/such/sushitrack_env.json");
    SushiTrack::CapturedLog log;
    sushitrack_params_t params{};
    sushitrack_get_default_params(&params);

    EXPECT_EQ(sushitrack_load_params_from_json(nullptr, &params), SUSHITRACK_INVALID_ARGUMENT);
    EXPECT_EQ(log.count("SUSHITRACK_CONFIG names no/such/sushitrack_env.json"), 1);
}

/**
 * @brief With no path, no variable and no file in the search, the load succeeds and says so at
 *        INFO.
 */
TEST(Medium_ConfigLoaderLibraryApiTest, NothingNamedAndNothingFoundSucceedsAtInfo)
{
    const ScopedEnvironment environment("SUSHITRACK_CONFIG", "");
    const std::filesystem::path previous = std::filesystem::current_path();
    const std::filesystem::path empty_folder =
        std::filesystem::temp_directory_path() / "sushitrack_config_loader_empty";
    std::filesystem::create_directories(empty_folder);
    std::filesystem::current_path(empty_folder);
    SushiTrack::CapturedLog log;
    sushitrack_params_t params{};
    sushitrack_get_default_params(&params);

    const sushitrack_status_t status = sushitrack_load_params_from_json(nullptr, &params);

    std::filesystem::current_path(previous);
    EXPECT_EQ(status, SUSHITRACK_SUCCESS);
    EXPECT_EQ(log.level_of("No configuration file found"), SushiLog::LogLevel::Info);
}

/**
 * @brief A value of the wrong kind is refused with its dotted path, params stays untouched and
 *        one ERROR carries the same path.
 */
TEST(Medium_ConfigLoaderStrictReaderTest, WrongTypedValueIsRefusedWithItsPath)
{
    SushiTrack::CapturedLog log;
    const std::string path = write_temp_config(
        "wrong_kind.json",
        R"({"sushitrack": {"basic": {"frame_rate": "fast", "track_buffer": 90}}})");
    sushitrack_params_t before{};
    sushitrack_get_default_params(&before);
    sushitrack_params_t after = before;

    EXPECT_EQ(sushitrack_load_params_from_json(path.c_str(), &after), SUSHITRACK_INVALID_ARGUMENT);
    EXPECT_EQ(after.frame_rate, before.frame_rate);
    EXPECT_EQ(after.track_buffer, before.track_buffer);
    EXPECT_EQ(log.count("sushitrack.basic.frame_rate: expected an integer, got a string"), 1);
}

/**
 * @brief A number with a fraction in an integer field is refused, not truncated.
 */
TEST(Medium_ConfigLoaderStrictReaderTest, FractionInAnIntegerFieldIsRefused)
{
    SushiTrack::CapturedLog log;
    const std::string path =
        write_temp_config("fraction.json", R"({"basic": {"frame_rate": 29.97}})");
    sushitrack_params_t params{};
    sushitrack_get_default_params(&params);
    const int before = params.frame_rate;

    EXPECT_EQ(sushitrack_load_params_from_json(path.c_str(), &params),
              SUSHITRACK_INVALID_ARGUMENT);
    EXPECT_EQ(params.frame_rate, before);
    EXPECT_EQ(log.count("basic.frame_rate: expected an integer, got 29.97"), 1);
}

/**
 * @brief A key no section reads is refused, whether it sits in a section or beside one.
 */
TEST(Medium_ConfigLoaderStrictReaderTest, UnknownKeyIsRefusedWhereverItSits)
{
    SushiTrack::CapturedLog log;
    sushitrack_params_t params{};
    sushitrack_get_default_params(&params);

    const std::string in_section =
        write_temp_config("typo_in_section.json", R"({"basic": {"frame_rat": 30}})");
    EXPECT_EQ(sushitrack_load_params_from_json(in_section.c_str(), &params),
              SUSHITRACK_INVALID_ARGUMENT);
    EXPECT_EQ(log.count("basic.frame_rat: unknown key"), 1);

    const std::string in_advanced = write_temp_config(
        "typo_in_advanced.json", R"({"sushitrack": {"advanced": {"asociation": {}}}})");
    EXPECT_EQ(sushitrack_load_params_from_json(in_advanced.c_str(), &params),
              SUSHITRACK_INVALID_ARGUMENT);
    EXPECT_EQ(log.count("sushitrack.advanced.asociation: unknown key"), 1);

    const std::string at_root =
        write_temp_config("typo_at_root.json", R"({"sushitrack": {}, "extra": 1})");
    EXPECT_EQ(sushitrack_load_params_from_json(at_root.c_str(), &params),
              SUSHITRACK_INVALID_ARGUMENT);
    EXPECT_EQ(log.count("extra: unknown key"), 1);
}

/**
 * @brief A flag written true or false and an integer written with a zero fraction are accepted.
 */
TEST(Medium_ConfigLoaderStrictReaderTest, FlagsAndWholeNumbersApply)
{
    const std::string path = write_temp_config(
        "flags.json",
        R"({"basic": {"enable_reid": true, "frame_rate": 30.0, "match_thresh": 0.6}})");
    sushitrack_params_t params{};
    sushitrack_get_default_params(&params);

    EXPECT_EQ(sushitrack_load_params_from_json(path.c_str(), &params), SUSHITRACK_SUCCESS);
    EXPECT_EQ(params.enable_reid, 1);
    EXPECT_EQ(params.frame_rate, 30);
    EXPECT_FLOAT_EQ(params.match_thresh, 0.6f);
}

/**
 * @brief A document that is not valid JSON keeps its RUNTIME_ERROR status and logs one ERROR.
 */
TEST(Medium_ConfigLoaderStrictReaderTest, MalformedJsonReturnsRuntimeError)
{
    SushiTrack::CapturedLog log;
    const std::string path = write_temp_config("malformed.json", "{ not json");
    sushitrack_params_t params{};
    sushitrack_get_default_params(&params);

    EXPECT_EQ(sushitrack_load_params_from_json(path.c_str(), &params), SUSHITRACK_RUNTIME_ERROR);
    EXPECT_EQ(log.count("Failed to parse configuration"), 1);
}

/**
 * @brief sushitrack_create_from_config returns no tracker for a refused file, where it once built
 *        one from the defaults without a word.
 */
TEST(Medium_ConfigLoaderCreateTest, RefusedFileGivesNoTracker)
{
    SushiTrack::CapturedLog log;
    const std::string path =
        write_temp_config("refused_for_create.json", R"({"basic": {"frame_rate": "x"}})");

    EXPECT_EQ(sushitrack_create_from_config(path.c_str()), nullptr);
    EXPECT_EQ(log.count("refused_for_create.json"), 1);
    EXPECT_EQ(sushitrack_create_from_config("no/such/file.json"), nullptr);
}
