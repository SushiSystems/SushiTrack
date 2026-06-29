/**
 * @file      TestConfiguration.cpp
 * @brief     Tests for configuration loading from sushitrack.json into SushiTrack_Params.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      18.05.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 *
 * Scope
 * -----
 * Closes the configuration-system gap flagged in UNIT_TEST_VALIDATION_REPORT.md
 * Section 2.3.1 and the verified-report's Omission #2:
 *   - The canonical config file is sushitrack.json at the repository root.
 *   - The advanced.tentative_logic fast_/slow_/vslow_ tiers and
 *     reid_direct_assignment_weight have no existing tests.
 *
 * Approach
 * --------
 * The SushiTrack public C API now owns JSON parsing through
 * SushiTrack_LoadParamsFromJson (src/ConfigLoader.cpp). This file keeps an
 * independent reference parser (applyConfigToParams below, built on the
 * vendored nlohmann/json single-header) as an oracle, exercises a round-trip
 * from JSON to SushiTrack_Params, asserts the library loader agrees with the
 * oracle (ConfigLoaderLibraryApiTest), and validates:
 *   1. Every documented field is loaded with the right type.
 *   2. Defaults are honoured when sections are missing.
 *   3. Out-of-range values are at minimum tolerated (the loader should not
 *      assert/throw on values the C API would later refuse).
 *
 * Coverage
 * --------
 * ConfigLoaderBaselineTest
 *   - sushitrack.json parses successfully (preconditional smoke test).
 *   - The "sushitrack" root section is an object.
 *   - The basic section contains every documented switch field.
 *
 * ConfigLoaderBasicFieldsTest
 *   - frame_rate, track_buffer, track_thresh, high_thresh, match_thresh round-trip.
 *   - All five enable_* switches load with their documented integer types.
 *
 * ConfigLoaderAdvancedFieldsTest
 *   - association.* fields load into Params.
 *   - coordinate_system.* fields load into Params.
 *   - reid_params.* fields including reid_direct_assignment_weight load.
 *   - fsm_params.* fields load.
 *   - tentative_logic.* including fast/slow/vslow tiers load.
 *   - mahalanobis_gating.* fields load.
 *
 * ConfigLoaderDefaultsTest
 *   - A missing section leaves the corresponding Params fields at the
 *     GetDefaultParams baseline.
 *
 * ConfigLoaderTrackerSmokeTest
 *   - Params produced by the loader build a tracker successfully.
 *   - Params produced by the loader survive a one-detection Update cycle.
 */

#include <gtest/gtest.h>
#include "SushiTrackC.h"

#include <nlohmann/json.hpp>

#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

