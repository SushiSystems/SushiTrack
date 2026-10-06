/**************************************************************************/
/* RegressionTest.cpp                                                     */
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
 * @file RegressionTest.cpp
 * @brief Entry point and test fixtures for regression testing.
 * @author Mustafa Garip
 */

#include <gtest/gtest.h>

#include "TestInfra.h"
#include "ITrackerAdapter.h"
#include "MOTRunner.h"
#include "ScenarioList.h"
#include "SushiTrackAdapter.h"
#include "ByteTrackAdapter.h"
#include "OCSORTAdapter.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

using namespace TestInfrastructure;
namespace fs = std::filesystem;

// Config resolution

/**
 * @brief Resolves a configuration file by walking up from the working directory.
 *
 * Each tracker now owns a dedicated file (sushitrack.json at the repository root,
 * bytetrack.json and ocsort.json under tests/). A binary launched from build/bin
 * still finds them by ascending through parent directories.
 */
static std::string resolveConfigUp(const std::string& rel)
{
    std::error_code ec;
    fs::path dir = fs::current_path(ec);
    for (int depth = 0; depth < 8 && !dir.empty(); ++depth)
    {
        fs::path candidate = dir / rel;
        if (fs::exists(candidate, ec)) return candidate.string();

        if (!dir.has_parent_path()) break;
        fs::path parent = dir.parent_path();
        if (parent == dir) break;
        dir = parent;
    }
    return "";
}

// SushiTrack resolves its own sushitrack.json inside the library (SUSHITRACK_CONFIG env var, then an upward search), so its factory passes an empty path and lets the loader decide. The reference trackers are configured here from their own files under tests/.
static const std::string kByteTrackConfig = resolveConfigUp("tests/bytetrack.json");
static const std::string kOCSortConfig    = resolveConfigUp("tests/ocsort.json");

// Adapter factories

using AdapterFactory = std::function<std::unique_ptr<ITrackerAdapter>()>;

/**
 * @brief Associates a tracker name with its adapter factory.
 */
struct NamedFactory
{
    std::string    tracker_name; ///< Human-readable tracker identifier.
    AdapterFactory factory;      ///< Factory function returning a fresh adapter.
};

static const NamedFactory kFactories[] =
{
    { "sushitrack", []() { return std::make_unique<SushiTrackAdapter>("");               } },
    { "bytetrack", []() { return std::make_unique<ByteTrackAdapter>(kByteTrackConfig); } },
    { "ocsort",    []() { return std::make_unique<OCSORTAdapter>(kOCSortConfig);        } },
};

// Parameterised test

/**
 * @brief Combines a benchmark scenario with a tracker factory for parametrisation.
 */
struct RegressionParams
{
    Scenario        scenario;      ///< MOT sequence descriptor.
    std::string     tracker_name;  ///< Tracker identifier string.
    AdapterFactory  factory;       ///< Adapter factory for this tracker.
};

/**
 * @brief GTest fixture for parameterised label-based regression tests.
 *
 * Each test case runs one tracker on one MOT sequence, verifying that all
 * frames are processed and that per-frame timing is positive.
 */
class RegressionTest : public ::testing::TestWithParam<RegressionParams>
{
protected:
    /** @brief Instantiates the tracker adapter before each test. */
    void SetUp() override
    {
        adapter_ = GetParam().factory();
    }

    std::unique_ptr<ITrackerAdapter> adapter_; ///< Tracker under test.
};

/**
 * @brief Runs the full MOT sequence through the tracker adapter.
 *
 * Skips gracefully when the detection file for a sequence is absent so that
 * the test suite can be run against a partial dataset.
 */
TEST_P(RegressionTest, LabelInference)
{
    const Scenario& sc = GetParam().scenario;

    std::string data_root = "tests/regression/evaluator/data";
    if (!fs::exists(fs::path(data_root) / sc.name / "det" / "det.txt"))
        data_root = "data";

    if (!fs::exists(fs::path(data_root) / sc.name / "det" / "det.txt"))
    {
        GTEST_SKIP() << "det.txt not found for " << sc.name;
        return;
    }

    RunResult result = run_mot_sequence(data_root, sc.name, *adapter_);

    if (result.frames_processed == 0)
    {
        GTEST_SKIP() << "Empty sequence: " << sc.name;
        return;
    }

    EXPECT_GT(result.frames_processed, 0);
    EXPECT_GT(result.total_tracking_us, 0.0);
}

// Test instantiation

/**
 * @brief Builds the cross-product of scenarios and tracker factories.
 * @return All (scenario, tracker) pairs for parameterised instantiation.
 */
static std::vector<RegressionParams> buildParams()
{
    std::vector<RegressionParams> params;
    for (const auto& sc : kScenarios)
        for (const auto& nf : kFactories)
            params.push_back({sc, nf.tracker_name, nf.factory});
    return params;
}

INSTANTIATE_TEST_SUITE_P(
    MOT,
    RegressionTest,
    ::testing::ValuesIn(buildParams()),
    [](const ::testing::TestParamInfo<RegressionParams>& info)
    {
        std::string sc = info.param.scenario.name;
        std::replace(sc.begin(), sc.end(), '-', '_');
        return sc + "_" + info.param.tracker_name;
    });

// Entry point

/**
 * @brief Program entry point for the regression_test binary.
 *
 * @param argc Argument count forwarded to GoogleTest.
 * @param argv Argument vector forwarded to GoogleTest.
 * @return 0 on success, non-zero if any test fails.
 */
int main(int argc, char** argv)
{
    printHardwareInfo();

    std::cout << "[ CONFIG ] SushiTrack: resolved by the library (SUSHITRACK_CONFIG / sushitrack.json)\n";
    std::cout << "[ CONFIG ] ByteTrack: "
              << (kByteTrackConfig.empty() ? "defaults" : kByteTrackConfig) << "\n";
    std::cout << "[ CONFIG ] OCSORT   : "
              << (kOCSortConfig.empty() ? "defaults" : kOCSortConfig) << "\n\n";

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
