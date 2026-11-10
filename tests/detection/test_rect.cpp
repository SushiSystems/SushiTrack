/**************************************************************************/
/* test_rect.cpp                                                          */
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
 * @file test_rect.cpp
 * @brief Tests Rect and the rectangle conversions.
 * @author Mustafa Garip
 */

#include <gtest/gtest.h>

#include <SushiTrack/detection/rect.hpp>

using namespace SushiTrack;

/**
 * @brief get_xywh() returns [cx, cy, w, h] where cx = x + w/2, cy = y + h/2.
 */
TEST(Small_RectTest, GetXywhReturnsCentroid)
{
    Rect<float> r(10.f, 20.f, 50.f, 80.f);
    auto xywh = r.get_xywh();

    EXPECT_FLOAT_EQ(xywh[0], 10.f + 50.f * 0.5f); // cx = 35
    EXPECT_FLOAT_EQ(xywh[1], 20.f + 80.f * 0.5f); // cy = 60
    EXPECT_FLOAT_EQ(xywh[2], 50.f);
    EXPECT_FLOAT_EQ(xywh[3], 80.f);
}

/**
 * @brief get_tlbr() returns [x1, y1, x2, y2] where x2 = x + w, y2 = y + h.
 */
TEST(Small_RectTest, GetTlbrReturnsBoundingCorners)
{
    Rect<float> r(10.f, 20.f, 50.f, 80.f);
    auto tlbr = r.get_tlbr();

    EXPECT_FLOAT_EQ(tlbr[0], 10.f);
    EXPECT_FLOAT_EQ(tlbr[1], 20.f);
    EXPECT_FLOAT_EQ(tlbr[2], 10.f + 50.f); // x2 = 60
    EXPECT_FLOAT_EQ(tlbr[3], 20.f + 80.f); // y2 = 100
}

/**
 * @brief generate_rect_by_tlbr() followed by get_tlbr() returns the original
 *        coordinates (round-trip fidelity).
 */
TEST(Small_RectTest, GenerateByTlbrRoundTrip)
{
    Tlbr<float> in;
    in << 10.f, 20.f, 60.f, 100.f;

    Rect<float> r = generate_rect_by_tlbr(in);
    auto out = r.get_tlbr();

    EXPECT_FLOAT_EQ(out[0], in[0]);
    EXPECT_FLOAT_EQ(out[1], in[1]);
    EXPECT_FLOAT_EQ(out[2], in[2]);
    EXPECT_FLOAT_EQ(out[3], in[3]);
}

namespace
{
    /**
     * @brief One tlbr input and the tlwh box generate_rect_by_tlbr() must give for it.
     */
    struct TlbrToTlwhRow
    {
            float tlbr[4];
            float tlwh[4];
    };
} // namespace

/**
 * @brief generate_rect_by_tlbr() produces x, y, width = x2 - x1 and height = y2 - y1, for a
 *        rectangular and a square box.
 */
class Small_RectTlbrDimensionsTest : public ::testing::TestWithParam<TlbrToTlwhRow>
{
};

TEST_P(Small_RectTlbrDimensionsTest, GenerateByTlbrProducesCorrectDimensions)
{
    Tlbr<float> in;
    in << GetParam().tlbr[0], GetParam().tlbr[1], GetParam().tlbr[2], GetParam().tlbr[3];

    Rect<float> r = generate_rect_by_tlbr(in);

    EXPECT_FLOAT_EQ(r.x(), GetParam().tlwh[0]);
    EXPECT_FLOAT_EQ(r.y(), GetParam().tlwh[1]);
    EXPECT_FLOAT_EQ(r.width(), GetParam().tlwh[2]);
    EXPECT_FLOAT_EQ(r.height(), GetParam().tlwh[3]);
}

INSTANTIATE_TEST_SUITE_P(Boxes, Small_RectTlbrDimensionsTest,
                         ::testing::Values(
                             // width 60 - 10 = 50, height 100 - 20 = 80
                             TlbrToTlwhRow{{10.f, 20.f, 60.f, 100.f}, {10.f, 20.f, 50.f, 80.f}},
                             // width 50 - 0 = 50, height 50 - 0 = 50
                             TlbrToTlwhRow{{0.f, 0.f, 50.f, 50.f}, {0.f, 0.f, 50.f, 50.f}}));