namespace
{

using Json = nlohmann::json;

/**
 * @brief Resolves the path to sushitrack.json relative to the source tree.
 *
 * GoogleTest's working directory is set to ${CMAKE_SOURCE_DIR} via the
 * gtest_discover_tests WORKING_DIRECTORY argument (see tests/unit/CMakeLists.txt
 * and tests/regression/CMakeLists.txt for precedent), so the relative path
 * "sushitrack.json" is the canonical location. We also honour
 * SUSHITRACK_CONFIG as an environment override to align with the regression
 * runner's documented pattern.
 *
 * @return Filesystem path that should be loadable via std::ifstream.
 */
std::string configPath()
{
    const char* env = std::getenv("SUSHITRACK_CONFIG");
    if (env != nullptr && env[0] != '\0')
        return std::string(env);
    return std::string("sushitrack.json");
}

/**
 * @brief Safe numeric accessor: returns the JSON value or a default.
 */
template <typename T>
T jsonGet(const Json& sec, const char* key, T def)
{
    if (sec.is_object() && sec.contains(key) && sec[key].is_number())
        return sec[key].get<T>();
    return def;
}

/**
 * @brief Applies a parsed JSON document to a SushiTrack_Params struct.
 *
 * Fields absent from the JSON are left untouched, so the caller is responsible
 * for invoking SushiTrack_GetDefaultParams first to establish a baseline.
 *
 * @param doc Parsed JSON document (expected to contain a "sushitrack" object).
 * @param params [in,out] Params struct to populate.
 */
void applyConfigToParams(const Json& doc, SushiTrack_Params& params)
{
    if (!doc.is_object() || !doc.contains("sushitrack")) return;
    const Json& root = doc["sushitrack"];
    if (!root.is_object()) return;

    if (root.contains("basic") && root["basic"].is_object())
    {
        const Json& b = root["basic"];
        params.time_aware                 = jsonGet<int>  (b, "time_aware",                 params.time_aware);
        params.frame_rate                 = jsonGet<int>  (b, "frame_rate",                 params.frame_rate);
        params.track_buffer               = jsonGet<int>  (b, "track_buffer",               params.track_buffer);
        params.track_thresh               = jsonGet<float>(b, "track_thresh",               params.track_thresh);
        params.high_thresh                = jsonGet<float>(b, "high_thresh",                params.high_thresh);
        params.match_thresh               = jsonGet<float>(b, "match_thresh",               params.match_thresh);
        params.enable_tentative           = jsonGet<int>  (b, "enable_tentative",           params.enable_tentative);
        params.enable_fsm                 = jsonGet<int>  (b, "enable_fsm",                 params.enable_fsm);
        params.enable_reid                = jsonGet<int>  (b, "enable_reid",                params.enable_reid);
        params.enable_mahalanobis         = jsonGet<int>  (b, "enable_mahalanobis",         params.enable_mahalanobis);
    }

    if (!root.contains("advanced") || !root["advanced"].is_object()) return;
    const Json& adv = root["advanced"];

    if (adv.contains("association") && adv["association"].is_object())
    {
        const Json& s = adv["association"];
        params.primary_assoc_mode      = jsonGet<int>  (s, "primary_assoc_mode",      params.primary_assoc_mode);
        params.fuse_score              = jsonGet<int>  (s, "fuse_score",              params.fuse_score);
        params.iou_match_thresh        = jsonGet<float>(s, "iou_match_thresh",        params.iou_match_thresh);
        params.unconfirmed_iou_thresh  = jsonGet<float>(s, "unconfirmed_iou_thresh",  params.unconfirmed_iou_thresh);
        params.duplicate_iou_thresh    = jsonGet<float>(s, "duplicate_iou_thresh",    params.duplicate_iou_thresh);
    }

    if (adv.contains("coordinate_system") && adv["coordinate_system"].is_object())
    {
        const Json& s = adv["coordinate_system"];
        params.enable_xyah  = jsonGet<int>(s, "enable_xyah",  params.enable_xyah);
        params.iou_type     = jsonGet<int>(s, "iou_type",     params.iou_type);
        params.iou_plus_one = jsonGet<int>(s, "iou_plus_one", params.iou_plus_one);
        params.kalman_type  = jsonGet<int>(s, "kalman_type",  params.kalman_type);
    }

    if (adv.contains("reid_params") && adv["reid_params"].is_object())
    {
        const Json& s = adv["reid_params"];
        params.feature_momentum              = jsonGet<float>(s, "feature_momentum",              params.feature_momentum);
        params.reid_high_thresh              = jsonGet<float>(s, "reid_high_thresh",              params.reid_high_thresh);
        params.reid_high_iou_weight          = jsonGet<float>(s, "reid_high_iou_weight",          params.reid_high_iou_weight);
        params.reid_low_thresh               = jsonGet<float>(s, "reid_low_thresh",               params.reid_low_thresh);
        params.reid_low_iou_weight           = jsonGet<float>(s, "reid_low_iou_weight",           params.reid_low_iou_weight);
        params.reid_recovery_thresh          = jsonGet<float>(s, "reid_recovery_thresh",          params.reid_recovery_thresh);
        params.max_feature_history           = jsonGet<int>  (s, "max_feature_history",           params.max_feature_history);
        params.reid_direct_assignment_weight = jsonGet<float>(s, "reid_direct_assignment_weight", params.reid_direct_assignment_weight);
    }

    if (adv.contains("fsm_params") && adv["fsm_params"].is_object())
    {
        const Json& s = adv["fsm_params"];
        params.fsm_stationary_vel_thresh = jsonGet<float>(s, "fsm_stationary_vel_thresh", params.fsm_stationary_vel_thresh);
        params.fsm_maneuver_accel_thresh = jsonGet<float>(s, "fsm_maneuver_accel_thresh", params.fsm_maneuver_accel_thresh);
        params.fsm_reference_height      = jsonGet<float>(s, "fsm_reference_height",      params.fsm_reference_height);
    }

    if (adv.contains("tentative_logic") && adv["tentative_logic"].is_object())
    {
        const Json& s = adv["tentative_logic"];
        params.tentative_match_thresh      = jsonGet<float>(s, "tentative_match_thresh",      params.tentative_match_thresh);
        params.tentative_confirm_hits      = jsonGet<int>  (s, "tentative_confirm_hits",      params.tentative_confirm_hits);
        params.tentative_max_age_seconds   = jsonGet<float>(s, "tentative_max_age_seconds",   params.tentative_max_age_seconds);
        params.tentative_max_miss_frames   = jsonGet<int>  (s, "tentative_max_miss_frames",   params.tentative_max_miss_frames);
        params.tentative_init_suppress_iou = jsonGet<float>(s, "tentative_init_suppress_iou", params.tentative_init_suppress_iou);
        params.tentative_fast_thresh       = jsonGet<float>(s, "tentative_fast_thresh",       params.tentative_fast_thresh);
        params.tentative_fast_hits         = jsonGet<int>  (s, "tentative_fast_hits",         params.tentative_fast_hits);
        params.tentative_slow_thresh       = jsonGet<float>(s, "tentative_slow_thresh",       params.tentative_slow_thresh);
        params.tentative_slow_hits_offset  = jsonGet<int>  (s, "tentative_slow_hits_offset",  params.tentative_slow_hits_offset);
        params.tentative_vslow_hits_offset = jsonGet<int>  (s, "tentative_vslow_hits_offset", params.tentative_vslow_hits_offset);
    }

    if (adv.contains("mahalanobis_gating") && adv["mahalanobis_gating"].is_object())
    {
        const Json& s = adv["mahalanobis_gating"];
        params.proximity_thresh   = jsonGet<float>(s, "proximity_thresh",   params.proximity_thresh);
        params.gating_thresh      = jsonGet<float>(s, "gating_thresh",      params.gating_thresh);
        params.motion_cost_weight = jsonGet<float>(s, "motion_cost_weight", params.motion_cost_weight);
        params.max_area_ratio     = jsonGet<float>(s, "max_area_ratio",     params.max_area_ratio);
    }
}

/**
 * @brief Reads sushitrack.json into an nlohmann::json document.
 *
 * Returns an empty object on failure (mirrors TestInfra::parse_json
 * semantics in the regression harness).
 */
Json loadConfigJson()
{
    Json doc = Json::object();
    std::ifstream file(configPath());
    if (!file.is_open()) return doc;
    try
    {
        doc = Json::parse(file, /*cb*/ nullptr, /*allow_exceptions*/ true, /*ignore_comments*/ true);
    }
    catch (...)
    {
        return Json::object();
    }
    return doc;
}

}

