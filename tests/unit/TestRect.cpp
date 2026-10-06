/**************************************************************************/
/* TestRect.cpp                                                           */
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
 * @file TestRect.cpp
 * @brief Unit tests for bounding box geometric operations.
 * @author Mustafa Garip
 */

#include <gtest/gtest.h>
#include <SushiTrack/rect.hpp>

using namespace SushiTrack;

// Construction

/**
 * @brief Constructor stores x, y, w, h in TLWH order.
 */
TEST(RectTest, ConstructorStoresComponents)
{
    Rect<float> r(10.f, 20.f, 50.f, 80.f);
    EXPECT_FLOAT_EQ(r.x(),      10.f);
    EXPECT_FLOAT_EQ(r.y(),      20.f);
    EXPECT_FLOAT_EQ(r.width(),  50.f);
    EXPECT_FLOAT_EQ(r.height(), 80.f);
}

// getXywh() — centroid format

/**
 * @brief getXywh() returns [cx, cy, w, h] where cx = x + w/2, cy = y + h/2.
 */
TEST(RectTest, GetXywhReturnsCentroid)
{
    Rect<float> r(10.f, 20.f, 50.f, 80.f);
    auto xywh = r.getXywh();

    EXPECT_FLOAT_EQ(xywh[0], 10.f + 50.f * 0.5f);   // cx = 35
    EXPECT_FLOAT_EQ(xywh[1], 20.f + 80.f * 0.5f);   // cy = 60
    EXPECT_FLOAT_EQ(xywh[2], 50.f);
    EXPECT_FLOAT_EQ(xywh[3], 80.f);
}

// getTlbr() — bottom-right format

/**
 * @brief getTlbr() returns [x1, y1, x2, y2] where x2 = x + w, y2 = y + h.
 */
TEST(RectTest, GetTlbrReturnsBoundingCorners)
{
    Rect<float> r(10.f, 20.f, 50.f, 80.f);
    auto tlbr = r.getTlbr();

    EXPECT_FLOAT_EQ(tlbr[0], 10.f);
    EXPECT_FLOAT_EQ(tlbr[1], 20.f);
    EXPECT_FLOAT_EQ(tlbr[2], 10.f + 50.f);   // x2 = 60
    EXPECT_FLOAT_EQ(tlbr[3], 20.f + 80.f);   // y2 = 100
}

// Factory: generate_rect_by_tlbr()

/**
 * @brief generate_rect_by_tlbr() followed by getTlbr() returns the original
 *        coordinates (round-trip fidelity).
 */
TEST(RectTest, GenerateByTlbrRoundTrip)
{
    Tlbr<float> in;
    in << 10.f, 20.f, 60.f, 100.f;

    Rect<float> r = generate_rect_by_tlbr(in);
    auto out = r.getTlbr();

    EXPECT_FLOAT_EQ(out[0], in[0]);
    EXPECT_FLOAT_EQ(out[1], in[1]);
    EXPECT_FLOAT_EQ(out[2], in[2]);
    EXPECT_FLOAT_EQ(out[3], in[3]);
}

/**
 * @brief generate_rect_by_tlbr() produces correct width and height from
 *        the input corner coordinates.
 */
TEST(RectTest, GenerateByTlbrProducesCorrectDimensions)
{
    Tlbr<float> in;
    in << 10.f, 20.f, 60.f, 100.f;

    Rect<float> r = generate_rect_by_tlbr(in);

    EXPECT_FLOAT_EQ(r.x(),      10.f);
    EXPECT_FLOAT_EQ(r.y(),      20.f);
    EXPECT_FLOAT_EQ(r.width(),  50.f);   // 60 - 10
    EXPECT_FLOAT_EQ(r.height(), 80.f);   // 100 - 20
}

// Factory: generate_rect_by_xywh()

/**
 * @brief generate_rect_by_xywh() followed by getXywh() returns the original
 *        centroid coordinates (round-trip fidelity).
 */
TEST(RectTest, GenerateByXywhRoundTrip)
{
    Xywh<float> in;
    in << 35.f, 60.f, 50.f, 80.f;   // cx=35, cy=60, w=50, h=80

    Rect<float> r = generate_rect_by_xywh(in);
    auto out = r.getXywh();

    EXPECT_NEAR(out[0], in[0], 1e-4f);
    EXPECT_NEAR(out[1], in[1], 1e-4f);
    EXPECT_FLOAT_EQ(out[2], in[2]);
    EXPECT_FLOAT_EQ(out[3], in[3]);
}

/**
 * @brief generate_rect_by_xywh() converts centroid back to correct top-left
 *        coordinates: x = cx - w/2, y = cy - h/2.
 */
TEST(RectTest, GenerateByXywhProducesCorrectTopLeft)
{
    Xywh<float> in;
    in << 35.f, 60.f, 50.f, 80.f;

    Rect<float> r = generate_rect_by_xywh(in);

    EXPECT_NEAR(r.x(), 35.f - 25.f, 1e-4f);   // cx - w/2 = 10
    EXPECT_NEAR(r.y(), 60.f - 40.f, 1e-4f);   // cy - h/2 = 20
}

// Mutable accessors

/**
 * @brief Mutable accessors allow modifying individual components in-place.
 */
TEST(RectTest, MutableAccessorsModifyComponents)
{
    Rect<float> r(10.f, 20.f, 50.f, 80.f);
    r.x()      = 15.f;
    r.y()      = 25.f;
    r.width()  = 60.f;
    r.height() = 90.f;

    EXPECT_FLOAT_EQ(r.x(),      15.f);
    EXPECT_FLOAT_EQ(r.y(),      25.f);
    EXPECT_FLOAT_EQ(r.width(),  60.f);
    EXPECT_FLOAT_EQ(r.height(), 90.f);
}

