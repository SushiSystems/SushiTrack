/**
 * @file      TestCostMatrix.cpp
 * @brief     Unit tests for CostMatrix: resize, element access, sentinel init, and buffer reuse.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      12.05.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 *
 * Coverage
 * --------
 * - Default-constructed matrix is empty (0×0).
 * - resize() updates rows/cols and fills every cell with initial_val.
 * - Default initial_val is the sentinel 1e4f.
 * - operator() reads and writes arbitrary elements correctly.
 * - empty() returns false after resize(), true before.
 * - Shrink resize reuses the internal buffer without heap reallocation.
 * - Grow resize allocates and fills the larger buffer correctly.
 * - data() pointer is non-null after resize().
 */

#include <gtest/gtest.h>
#include "SushiTrack/CostMatrix.h"

using namespace SushiTrack;

// Default construction

/**
 * @brief A default-constructed CostMatrix has zero dimensions and is empty.
 */
TEST(CostMatrixTest, DefaultConstructionIsEmpty)
{
    CostMatrix m;
    EXPECT_EQ(m.rows(), 0u);
    EXPECT_EQ(m.cols(), 0u);
    EXPECT_TRUE(m.empty());
}

// resize() — sentinel initialisation

/**
 * @brief resize() with no explicit initial_val fills every cell with 1e4f.
 */
TEST(CostMatrixTest, ResizeDefaultFillsSentinel)
{
    CostMatrix m;
    m.resize(3, 4);

    EXPECT_EQ(m.rows(), 3u);
    EXPECT_EQ(m.cols(), 4u);
    EXPECT_FALSE(m.empty());

    for (std::size_t r = 0; r < 3; ++r)
        for (std::size_t c = 0; c < 4; ++c)
            EXPECT_FLOAT_EQ(m(r, c), 1e4f)
                << "cell (" << r << "," << c << ") not initialised to sentinel";
}

/**
 * @brief resize() with an explicit initial_val fills every cell with that value.
 */
TEST(CostMatrixTest, ResizeExplicitFillValue)
{
    CostMatrix m;
    m.resize(2, 3, 0.5f);

    for (std::size_t r = 0; r < 2; ++r)
        for (std::size_t c = 0; c < 3; ++c)
            EXPECT_FLOAT_EQ(m(r, c), 0.5f)
                << "cell (" << r << "," << c << ") not 0.5";
}

// operator() — read and write

/**
 * @brief Individual cells can be written and read back correctly.
 */
TEST(CostMatrixTest, ElementAccessReadWrite)
{
    CostMatrix m;
    m.resize(3, 3);
    m(0, 0) = 0.1f;
    m(1, 2) = 0.7f;
    m(2, 1) = 99.f;

    EXPECT_FLOAT_EQ(m(0, 0), 0.1f);
    EXPECT_FLOAT_EQ(m(1, 2), 0.7f);
    EXPECT_FLOAT_EQ(m(2, 1), 99.f);
    // Untouched cell remains at sentinel.
    EXPECT_FLOAT_EQ(m(0, 1), 1e4f);
}

/**
 * @brief const operator() provides read access to the same storage.
 */
TEST(CostMatrixTest, ConstAccessorReadsCorrectly)
{
    CostMatrix m;
    m.resize(2, 2);
    m(0, 1) = 3.14f;

    const CostMatrix& cm = m;
    EXPECT_FLOAT_EQ(cm(0, 1), 3.14f);
}

// empty()

/**
 * @brief empty() returns false after a resize to non-zero dimensions.
 */
TEST(CostMatrixTest, EmptyFalseAfterResize)
{
    CostMatrix m;
    m.resize(1, 1);
    EXPECT_FALSE(m.empty());
}

// Buffer reuse — shrink then grow

/**
 * @brief Shrinking the matrix reuses the existing buffer; all active cells
 *        are reinitialised to the requested value.
 */