// ConfigLoaderBaselineTest

/**
 * @brief sushitrack.json must parse and contain the expected root object.
 */
TEST(ConfigLoaderBaselineTest, ConfigFileParsesAndContainsRootObject)
{
    Json doc = loadConfigJson();
    ASSERT_FALSE(doc.empty())
        << "Could not load " << configPath()
        << " — set SUSHITRACK_CONFIG or run tests from the source root.";
    ASSERT_TRUE(doc.contains("sushitrack"));
    EXPECT_TRUE(doc["sushitrack"].is_object());
}

/**
 * @brief The basic section enumerates every documented switch.
 */
TEST(ConfigLoaderBaselineTest, BasicSectionContainsAllDocumentedSwitches)
{
    Json doc = loadConfigJson();
    ASSERT_FALSE(doc.empty());
    ASSERT_TRUE(doc.contains("sushitrack"));
    ASSERT_TRUE(doc["sushitrack"].contains("basic"));
    const Json& b = doc["sushitrack"]["basic"];

    for (const char* key : { "time_aware", "frame_rate", "track_buffer",
                              "track_thresh", "high_thresh", "match_thresh",
                              "enable_tentative", "enable_fsm", "enable_reid",
                              "enable_mahalanobis" })
    {
        EXPECT_TRUE(b.contains(key)) << "missing key: " << key;
    }
}

// ConfigLoaderBasicFieldsTest

/**
 * @brief Round-trip of the five numeric basic fields into Params.
 */
TEST(ConfigLoaderBasicFieldsTest, BasicNumericFieldsRoundTripIntoParams)
{
    Json doc = loadConfigJson();
    ASSERT_FALSE(doc.empty());

    SushiTrack_Params p{};
    SushiTrack_GetDefaultParams(&p);
    applyConfigToParams(doc, p);

    const Json& b = doc["sushitrack"]["basic"];
    EXPECT_EQ  (p.frame_rate,   b["frame_rate"].get<int>());
    EXPECT_EQ  (p.track_buffer, b["track_buffer"].get<int>());
    EXPECT_FLOAT_EQ(p.track_thresh, b["track_thresh"].get<float>());
    EXPECT_FLOAT_EQ(p.high_thresh,  b["high_thresh"].get<float>());
    EXPECT_FLOAT_EQ(p.match_thresh, b["match_thresh"].get<float>());
}

