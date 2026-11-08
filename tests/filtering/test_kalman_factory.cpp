/**************************************************************************/
/* test_kalman_factory.cpp                                                */
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
 * @file test_kalman_factory.cpp
 * @brief Tests the KalmanFactory contract: every create gives a new, independent predictor.
 * @author Mustafa Garip
 */

#include <memory>

#include <gtest/gtest.h>

#include <SushiTrack/filtering/generic_kalman_factory.hpp>
#include <SushiTrack/filtering/kalman_factory.hpp>
#include <SushiTrack/filtering/kalman_filter_registry.hpp>
#include <SushiTrack/filtering/state_predictor.hpp>

#include <support/kalman_registry_builders.hpp>

using namespace SushiTrack;

/**
 * @brief Through KalmanFactory&, two calls to create give two distinct predictors.
 */
TEST(Small_KalmanFactoryTest, EveryCreateGivesANewPredictor)
{
    const std::unique_ptr<KalmanFilterRegistry> registry = TestSupport::library_kalman_registry();
    const GenericKalmanFactory generic(0, false, *registry);
    const KalmanFactory& factory = generic;

    const std::unique_ptr<StatePredictor> first = factory.create();
    const std::unique_ptr<StatePredictor> second = factory.create();

    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    EXPECT_NE(first.get(), second.get());
}
