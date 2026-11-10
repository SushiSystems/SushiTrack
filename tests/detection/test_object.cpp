/**************************************************************************/
/* test_object.cpp                                                        */
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
 * @file test_object.cpp
 * @brief Tests Object, the detection the tracker takes in.
 * @author Mustafa Garip
 */

#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/detection/object.hpp>
#include <SushiTrack/detection/rect.hpp>

using namespace SushiTrack;

/**
 * @brief The constructor keeps the box, the label, the score and the feature it is given.
 */
TEST(Small_ObjectTest, KeepsWhatItIsGiven)
{
    const Object object(Rect<float>(10.f, 20.f, 30.f, 40.f), 3, 0.75f, {1.f, 0.f});

    EXPECT_FLOAT_EQ(object.rect.x(), 10.f);
    EXPECT_FLOAT_EQ(object.rect.y(), 20.f);
    EXPECT_FLOAT_EQ(object.rect.width(), 30.f);
    EXPECT_FLOAT_EQ(object.rect.height(), 40.f);
    EXPECT_EQ(object.label, 3);
    EXPECT_FLOAT_EQ(object.prob, 0.75f);
    EXPECT_EQ(object.feature, (std::vector<float>{1.f, 0.f}));
}

namespace
{
    /**
     * @brief Returns five objects with distinct boxes and labels, built in an ObjectVector.
     */
    ObjectVector five_objects()
    {
        ObjectVector objects;
        objects.emplace_back(Rect<float>(0.f, 0.f, 1.f, 1.f), 0, 0.5f);
        objects.emplace_back(Rect<float>(1.f, 0.f, 1.f, 1.f), 1, 0.5f);
        objects.emplace_back(Rect<float>(2.f, 0.f, 1.f, 1.f), 2, 0.5f);
        objects.emplace_back(Rect<float>(3.f, 0.f, 1.f, 1.f), 3, 0.5f);
        objects.emplace_back(Rect<float>(4.f, 0.f, 1.f, 1.f), 4, 0.5f);
        return objects;
    }
} // namespace

/**
 * @brief Every object an ObjectVector holds sits on a 16-byte boundary, as Eigen needs.
 */
TEST(Small_ObjectTest, ObjectVectorKeepsSixteenByteAlignment)
{
    const ObjectVector objects = five_objects();

    ASSERT_EQ(objects.size(), 5u);
    for (const Object& object : objects)
        EXPECT_EQ(reinterpret_cast<std::uintptr_t>(&object) % 16u, 0u);
}