/**
 * @brief generate_rect_by_xywh() followed by get_xywh() returns the original
 *        centroid coordinates (round-trip fidelity).
 *
 * The values are exact in float: x = 35 - 50/2 = 10 and y = 60 - 80/2 = 20, and back,
 * 10 + 50/2 = 35 and 20 + 80/2 = 60, all dyadic.
 */
TEST(Small_RectTest, GenerateByXywhRoundTrip)
{
    Xywh<float> in;
    in << 35.f, 60.f, 50.f, 80.f; // cx=35, cy=60, w=50, h=80

    Rect<float> r = generate_rect_by_xywh(in);
    auto out = r.get_xywh();

    EXPECT_FLOAT_EQ(out[0], in[0]);
    EXPECT_FLOAT_EQ(out[1], in[1]);
    EXPECT_FLOAT_EQ(out[2], in[2]);
    EXPECT_FLOAT_EQ(out[3], in[3]);
}

/**
 * @brief generate_rect_by_xywh() converts centroid back to correct top-left
 *        coordinates: x = cx - w/2, y = cy - h/2.
 */
TEST(Small_RectTest, GenerateByXywhProducesCorrectTopLeft)
{
    Xywh<float> in;
    in << 35.f, 60.f, 50.f, 80.f;

    Rect<float> r = generate_rect_by_xywh(in);

    EXPECT_FLOAT_EQ(r.x(), 35.f - 25.f); // cx - w/2 = 10
    EXPECT_FLOAT_EQ(r.y(), 60.f - 40.f); // cy - h/2 = 20
}

/**
 * @brief A zero-width, zero-height Rect stores dimensions as zero without
 *        crashing and get_tlbr() returns coincident corners.
 */
TEST(Small_RectTest, ZeroSizeRectHasCoincidentCorners)
{
    Rect<float> r(5.f, 10.f, 0.f, 0.f);
    auto tlbr = r.get_tlbr();
    EXPECT_FLOAT_EQ(tlbr[0], 5.f);
    EXPECT_FLOAT_EQ(tlbr[1], 10.f);
    EXPECT_FLOAT_EQ(tlbr[2], 5.f);
    EXPECT_FLOAT_EQ(tlbr[3], 10.f);
}

/**
 * @brief Negative top-left coordinates are stored as given, and get_xywh() returns the correct
 *        centroid for a box with a negative origin.
 */
TEST(Small_RectTest, NegativeOriginCentroidIsCorrect)
{
    Rect<float> r(-20.f, -40.f, 40.f, 80.f);
    auto tlwh = r.get_tlwh();
    auto xywh = r.get_xywh();

    EXPECT_FLOAT_EQ(tlwh[0], -20.f);
    EXPECT_FLOAT_EQ(tlwh[1], -40.f);
    EXPECT_FLOAT_EQ(xywh[0], 0.f); // cx = -20 + 20
    EXPECT_FLOAT_EQ(xywh[1], 0.f); // cy = -40 + 40
}

/**
 * @brief Validates behavior when TLBR input has reversed corners (e.g., top > bottom).
 *
 * Verifies that the factory function produces a rectangle with negative dimensions
 * when the input corners are provided in reverse order.
 */
TEST(Small_RectTest, GenerateByTlbrReversesCorners)
{
    Tlbr<float> tlbr;
    tlbr << 50.0f, 60.0f, 10.0f, 20.0f; // Top > Bottom, Left > Right
    Rect<float> rect = generate_rect_by_tlbr(tlbr);
    auto tlwh = rect.get_tlwh();

    // width = x2 - x1 = 10 - 50 = -40
    EXPECT_FLOAT_EQ(tlwh[2], -40.0f);
    // height = y2 - y1 = 20 - 60 = -40
    EXPECT_FLOAT_EQ(tlwh[3], -40.0f);
}
