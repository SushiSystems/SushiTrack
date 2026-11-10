/**************************************************************************/
/* test_kalman_types.cpp                                                  */
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
 * @file test_kalman_types.cpp
 * @brief Tests the shapes and the layout of the KalmanTypes matrices.
 * @author Mustafa Garip
 */

#include <cstdint>

#include <gtest/gtest.h>

#include <SushiTrack/filtering/kalman_types.hpp>

#include <measurement_builders.hpp>

using namespace SushiTrack;

static_assert(KalmanTypes::DetectBox::ColsAtCompileTime == 4, "a measurement has four values");
static_assert(KalmanTypes::StateMean::ColsAtCompileTime == 8, "a state has eight values");
static_assert(KalmanTypes::StateCov::RowsAtCompileTime == 8 &&
                  KalmanTypes::StateCov::ColsAtCompileTime == 8,
              "a covariance is 8 x 8");

namespace
{
    /**
     * @brief Returns nine measurements whose first value counts from 0 to 8, pushed one by one so
     *        the vector reallocates while it grows.
     */
    KalmanTypes::DetectBoxVector nine_boxes()
    {
        KalmanTypes::DetectBoxVector boxes;
        boxes.push_back(make_box(0.f, 2.f, 3.f, 4.f));
        boxes.push_back(make_box(1.f, 2.f, 3.f, 4.f));
        boxes.push_back(make_box(2.f, 2.f, 3.f, 4.f));
        boxes.push_back(make_box(3.f, 2.f, 3.f, 4.f));
        boxes.push_back(make_box(4.f, 2.f, 3.f, 4.f));
        boxes.push_back(make_box(5.f, 2.f, 3.f, 4.f));
        boxes.push_back(make_box(6.f, 2.f, 3.f, 4.f));
        boxes.push_back(make_box(7.f, 2.f, 3.f, 4.f));
        boxes.push_back(make_box(8.f, 2.f, 3.f, 4.f));
        return boxes;
    }
} // namespace

/**
 * @brief A DetectBoxVector keeps every measurement intact and aligned for Eigen.
 */
TEST(Small_KalmanTypesTest, DetectBoxVectorKeepsAlignedMeasurements)
{
    const KalmanTypes::DetectBoxVector boxes = nine_boxes();

    ASSERT_EQ(boxes.size(), 9u);
    EXPECT_FLOAT_EQ(boxes.front()(0), 0.f);
    EXPECT_FLOAT_EQ(boxes.back()(0), 8.f);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(boxes.data()) % 16u, 0u);
}
