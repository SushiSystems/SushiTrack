/**************************************************************************/
/* test_generic_kalman_factory.cpp                                        */
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
 * @file test_generic_kalman_factory.cpp
 * @brief Tests GenericKalmanFactory, which builds state predictors from the registry.
 * @author Mustafa Garip
 */

#include <memory>

#include <gtest/gtest.h>

#include <SushiTrack/filtering/generic_kalman_factory.hpp>
#include <SushiTrack/filtering/imm_filter.hpp>
#include <SushiTrack/filtering/kalman_filter.hpp>
#include <SushiTrack/filtering/kalman_filter_registry.hpp>
#include <SushiTrack/filtering/nsa_kalman_filter.hpp>
#include <SushiTrack/filtering/state_predictor.hpp>

#include <captured_log.hpp>
#include <support/kalman_registry_builders.hpp>

using namespace SushiTrack;

/**
 * @brief The factory builds what its registry holds for its type.
 */
TEST(Small_GenericKalmanFactoryTest, BuildsWhatItsRegistryHoldsForTheType)
{
    const std::unique_ptr<KalmanFilterRegistry> registry = TestSupport::library_kalman_registry();

    const std::unique_ptr<StatePredictor> nsa = GenericKalmanFactory(1, false, *registry).create();

    EXPECT_NE(dynamic_cast<NSAKalmanFilter*>(nsa.get()), nullptr);
}

/**
 * @brief Through the library's registry, types 0, 1 and 2 build a KalmanFilter, an
 *        NSAKalmanFilter and an IMMFilter.
 */
TEST(Medium_GenericKalmanFactoryTest, BuildsTheRegisteredTypes)
{
    const std::unique_ptr<StatePredictor> linear = GenericKalmanFactory(0).create();
    const std::unique_ptr<StatePredictor> nsa = GenericKalmanFactory(1).create();
    const std::unique_ptr<StatePredictor> imm = GenericKalmanFactory(2).create();

    EXPECT_NE(dynamic_cast<KalmanFilter*>(linear.get()), nullptr);
    EXPECT_NE(dynamic_cast<NSAKalmanFilter*>(nsa.get()), nullptr);
    EXPECT_NE(dynamic_cast<IMMFilter*>(imm.get()), nullptr);
}

/**
 * @brief A type its registry does not hold builds nothing, and the registry logs one ERROR.
 */
TEST(Small_GenericKalmanFactoryTest, UnregisteredTypeBuildsNothing)
{
    const CapturedLog log(SushiLog::LogLevel::Error);
    const KalmanFilterRegistry registry;

    EXPECT_EQ(GenericKalmanFactory(987654, false, registry).create(), nullptr);
    EXPECT_EQ(log.count("Unregistered Kalman type requested: 987654"), 1);
}
