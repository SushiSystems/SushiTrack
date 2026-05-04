/**
 * @file      MainTest.cpp
 * @brief     Main entry point for GoogleTest-based MOT benchmarks; orchestrates the execution of tracking algorithms across parameterized scenarios and datasets.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      30.04.2026
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

static std::string resolveConfig()
{
    // Allow the user to point at any config file (e.g. data/config.bytetrack_identical.json)
    // without recompiling. Falls back to the default search path if the variable is unset.
    if (const char* env_path = std::getenv("SUSHITRACK_CONFIG"))
    {
        if (env_path[0] != '\0' && fs::exists(env_path))
            return std::string(env_path);
    }

    for (const auto& p : {"data/config.json", "test/data/config.json"})
        if (fs::exists(p)) return p;
    return "";
}

static const std::string kConfigPath = resolveConfig();

using AdapterFactory = std::function<std::unique_ptr<ITrackerAdapter>()>;

struct NamedFactory
{
    std::string    tracker_name;
    AdapterFactory factory;
};

static const NamedFactory kFactories[] =
{
    { "sushitrack", []() { return std::make_unique<SushiTrackAdapter>(kConfigPath); } },
    { "bytetrack", []() { return std::make_unique<ByteTrackAdapter>(kConfigPath); } },
    { "ocsort",    []() { return std::make_unique<OCSORTAdapter>(kConfigPath);    } },
};

struct MOTParams
{
    Scenario        scenario;
    std::string     tracker_name;
    AdapterFactory  factory;
};

class TrackerTest : public ::testing::TestWithParam<MOTParams>
{
protected:
    void SetUp() override
    {
        adapter_ = GetParam().factory();
    }

    std::unique_ptr<ITrackerAdapter> adapter_;
};

TEST_P(TrackerTest, MOT)
{
    const Scenario& sc = GetParam().scenario;

    std::string data_root = "data";
    if (!fs::exists(fs::path(data_root) / sc.name / "det" / "det.txt"))
        data_root = "test/data";

    if (!fs::exists(fs::path(data_root) / sc.name / "det" / "det.txt"))
    {
        GTEST_SKIP() << "det.txt not found for " << sc.name;
        return;
    }

    RunResult result = run_mot_sequence(data_root, sc.name, *adapter_);

    if (result.frames_processed == 0)
    {
        GTEST_SKIP() << "Empty sequence for " << sc.name;
        return;
    }

    EXPECT_GT(result.frames_processed, 0);
    EXPECT_GT(result.total_tracking_us, 0.0);
}

static std::vector<MOTParams> buildParams()
{
    std::vector<MOTParams> params;
    for (const auto& sc : kScenarios)
        for (const auto& nf : kFactories)
            params.push_back({sc, nf.tracker_name, nf.factory});
    return params;
}

INSTANTIATE_TEST_SUITE_P(
    MOT,
    TrackerTest,
    ::testing::ValuesIn(buildParams()),
    [](const ::testing::TestParamInfo<MOTParams>& info) {
        std::string sc = info.param.scenario.name;
        std::replace(sc.begin(), sc.end(), '-', '_');
        return sc + "_" + info.param.tracker_name;
    });

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