// Integer specialisation smoke test

/**
 * @brief Rect<int> compiles and stores integer coordinates correctly.
 */
TEST(RectTest, IntSpecialisationCompiles)
{
    Rect<int> r(0, 0, 100, 200);
    EXPECT_EQ(r.width(),  100);
    EXPECT_EQ(r.height(), 200);
}

// Tier 3 edge cases

/**
 * @brief A zero-width, zero-height Rect stores dimensions as zero without
 *        crashing and getTlbr() returns coincident corners.
 */
TEST(RectTest, ZeroSizeRectHasCoincidentCorners)
{
    Rect<float> r(5.f, 10.f, 0.f, 0.f);
    auto tlbr = r.getTlbr();
    EXPECT_FLOAT_EQ(tlbr[0], 5.f);
    EXPECT_FLOAT_EQ(tlbr[1], 10.f);
    EXPECT_FLOAT_EQ(tlbr[2], 5.f);
    EXPECT_FLOAT_EQ(tlbr[3], 10.f);
}

/**
 * @brief Negative top-left coordinates are stored and converted correctly;
 *        getXywh() returns the correct centroid for boxes with negative origin.
 */
TEST(RectTest, NegativeOriginCentroidIsCorrect)
{
    Rect<float> r(-20.f, -40.f, 40.f, 80.f);
    auto xywh = r.getXywh();
    EXPECT_FLOAT_EQ(xywh[0], 0.f);   // cx = -20 + 20
    EXPECT_FLOAT_EQ(xywh[1], 0.f);   // cy = -40 + 40
}

/**
 * @brief generate_rect_by_tlbr() with a square box produces equal width and height.
 */
TEST(RectTest, SquareBoxFromTlbrHasEqualDimensions)
{
    Tlbr<float> in;
    in << 0.f, 0.f, 50.f, 50.f;
    Rect<float> r = generate_rect_by_tlbr(in);
    EXPECT_FLOAT_EQ(r.width(), r.height());
}

/**
 * @brief generate_rect_by_xywh() with a very large box does not overflow
 *        and getTlbr() round-trips correctly.
 */
TEST(RectTest, LargeBoxRoundTripIsExact)
{
    Xywh<float> in;
    in << 1000.f, 2000.f, 400.f, 800.f;
    Rect<float> r = generate_rect_by_xywh(in);
    auto out = r.getXywh();
    EXPECT_NEAR(out[0], 1000.f, 1e-3f);
    EXPECT_NEAR(out[1], 2000.f, 1e-3f);
    EXPECT_FLOAT_EQ(out[2], 400.f);
    EXPECT_FLOAT_EQ(out[3], 800.f);
}

/**
 * @brief Verifies that a rectangle with zero dimensions (width/height) is handled correctly.
 * 
 * Validates that the top-left and bottom-right corners coincide when dimensions are zero.
 */
TEST(RectTest, HandlesZeroSize) 
{
    Rect<float> rect(10.0f, 15.0f, 0.0f, 0.0f);
    auto tlbr = rect.getTlbr();
    EXPECT_FLOAT_EQ(tlbr[0], 10.0f); // Top/X1
    EXPECT_FLOAT_EQ(tlbr[1], 15.0f); // Left/Y1
    EXPECT_FLOAT_EQ(tlbr[2], 10.0f); // Bottom/X2 (Top + Height)
    EXPECT_FLOAT_EQ(tlbr[3], 15.0f); // Right/Y2 (Left + Width)
}

/**
 * @brief Ensures that negative coordinates are preserved in the internal representation.
 * 
 * Confirms that the top-left coordinates match the input even when they are negative.
 */
TEST(RectTest, AllowsNegativeCoordinates) 
{
    Rect<float> rect(-20.5f, -10.0f, 50.0f, 40.0f);
    auto tlwh = rect.getTlwh();
    EXPECT_FLOAT_EQ(tlwh[0], -20.5f);
    EXPECT_FLOAT_EQ(tlwh[1], -10.0f);
}

/**
 * @brief Validates behavior when TLBR input has reversed corners (e.g., top > bottom).
 * 
 * Verifies that the factory function produces a rectangle with negative dimensions 
 * when the input corners are provided in reverse order.
 */
TEST(RectTest, GenerateByTlbrReversesCorners) 
{
    Tlbr<float> tlbr;
    tlbr << 50.0f, 60.0f, 10.0f, 20.0f; // Top > Bottom, Left > Right
    Rect<float> rect = generate_rect_by_tlbr(tlbr);
    auto tlwh = rect.getTlwh();
    
    // width = x2 - x1 = 10 - 50 = -40
    EXPECT_FLOAT_EQ(tlwh[2], -40.0f); 
    // height = y2 - y1 = 20 - 60 = -40
    EXPECT_FLOAT_EQ(tlwh[3], -40.0f); 
}

/**
 * @brief Tests aspect ratio calculation based on XYWH representation.
 * 
 * Ensures that the aspect ratio (width/height) derived from the centroid format 
 * matches the expected value for a standard resolution box.
 */
TEST(RectTest, XywhAspectRatioCalculation) {
    Rect<float> rect(0.0f, 0.0f, 1920.0f, 1080.0f);
    auto xywh = rect.getXywh(); // {cx, cy, w, h}
    float aspect_ratio = xywh[2] / xywh[3];
    EXPECT_FLOAT_EQ(aspect_ratio, 1920.0f / 1080.0f);
}
