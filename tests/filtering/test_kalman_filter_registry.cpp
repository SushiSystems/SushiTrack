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

#include <memory>
#include <string>

#include <gtest/gtest.h>

#include <SushiTrack/filtering/gated_state_predictor.hpp>
#include <SushiTrack/filtering/kalman_filter.hpp>
#include <SushiTrack/filtering/kalman_filter_registry.hpp>

#include <captured_log.hpp>

using namespace SushiTrack;

namespace
{
    /** @brief The type no test registers anywhere. */
    constexpr int K_UNREGISTERED_TYPE = 987654;

    /** @brief The type the registration tests register twice. */
    constexpr int K_TWICE_REGISTERED_TYPE = 987001;

    /** @brief A creator that builds nothing. */
    std::unique_ptr<GatedStatePredictor> create_nothing(bool)
    {
        return nullptr;
    }

    /** @brief A creator that builds a linear KalmanFilter. */
    std::unique_ptr<GatedStatePredictor> create_linear(bool enable_xyah)
    {
        return std::make_unique<KalmanFilter>(enable_xyah);
    }
} // namespace

/**
 * @brief An unregistered type yields no filter.
 */
TEST(Small_KalmanFilterRegistryTest, UnregisteredTypeReturnsNull)
{
    const KalmanFilterRegistry registry;

    EXPECT_EQ(registry.create(K_UNREGISTERED_TYPE, false), nullptr);
}

/**
 * @brief An unregistered type logs one ERROR line naming the type.
 */
TEST(Small_KalmanFilterRegistryTest, UnregisteredTypeLogsOneErrorNamingTheType)
{
    const CapturedLog log(SushiLog::LogLevel::Error);
    const KalmanFilterRegistry registry;

    const std::unique_ptr<GatedStatePredictor> filter = registry.create(K_UNREGISTERED_TYPE, false);

    EXPECT_EQ(log.count("Unregistered Kalman type requested: 987654"), 1);
    EXPECT_EQ(log.level_of("Unregistered Kalman type requested"), SushiLog::LogLevel::Error);
}

/**
 * @brief Registering a type once logs no warning.
 */
TEST(Small_KalmanFilterRegistryTest, RegisteringATypeOnceDoesNotWarn)
{
    const CapturedLog log;
    KalmanFilterRegistry registry;

    registry.register_creator(K_TWICE_REGISTERED_TYPE, create_nothing);

    EXPECT_EQ(log.count("registered twice"), 0);
}

/**
 * @brief Registering one type twice logs one WARN naming it.
 */
TEST(Small_KalmanFilterRegistryTest, RegisteringATypeTwiceWarnsOnce)
{
    const CapturedLog log;
    KalmanFilterRegistry registry;
    registry.register_creator(K_TWICE_REGISTERED_TYPE, create_nothing);

    registry.register_creator(K_TWICE_REGISTERED_TYPE, create_nothing);

    EXPECT_EQ(log.count("Kalman type 987001 registered twice"), 1);
    EXPECT_EQ(log.level_of("registered twice"), SushiLog::LogLevel::Warn);
}

/**
 * @brief After a type is registered twice, create() calls the later creator.
 *
 * The first creator builds nothing and the second builds a KalmanFilter, so a non-null result
 * of that class can only come from the second.
 */
TEST(Small_KalmanFilterRegistryTest, TheLaterCreatorWinsWhenATypeIsRegisteredTwice)
{
    KalmanFilterRegistry registry;
    registry.register_creator(K_TWICE_REGISTERED_TYPE, create_nothing);
    registry.register_creator(K_TWICE_REGISTERED_TYPE, create_linear);

    const std::unique_ptr<GatedStatePredictor> filter =
        registry.create(K_TWICE_REGISTERED_TYPE, false);

    ASSERT_NE(filter, nullptr);
    EXPECT_NE(dynamic_cast<KalmanFilter*>(filter.get()), nullptr);
}
