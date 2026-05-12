/**
 * @file      test_rect.cpp
 * @brief     Unit tests for Rect<float>: coordinate conversions and factory functions.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      12.05.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 *
 * Coverage
 * --------
 * - Constructor stores (x, y, w, h) correctly.
 * - getXywh() returns [cx, cy, w, h] centroid format.
 * - getTlbr() returns [x1, y1, x2, y2] format.
 * - generate_rect_by_tlbr() round-trips through getTlbr().
 * - generate_rect_by_xywh() round-trips through getXywh().
 * - Mutable accessors modify stored values.
 */

#include <gtest/gtest.h>
#include "SushiTrack/Rect.h"

using namespace SushiTrack;

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// getXywh() — centroid format
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// getTlbr() — bottom-right format
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// Factory: generate_rect_by_tlbr()
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// Factory: generate_rect_by_xywh()
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// Mutable accessors
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// Integer specialisation smoke test
// ---------------------------------------------------------------------------

/**
 * @brief Rect<int> compiles and stores integer coordinates correctly.
 */
TEST(RectTest, IntSpecialisationCompiles)
{
    Rect<int> r(0, 0, 100, 200);
    EXPECT_EQ(r.width(),  100);
    EXPECT_EQ(r.height(), 200);
}