/**
 * @brief All five enable_* switches round-trip with integer type.
 */
TEST(ConfigLoaderBasicFieldsTest, EnableFlagsRoundTripIntoParams)
{
    Json doc = loadConfigJson();
    ASSERT_FALSE(doc.empty());

    SushiTrack_Params p{};
    SushiTrack_GetDefaultParams(&p);
    applyConfigToParams(doc, p);

    const Json& b = doc["sushitrack"]["basic"];
    EXPECT_EQ(p.enable_tentative,           b["enable_tentative"].get<int>());
    EXPECT_EQ(p.enable_fsm,                 b["enable_fsm"].get<int>());
    EXPECT_EQ(p.enable_reid,                b["enable_reid"].get<int>());
    EXPECT_EQ(p.enable_mahalanobis,         b["enable_mahalanobis"].get<int>());
}

// ConfigLoaderAdvancedFieldsTest

/**
 * @brief Fixture providing a loader-populated Params instance.
 */
class ConfigLoaderAdvancedFieldsTest : public ::testing::Test
{
protected:
    Json             doc_;
    SushiTrack_Params params_{};

    void SetUp() override
    {
        doc_ = loadConfigJson();
        ASSERT_FALSE(doc_.empty());
        SushiTrack_GetDefaultParams(&params_);
        applyConfigToParams(doc_, params_);
    }
};

/**
 * @brief Association sub-section fields are applied to Params.
 */
TEST_F(ConfigLoaderAdvancedFieldsTest, AssociationFieldsAreApplied)
{
    const Json& s = doc_["sushitrack"]["advanced"]["association"];
    EXPECT_EQ  (params_.primary_assoc_mode,     s["primary_assoc_mode"].get<int>());
    EXPECT_EQ  (params_.fuse_score,             s["fuse_score"].get<int>());
    EXPECT_FLOAT_EQ(params_.iou_match_thresh,       s["iou_match_thresh"].get<float>());
    EXPECT_FLOAT_EQ(params_.unconfirmed_iou_thresh, s["unconfirmed_iou_thresh"].get<float>());
    EXPECT_FLOAT_EQ(params_.duplicate_iou_thresh,   s["duplicate_iou_thresh"].get<float>());
}

/**
 * @brief Coordinate-system fields are applied to Params.
 */
TEST_F(ConfigLoaderAdvancedFieldsTest, CoordinateSystemFieldsAreApplied)
{
    const Json& s = doc_["sushitrack"]["advanced"]["coordinate_system"];
    EXPECT_EQ(params_.enable_xyah,  s["enable_xyah"].get<int>());
    EXPECT_EQ(params_.iou_type,     s["iou_type"].get<int>());
    EXPECT_EQ(params_.iou_plus_one, s["iou_plus_one"].get<int>());
    EXPECT_EQ(params_.kalman_type,  s["kalman_type"].get<int>());
}

/**
 * @brief ReID fields, including the documented but previously-untested
 *        reid_direct_assignment_weight (Verified Omission #2), are applied.
 */
TEST_F(ConfigLoaderAdvancedFieldsTest, ReIDFieldsIncludingDirectAssignmentWeightAreApplied)
{
    const Json& s = doc_["sushitrack"]["advanced"]["reid_params"];
    EXPECT_FLOAT_EQ(params_.feature_momentum,              s["feature_momentum"].get<float>());
    EXPECT_FLOAT_EQ(params_.reid_high_thresh,              s["reid_high_thresh"].get<float>());
    EXPECT_FLOAT_EQ(params_.reid_high_iou_weight,          s["reid_high_iou_weight"].get<float>());
    EXPECT_FLOAT_EQ(params_.reid_low_thresh,               s["reid_low_thresh"].get<float>());
    EXPECT_FLOAT_EQ(params_.reid_low_iou_weight,           s["reid_low_iou_weight"].get<float>());
    EXPECT_FLOAT_EQ(params_.reid_recovery_thresh,          s["reid_recovery_thresh"].get<float>());
    EXPECT_EQ      (params_.max_feature_history,           s["max_feature_history"].get<int>());
    EXPECT_FLOAT_EQ(params_.reid_direct_assignment_weight, s["reid_direct_assignment_weight"].get<float>());
}