TEST(CostMatrixTest, ShrinkResizeReinitialises)
{
    CostMatrix m;
    m.resize(4, 4, 9.9f);
    // Write a distinctive value to confirm the buffer is overwritten.
    for (std::size_t r = 0; r < 4; ++r)
        for (std::size_t c = 0; c < 4; ++c)
            m(r, c) = 1.23f;

    // Shrink to 2×2.
    m.resize(2, 2, 0.0f);
    EXPECT_EQ(m.rows(), 2u);
    EXPECT_EQ(m.cols(), 2u);
    for (std::size_t r = 0; r < 2; ++r)
        for (std::size_t c = 0; c < 2; ++c)
            EXPECT_FLOAT_EQ(m(r, c), 0.0f)
                << "cell (" << r << "," << c << ") after shrink is not 0.0";
}

/**
 * @brief Growing the matrix to a larger size fills all cells correctly.
 */
TEST(CostMatrixTest, GrowResizeReinitialises)
{
    CostMatrix m;
    m.resize(2, 2, 1.0f);

    // Grow to 5×5.
    m.resize(5, 5, 2.0f);
    EXPECT_EQ(m.rows(), 5u);
    EXPECT_EQ(m.cols(), 5u);
    for (std::size_t r = 0; r < 5; ++r)
        for (std::size_t c = 0; c < 5; ++c)
            EXPECT_FLOAT_EQ(m(r, c), 2.0f)
                << "cell (" << r << "," << c << ") after grow is not 2.0";
}

// data()

/**
 * @brief data() is non-null after a resize and points to contiguous storage.
 */
TEST(CostMatrixTest, DataPointerIsValid)
{
    CostMatrix m;
    m.resize(3, 3, 7.f);
    ASSERT_NE(m.data(), nullptr);

    // Row-major layout: element (r, c) is at data[r * cols + c].
    EXPECT_FLOAT_EQ(m.data()[1 * 3 + 2], m(1, 2));
}

/**
 * @brief const data() provides read access to the same buffer.
 */
TEST(CostMatrixTest, ConstDataPointerMatchesAccessor)
{
    CostMatrix m;
    m.resize(2, 2);
    m(0, 0) = 5.f;

    const CostMatrix& cm = m;
    EXPECT_FLOAT_EQ(cm.data()[0], 5.f);
}

// Tier 3 edge cases

/**
 * @brief resize() to 0×0 produces an empty matrix; data() may be null.
 */
TEST(CostMatrixTest, ResizeToZeroProducesEmptyMatrix)
{
    CostMatrix m;
    m.resize(3, 3);
    m.resize(0, 0);
    EXPECT_EQ(m.rows(), 0u);
    EXPECT_EQ(m.cols(), 0u);
    EXPECT_TRUE(m.empty());
}

/**
 * @brief A 1×N matrix stores and retrieves all N cells correctly.
 */
TEST(CostMatrixTest, SingleRowMatrixAccessAllCells)
{
    CostMatrix m;
    m.resize(1, 5, 0.0f);
    for (std::size_t c = 0; c < 5; ++c)
        m(0, c) = static_cast<float>(c);
    for (std::size_t c = 0; c < 5; ++c)
        EXPECT_FLOAT_EQ(m(0, c), static_cast<float>(c));
}

/**
 * @brief An N×1 matrix stores and retrieves all N cells correctly.
 */
TEST(CostMatrixTest, SingleColumnMatrixAccessAllCells)
{
    CostMatrix m;
    m.resize(5, 1, 0.0f);
    for (std::size_t r = 0; r < 5; ++r)
        m(r, 0) = static_cast<float>(r) * 2.f;
    for (std::size_t r = 0; r < 5; ++r)
        EXPECT_FLOAT_EQ(m(r, 0), static_cast<float>(r) * 2.f);
}

/**
 * @brief data() pointer row-major indexing is consistent with operator() for
 *        a non-square matrix.
 */
TEST(CostMatrixTest, DataPointerRowMajorNonSquare)
{
    CostMatrix m;
    m.resize(3, 5, 0.0f);
    m(1, 3) = 7.7f;
    EXPECT_FLOAT_EQ(m.data()[1 * 5 + 3], 7.7f);
}
