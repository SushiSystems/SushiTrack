/**************************************************************************/
/* test_create_tracker.cpp                                                */
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
 * @file test_create_tracker.cpp
 * @brief Tests SushiTrack::create_tracker, which builds a tracker from a registry the caller owns.
 * @author Mustafa Garip
 */

#include <memory>
#include <new>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/c_api/create_tracker.hpp>
#include <SushiTrack/c_api/sushitrack_c.h>
#include <SushiTrack/filtering/gated_state_predictor.hpp>
#include <SushiTrack/filtering/kalman_filter.hpp>
#include <SushiTrack/filtering/kalman_filter_registry.hpp>

namespace
{
    /// @brief Kalman type whose creator throws std::runtime_error.
    constexpr int K_THROWING_KALMAN_TYPE = 9101;
    /// @brief Kalman type whose creator throws std::bad_alloc.
    constexpr int K_OUT_OF_MEMORY_KALMAN_TYPE = 9102;
    /// @brief Kalman type whose creator builds type 0 until told to throw.
    constexpr int K_FAILING_LATER_KALMAN_TYPE = 9103;

    /// @brief Returns default parameters naming Kalman type @p type.
    sushitrack_params_t params_with_kalman_type(int type)
    {
        sushitrack_params_t params{};
        sushitrack_get_default_params(&params);
        params.kalman_type = type;
        return params;
    }

    /// @brief Returns a detection of a 50 by 100 box at (@p x, @p y) with score 0.9.
    sushitrack_object_t make_det(float x, float y)
    {
        sushitrack_object_t object{};
        object.x = x;
        object.y = y;
        object.width = 50.0f;
        object.height = 100.0f;
        object.score = 0.9f;
        object.label = 1;
        return object;
    }
} // namespace

/**
 * @brief A component that throws while the tracker is built returns
 *        SUSHITRACK_INITIALIZATION_FAILURE and no handle.
 */
TEST(Small_CreateTrackerTest, InitFailureReturnsInitializationFailureCode)
{
    SushiTrack::KalmanFilterRegistry registry;
    registry.register_creator(
        K_THROWING_KALMAN_TYPE,
        [](bool) -> std::unique_ptr<SushiTrack::GatedStatePredictor>
        {
            throw std::runtime_error("injected initialisation failure");
        });
    const sushitrack_params_t params = params_with_kalman_type(K_THROWING_KALMAN_TYPE);
    sushitrack_tracker_t tracker = reinterpret_cast<sushitrack_tracker_t>(0x1);

    EXPECT_EQ(SushiTrack::create_tracker(&params, registry, &tracker),
              SUSHITRACK_INITIALIZATION_FAILURE);
    EXPECT_EQ(tracker, nullptr);
}

/**
 * @brief An exception inside sushitrack_update returns SUSHITRACK_RUNTIME_ERROR with a count
 *        of 0.
 */
TEST(Small_CreateTrackerTest, RuntimeErrorReturnsRuntimeErrorCode)
{
    bool fail_later_throws = false;
    SushiTrack::KalmanFilterRegistry registry;
    registry.register_creator(
        K_FAILING_LATER_KALMAN_TYPE,
        [&fail_later_throws](bool enable_xyah) -> std::unique_ptr<SushiTrack::GatedStatePredictor>
        {
            if (fail_later_throws)
                throw std::runtime_error("injected update failure");
            return std::make_unique<SushiTrack::KalmanFilter>(enable_xyah);
        });
    const sushitrack_params_t params = params_with_kalman_type(K_FAILING_LATER_KALMAN_TYPE);
    sushitrack_tracker_t tracker = nullptr;
    ASSERT_EQ(SushiTrack::create_tracker(&params, registry, &tracker), SUSHITRACK_SUCCESS);
    fail_later_throws = true;
    const sushitrack_object_t object = make_det(10.0f, 20.0f);
    std::vector<sushitrack_track_t> out(4);
    int count = -1;

    EXPECT_EQ(sushitrack_update(tracker, &object, 1, out.data(), 4, 0.04f, &count),
              SUSHITRACK_RUNTIME_ERROR);
    EXPECT_EQ(count, 0);
    sushitrack_destroy(tracker);
}

/**
 * @brief An allocation that fails while the tracker is built returns SUSHITRACK_OUT_OF_MEMORY
 *        and no handle.
 */
TEST(Small_CreateTrackerTest, OutOfMemoryReturnsOutOfMemoryCode)
{
    SushiTrack::KalmanFilterRegistry registry;
    registry.register_creator(
        K_OUT_OF_MEMORY_KALMAN_TYPE,
        [](bool) -> std::unique_ptr<SushiTrack::GatedStatePredictor>
        {
            throw std::bad_alloc();
        });
    const sushitrack_params_t params = params_with_kalman_type(K_OUT_OF_MEMORY_KALMAN_TYPE);
    sushitrack_tracker_t tracker = reinterpret_cast<sushitrack_tracker_t>(0x1);

    EXPECT_EQ(SushiTrack::create_tracker(&params, registry, &tracker), SUSHITRACK_OUT_OF_MEMORY);
    EXPECT_EQ(tracker, nullptr);
}