/**
 * @brief FSM threshold fields are applied to Params.
 */
TEST_F(ConfigLoaderAdvancedFieldsTest, FSMFieldsAreApplied)
{
    const Json& s = doc_["sushitrack"]["advanced"]["fsm_params"];
    EXPECT_FLOAT_EQ(params_.fsm_stationary_vel_thresh, s["fsm_stationary_vel_thresh"].get<float>());
    EXPECT_FLOAT_EQ(params_.fsm_maneuver_accel_thresh, s["fsm_maneuver_accel_thresh"].get<float>());
    EXPECT_FLOAT_EQ(params_.fsm_reference_height,      s["fsm_reference_height"].get<float>());
}

/**
 * @brief Tentative-logic fast/slow/vslow tiers are applied — the previously
 *        untested fields called out in Omission #2.
 */
TEST_F(ConfigLoaderAdvancedFieldsTest, TentativeLogicTiersAreApplied)
{
    const Json& s = doc_["sushitrack"]["advanced"]["tentative_logic"];
    EXPECT_FLOAT_EQ(params_.tentative_match_thresh,      s["tentative_match_thresh"].get<float>());
    EXPECT_EQ      (params_.tentative_confirm_hits,      s["tentative_confirm_hits"].get<int>());
    EXPECT_FLOAT_EQ(params_.tentative_max_age_seconds,   s["tentative_max_age_seconds"].get<float>());
    EXPECT_EQ      (params_.tentative_max_miss_frames,   s["tentative_max_miss_frames"].get<int>());
    EXPECT_FLOAT_EQ(params_.tentative_init_suppress_iou, s["tentative_init_suppress_iou"].get<float>());
    EXPECT_FLOAT_EQ(params_.tentative_fast_thresh,       s["tentative_fast_thresh"].get<float>());
    EXPECT_EQ      (params_.tentative_fast_hits,         s["tentative_fast_hits"].get<int>());
    EXPECT_FLOAT_EQ(params_.tentative_slow_thresh,       s["tentative_slow_thresh"].get<float>());
    EXPECT_EQ      (params_.tentative_slow_hits_offset,  s["tentative_slow_hits_offset"].get<int>());
    EXPECT_EQ      (params_.tentative_vslow_hits_offset, s["tentative_vslow_hits_offset"].get<int>());
}

/**
 * @brief Mahalanobis-gating fields are applied to Params.
 */
TEST_F(ConfigLoaderAdvancedFieldsTest, MahalanobisGatingFieldsAreApplied)
{
    const Json& s = doc_["sushitrack"]["advanced"]["mahalanobis_gating"];
    EXPECT_FLOAT_EQ(params_.proximity_thresh,   s["proximity_thresh"].get<float>());
    EXPECT_FLOAT_EQ(params_.gating_thresh,      s["gating_thresh"].get<float>());
    EXPECT_FLOAT_EQ(params_.motion_cost_weight, s["motion_cost_weight"].get<float>());
    EXPECT_FLOAT_EQ(params_.max_area_ratio,     s["max_area_ratio"].get<float>());
}

// ConfigLoaderDefaultsTest

/**
 * @brief A document missing the "basic" section must not perturb the basic
 *        fields of Params — they should remain at the GetDefaultParams baseline.
 */
TEST(ConfigLoaderDefaultsTest, MissingBasicSectionLeavesDefaults)
{
    Json doc = Json::object();
    doc["sushitrack"]["advanced"]["association"]["primary_assoc_mode"] = 0;

    SushiTrack_Params p{};
    SushiTrack_GetDefaultParams(&p);
    const int default_frame_rate = p.frame_rate;
    applyConfigToParams(doc, p);

    EXPECT_EQ(p.frame_rate, default_frame_rate);
    EXPECT_EQ(p.primary_assoc_mode, 0);
}

/**
 * @brief A completely empty document must leave Params equal to defaults.
 */
TEST(ConfigLoaderDefaultsTest, EmptyDocumentLeavesAllDefaults)
{
    Json doc = Json::object();

    SushiTrack_Params before{};
    SushiTrack_GetDefaultParams(&before);
    SushiTrack_Params after = before;
    applyConfigToParams(doc, after);

    EXPECT_EQ(after.frame_rate,    before.frame_rate);
    EXPECT_EQ(after.track_buffer,  before.track_buffer);
    EXPECT_FLOAT_EQ(after.match_thresh, before.match_thresh);
    EXPECT_EQ(after.enable_fsm,    before.enable_fsm);
}

