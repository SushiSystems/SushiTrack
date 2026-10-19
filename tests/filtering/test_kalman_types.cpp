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
 * @file test_kalman_types.cpp
 * @brief Tests the shapes and the layout of the KalmanTypes matrices.
 * @author Mustafa Garip
 */

#include <cstdint>

#include <gtest/gtest.h>

#include <SushiTrack/filtering/kalman_types.hpp>

using namespace SushiTrack;

static_assert(KalmanTypes::DetectBox::ColsAtCompileTime == 4, "a measurement has four values");
static_assert(KalmanTypes::StateMean::ColsAtCompileTime == 8, "a state has eight values");
static_assert(KalmanTypes::StateCov::RowsAtCompileTime == 8 &&
                  KalmanTypes::StateCov::ColsAtCompileTime == 8,
              "a covariance is 8 x 8");

/**
 * @brief The matrices are row-major, so a measurement's values sit in [x, y, w, h] order.
 */
TEST(Small_KalmanTypesTest, MeasurementIsRowMajor)
{
    KalmanTypes::DetectBox box;
    box << 1.f, 2.f, 3.f, 4.f;

    EXPECT_FLOAT_EQ(box.data()[0], 1.f);
    EXPECT_FLOAT_EQ(box.data()[3], 4.f);
    EXPECT_TRUE(KalmanTypes::StateCov::IsRowMajor);
}

/**
 * @brief A DetectBoxVector keeps every measurement intact and aligned for Eigen.
 */
TEST(Small_KalmanTypesTest, DetectBoxVectorKeepsAlignedMeasurements)
{
    KalmanTypes::DetectBoxVector boxes;
    for (int i = 0; i < 9; ++i)
    {
        KalmanTypes::DetectBox box;
        box << 1.f * i, 2.f, 3.f, 4.f;
        boxes.push_back(box);
    }

    for (int i = 0; i < 9; ++i)
        EXPECT_FLOAT_EQ(boxes[i](0), 1.f * i);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(boxes.data()) % 16u, 0u);
}
