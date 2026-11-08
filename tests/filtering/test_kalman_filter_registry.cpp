/**************************************************************************/
/* test_kalman_filter_registry.cpp                                        */
/**************************************************************************/
/*                          This file is part of:                         */
/*                               SushiTrack                               */
/*               https://github.com/SushiSystems/SushiTrack               */
/*                         https://sushisystems.io                        */
/**************************************************************************/
/* Copyright (c) 2026-present Mustafa Garip & Sushi Systems               */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License"); you    */
/* may not use this file except in compliance with the License. You may   */
/* obtain a copy of the License at                                        */
/*                                                                        */
/*     https://www.apache.org/licenses/LICENSE-2.0                        */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                         */
/**************************************************************************/

/**
 * @file test_kalman_filter_registry.cpp
 * @brief Tests KalmanFilterRegistry.
 * @author Mustafa Garip
 */

#include <cmath>
#include <string>

#include <Eigen/Dense>
#include <gtest/gtest.h>

#include <SushiTrack/filtering/gated_state_predictor.hpp>
#include <SushiTrack/filtering/kalman_filter_registry.hpp>
#include <SushiTrack/filtering/kalman_types.hpp>

#include <captured_log.hpp>

using namespace SushiTrack;

using StateMean = KalmanTypes::StateMean;

using StateCov = KalmanTypes::StateCov;

using DetectBox = KalmanTypes::DetectBox;

// KalmanFilterRegistry

/**
 * @brief An unregistered type yields no filter and one ERROR line naming the type.
 */
TEST(Small_KalmanFilterRegistryTest, UnregisteredTypeLogsTheTypeAndReturnsNull)
{
    std::unique_ptr<GatedStatePredictor> filter;
    std::string logged;
    {
        CapturedLog log(SushiLog::LogLevel::Error);
        const KalmanFilterRegistry registry;
        filter = registry.create(987654, false);
        logged = log.last("Unregistered Kalman type requested");
    }

    EXPECT_EQ(filter, nullptr);
    EXPECT_NE(logged.find("Unregistered Kalman type requested: 987654"), std::string::npos)
        << "logged: " << logged;
}

/**
 * @brief Registering one type twice logs one WARN naming it, and the later creator wins.
 */
TEST(Small_KalmanFilterRegistryTest, RegisteringATypeTwiceWarnsAndTheLaterCreatorWins)
{
    CapturedLog log;
    KalmanFilterRegistry registry;
    const auto none = [](bool)
    {
        return std::unique_ptr<GatedStatePredictor>();
    };
    registry.register_creator(987001, none);
    EXPECT_EQ(log.count("registered twice"), 0);

    registry.register_creator(987001, none);

    EXPECT_EQ(log.count("Kalman type 987001 registered twice"), 1);
    EXPECT_EQ(log.level_of("registered twice"), SushiLog::LogLevel::Warn);
}