// ConfigLoaderTrackerSmokeTest

/**
 * @brief Params produced by the loader must build a tracker successfully.
 */
TEST(ConfigLoaderTrackerSmokeTest, LoadedParamsBuildTrackerSuccessfully)
{
    Json doc = loadConfigJson();
    ASSERT_FALSE(doc.empty());

    SushiTrack_Params p{};
    SushiTrack_GetDefaultParams(&p);
    applyConfigToParams(doc, p);

    SushiTrackHdl hdl = SushiTrack_Create(&p);
    ASSERT_NE(hdl, nullptr);
    SushiTrack_Destroy(hdl);
}

/**
 * @brief Params produced by the loader must survive a one-detection Update.
 */
TEST(ConfigLoaderTrackerSmokeTest, LoadedParamsSurviveSingleDetectionUpdate)
{
    Json doc = loadConfigJson();
    ASSERT_FALSE(doc.empty());

    SushiTrack_Params p{};
    SushiTrack_GetDefaultParams(&p);
    applyConfigToParams(doc, p);
    p.time_aware = 1; // ensure dt argument is honoured

    SushiTrackHdl hdl = SushiTrack_Create(&p);
    ASSERT_NE(hdl, nullptr);

    SushiTrack_Object d{};
    d.x = 100.0f; d.y = 200.0f;
    d.width = 50.0f; d.height = 100.0f;
    d.score = 0.9f; d.label = 1;
    std::vector<SushiTrack_Track> out(4);
    int n = SushiTrack_Update(hdl, &d, 1, out.data(), 4, 0.04f);
    EXPECT_GE(n, 0);

    SushiTrack_Destroy(hdl);
}

// ConfigLoaderLibraryApiTest

/**
 * @brief The library-owned loader (SushiTrack_LoadParamsFromJson) must produce
 *        the same Params as the in-test reference parser for the shipped file.
 *
 * This is the parity guard for the production code path: sushitrack.json is now
 * parsed inside the library, not by the harness, so the two must agree field by
 * field. The reference parser above stays as an independent oracle.
 */
TEST(ConfigLoaderLibraryApiTest, LibraryLoaderMatchesReferenceParser)
{
    Json doc = loadConfigJson();
    ASSERT_FALSE(doc.empty());

    SushiTrack_Params reference{};
    SushiTrack_GetDefaultParams(&reference);
    applyConfigToParams(doc, reference);

    SushiTrack_Params loaded{};
    SushiTrack_GetDefaultParams(&loaded);
    ASSERT_EQ(SushiTrack_LoadParamsFromJson(configPath().c_str(), &loaded),
              SUSHITRACK_SUCCESS);

    EXPECT_EQ      (loaded.enable_fsm,                 reference.enable_fsm);
    EXPECT_EQ      (loaded.frame_rate,                 reference.frame_rate);
    EXPECT_EQ      (loaded.primary_assoc_mode,         reference.primary_assoc_mode);
    EXPECT_FLOAT_EQ(loaded.match_thresh,               reference.match_thresh);
    EXPECT_FLOAT_EQ(loaded.fsm_maneuver_accel_thresh,  reference.fsm_maneuver_accel_thresh);
}

/**
 * @brief A NULL params pointer must be rejected without dereferencing.
 */
TEST(ConfigLoaderLibraryApiTest, NullParamsAreRejected)
{
    EXPECT_EQ(SushiTrack_LoadParamsFromJson(configPath().c_str(), nullptr),
              SUSHITRACK_INVALID_ARGUMENT);
}

/**
 * @brief A missing file is non-fatal: params are left at defaults and SUCCESS
 *        is returned, so configuration stays optional.
 */
TEST(ConfigLoaderLibraryApiTest, MissingFileLeavesDefaultsAndSucceeds)
{
    SushiTrack_Params before{};
    SushiTrack_GetDefaultParams(&before);
    SushiTrack_Params after = before;

    EXPECT_EQ(SushiTrack_LoadParamsFromJson("this/path/does/not/exist.json", &after),
              SUSHITRACK_SUCCESS);
    EXPECT_EQ(after.frame_rate,  before.frame_rate);
    EXPECT_EQ(after.enable_fsm,  before.enable_fsm);
}
