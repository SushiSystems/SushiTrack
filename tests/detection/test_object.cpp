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
/* Licensed under the PolyForm Noncommercial License 1.0.0 (the           */
/* "License"); you may not use this file except in compliance with the    */
/* License. You may obtain a copy of the License at                       */
/*                                                                        */
/*     https://polyformproject.org/licenses/noncommercial/1.0.0           */
/*                                                                        */
/* Noncommercial use is free. Commercial use requires a separate licence  */
/* from Sushi Systems; see COMMERCIAL.md. The software is provided        */
/* "as is", without warranty of any kind.                                 */
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

/**
 * @brief An object built without a feature carries an empty one.
 */
TEST(Small_ObjectTest, FeatureDefaultsToEmpty)
{
    const Object object(Rect<float>(0.f, 0.f, 1.f, 1.f), 0, 0.5f);
    EXPECT_TRUE(object.feature.empty());
}

/**
 * @brief Every object an ObjectVector holds sits on a 16-byte boundary, as Eigen needs.
 */
TEST(Small_ObjectTest, ObjectVectorKeepsSixteenByteAlignment)
{
    ObjectVector objects;
    for (int i = 0; i < 5; ++i)
        objects.emplace_back(Rect<float>(1.f * i, 0.f, 1.f, 1.f), i, 0.5f);

    for (const Object& object : objects)
        EXPECT_EQ(reinterpret_cast<std::uintptr_t>(&object) % 16u, 0u);
}
