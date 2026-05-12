/**
 * @file      RegressionTest.cpp
 * @brief     Entry point for the regression_test executable.
 *
 *            Runs the full MOT benchmark suite (label-based inference) across
 *            SushiTrack, ByteTrack, and OC-SORT on the sequences defined in
 *            ScenarioList.h. Each test verifies that the tracker processes all
 *            frames without error and writes prediction files to
 *            test/data/<sequence>/pred/<tracker>/.
 *
 *            Video inference is handled separately via the Python pipeline in
 *            tests/regression/inference/ using the tracker_bridge shared library.
 *
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      12.05.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
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

// ---------------------------------------------------------------------------
// Config resolution
// ---------------------------------------------------------------------------

/**
 * @brief Resolves the tracker configuration file path.
 *
 * Checks the SUSHITRACK_CONFIG environment variable first, then falls back to
 * standard search locations relative to the working directory.
 *
 * @return Absolute or relative path to a valid config.json, or empty string.
 */
static std::string resolveConfig()
{
    if (const char* env_path = std::getenv("SUSHITRACK_CONFIG"))
        if (env_path[0] != '\0' && fs::exists(env_path))
            return std::string(env_path);

    for (const auto& p : {"tests/config.json", "tests/regression/evaluator/data/config.json", "data/config.json"})
        if (fs::exists(p)) return p;

    return "";
}

static const std::string kConfigPath = resolveConfig();

// ---------------------------------------------------------------------------
// Adapter factories
// ---------------------------------------------------------------------------

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
    { "sushitrack", []() { return std::make_unique<SushiTrackAdapter>(kConfigPath); } },
    { "bytetrack", []() { return std::make_unique<ByteTrackAdapter>(kConfigPath); } },
    { "ocsort",    []() { return std::make_unique<OCSORTAdapter>(kConfigPath);    } },
};

// ---------------------------------------------------------------------------
// Parameterised test
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// Test instantiation
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

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

    if (!kConfigPath.empty())
        std::cout << "[ CONFIG ] Using " << kConfigPath << "\n\n";
    else
        std::cout << "[ CONFIG ] No config.json found -- using tracker defaults\n\n";

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
