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
#include <typeinfo>

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

namespace
{
    /**
     * @brief One registered type and the class the factory must build for it.
     */
    struct TypeRow
    {
            int type;
            const std::type_info* built;
    };

    /** @brief Runs each kalman_type of the library registry: 0 linear, 1 NSA, 2 IMM. */
    class Small_GenericKalmanFactoryTest : public ::testing::TestWithParam<TypeRow>
    {
    };
} // namespace

/**
 * @brief The factory builds the class its registry holds for its type: 0 a KalmanFilter, 1 an
 *        NSAKalmanFilter, 2 an IMMFilter.
 */
TEST_P(Small_GenericKalmanFactoryTest, BuildsWhatItsRegistryHoldsForTheType)
{
    const std::unique_ptr<KalmanFilterRegistry> registry = TestSupport::library_kalman_registry();

    const std::unique_ptr<StatePredictor> built =
        GenericKalmanFactory(GetParam().type, false, *registry).create();

    ASSERT_NE(built, nullptr);
    EXPECT_EQ(typeid(*built), *GetParam().built);
}

INSTANTIATE_TEST_SUITE_P(EveryRegisteredType, Small_GenericKalmanFactoryTest,
                         ::testing::Values(TypeRow{0, &typeid(KalmanFilter)},
                                           TypeRow{1, &typeid(NSAKalmanFilter)},
                                           TypeRow{2, &typeid(IMMFilter)}));

/**
 * @brief A type its registry does not hold builds nothing, and the registry logs one ERROR.
 */
TEST(Small_GenericKalmanFactoryUnregisteredTest, UnregisteredTypeBuildsNothing)
{
    const CapturedLog log(SushiLog::LogLevel::Error);
    const KalmanFilterRegistry registry;

    EXPECT_EQ(GenericKalmanFactory(987654, false, registry).create(), nullptr);
    EXPECT_EQ(log.count("Unregistered Kalman type requested: 987654"), 1);
}
