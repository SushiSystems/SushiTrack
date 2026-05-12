/**
 * @file      TestDistanceStrategies.cpp
 * @brief     Unit tests for IoU, DIoU, Cosine, FuseScoreDecorator cost calculators.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      12.05.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 *
 * Coverage
 * --------
 * IoUCostCalculator
 *   - Identical boxes produce cost ≈ 0 (IoU = 1.0).
 *   - Non-overlapping boxes produce cost = 1.0.
 *   - Partial overlap produces 0 < cost < 1.
 *   - Area-ratio violation produces sentinel cost (1e4f).
 *   - Empty tracklet collection produces empty matrix.
 *   - calculate_single agrees with the batch path.
 *
 * DIoUCostCalculator
 *   - Identical boxes produce cost ≈ 0.
 *   - Separated boxes produce cost > 0.
 *   - calculate_single agrees with the batch path.
 *
 * CosineCostCalculator
 *   - Identical unit features produce cost ≈ 0.
 *   - Orthogonal features produce cost ≈ 1.
 *   - Missing feature on either side produces cost = 1.
 *   - calculate_single agrees with the batch path.
 *
 * FuseScoreDecorator
 *   - High-score detection lowers the effective cost.
 *   - Score = 1 does not change the cost (neutral).
 *   - Sentinel cells (≥ 1e3) are preserved unchanged.
 *   - calculate_single applies the same fusion formula.
 */

#include <gtest/gtest.h>
#include "SushiTrack/DistanceStrategies.h"
#include "SushiTrack/Tracklet.h"
#include "SushiTrack/KalmanFilter.h"
#include "SushiTrack/Rect.h"
#include "SushiTrack/CostMatrix.h"

#include <memory>
#include <vector>
#include <cmath>

using namespace SushiTrack;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/** @brief Returns a Tracklet with TLWH (x, y, w, h) at score s. */
static std::shared_ptr<Tracklet> makeT(float x, float y, float w, float h,
                                       float score = 0.9f,
                                       std::vector<float> feat = {})
{
    return std::make_shared<Tracklet>(
        std::make_unique<KalmanFilter>(),
        Rect<float>(x, y, w, h),
        /*label=*/1, score,
        std::move(feat)
    );
}

/** @brief Returns a unit-feature Tracklet along the given basis axis. */
static std::shared_ptr<Tracklet> makeFeatT(float x, float y,
                                           int feat_dim, int axis,
                                           float score = 0.9f)
{
    std::vector<float> feat(feat_dim, 0.f);
    feat[axis] = 1.f;
    return makeT(x, y, 50.f, 80.f, score, std::move(feat));
}

// ===========================================================================
// IoUCostCalculator
// ===========================================================================

/** @brief Identical boxes — IoU = 1.0 → cost ≈ 0. */
TEST(IoUCostCalculatorTest, IdenticalBoxesCostIsZero)
{
    IoUCostCalculator calc;
    auto a = makeT(100.f, 100.f, 50.f, 80.f);
    auto b = makeT(100.f, 100.f, 50.f, 80.f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), 0.f, 1e-4f);
}

/** @brief Non-overlapping boxes — IoU = 0 → cost = 1.0. */
TEST(IoUCostCalculatorTest, NonOverlappingBoxesCostIsOne)
{
    IoUCostCalculator calc;
    auto a = makeT(  0.f, 0.f, 50.f, 50.f);
    auto b = makeT(200.f, 0.f, 50.f, 50.f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_FLOAT_EQ(m(0, 0), 1.f);
}

/** @brief Partial overlap produces 0 < cost < 1. */
TEST(IoUCostCalculatorTest, PartialOverlapCostInRange)
{
    IoUCostCalculator calc;
    // 50×50 boxes with 25-pixel horizontal shift → intersection = 25×50.
    auto a = makeT( 0.f, 0.f, 50.f, 50.f);
    auto b = makeT(25.f, 0.f, 50.f, 50.f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_GT(m(0, 0), 0.f);
    EXPECT_LT(m(0, 0), 1.f);
}

/** @brief Area-ratio violation (max_area_ratio=4) → sentinel 1e4f. */
TEST(IoUCostCalculatorTest, AreaRatioViolationIsSentinel)
{
    IoUCostCalculator calc(4.0f);
    // Area ratio = (200*200)/(10*10) = 400, far above 4.
    auto big   = makeT(0.f, 0.f, 200.f, 200.f);
    auto small_ = makeT(0.f, 0.f,  10.f,  10.f);

    std::vector<std::shared_ptr<Tracklet>> av{big}, bv{small_};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_GE(m(0, 0), 1e3f);
}

/** @brief Empty column collection produces an empty matrix. */
TEST(IoUCostCalculatorTest, EmptyBCollectionProducesEmptyMatrix)
{
    IoUCostCalculator calc;
    auto a = makeT(0.f, 0.f, 50.f, 50.f);
    std::vector<std::shared_ptr<Tracklet>> av{a}, bv;
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_TRUE(m.empty());
}

/** @brief calculate_single agrees with the batch matrix for an overlapping pair. */
TEST(IoUCostCalculatorTest, SingleMatchesBatch)
{
    IoUCostCalculator calc;
    auto a = makeT(0.f, 0.f, 100.f, 100.f);
    auto b = makeT(50.f, 0.f, 100.f, 100.f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    float single = calc.calculate_single(a, b);
    EXPECT_NEAR(m(0, 0), single, 1e-5f);
}

// ===========================================================================
// DIoUCostCalculator
// ===========================================================================

/** @brief Identical boxes — cost ≈ 0. */
TEST(DIoUCostCalculatorTest, IdenticalBoxesCostIsZero)
{
    DIoUCostCalculator calc;
    auto a = makeT(50.f, 50.f, 80.f, 80.f);
    auto b = makeT(50.f, 50.f, 80.f, 80.f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), 0.f, 1e-4f);
}

/** @brief Separated boxes (no overlap) give a cost > 0. */
TEST(DIoUCostCalculatorTest, SeparatedBoxesCostPositive)
{
    DIoUCostCalculator calc;
    auto a = makeT(  0.f, 0.f, 50.f, 50.f);
    auto b = makeT(300.f, 0.f, 50.f, 50.f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_GT(m(0, 0), 0.f);
}

/** @brief calculate_single agrees with batch for a partial overlap pair. */
TEST(DIoUCostCalculatorTest, SingleMatchesBatch)
{
    DIoUCostCalculator calc;
    auto a = makeT(0.f, 0.f, 100.f, 100.f);
    auto b = makeT(40.f, 40.f, 100.f, 100.f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), calc.calculate_single(a, b), 1e-5f);
}

// ===========================================================================
// CosineCostCalculator
// ===========================================================================

/** @brief Same unit-feature tracklets → cosine similarity 1 → cost ≈ 0. */
TEST(CosineCostCalculatorTest, IdenticalFeaturesCostIsZero)
{
    CosineCostCalculator calc;
    auto a = makeFeatT(0.f, 0.f, 3, 0);
    auto b = makeFeatT(0.f, 0.f, 3, 0);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), 0.f, 1e-4f);
}

/** @brief Orthogonal features → cosine similarity 0 → cost ≈ 1. */
TEST(CosineCostCalculatorTest, OrthogonalFeaturesCostIsOne)
{
    CosineCostCalculator calc;
    auto a = makeFeatT(0.f, 0.f, 3, 0);  // [1, 0, 0]
    auto b = makeFeatT(0.f, 0.f, 3, 1);  // [0, 1, 0]

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), 1.f, 1e-4f);
}

/** @brief Missing feature on 'a' side → cost = 1.0. */
TEST(CosineCostCalculatorTest, MissingFeatureOnAProducesCostOne)
{
    CosineCostCalculator calc;
    auto a = makeT(0.f, 0.f, 50.f, 80.f);              // no feature
    auto b = makeFeatT(0.f, 0.f, 3, 0);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_FLOAT_EQ(m(0, 0), 1.f);
}

/** @brief calculate_single agrees with batch for identical-feature pair. */
TEST(CosineCostCalculatorTest, SingleMatchesBatch)
{
    CosineCostCalculator calc;
    auto a = makeFeatT(0.f, 0.f, 4, 2);
    auto b = makeFeatT(0.f, 0.f, 4, 2);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), calc.calculate_single(a, b), 1e-5f);
}

// ===========================================================================
// FuseScoreDecorator
// ===========================================================================

/** @brief A detection score < 1 lowers the effective cost below the raw IoU cost. */
TEST(FuseScoreDecoratorTest, HighScoreLowersCost)
{
    // Inner calculator: IoU on identical boxes → cost = 0.
    // Decorated cost = 1 - (1 - 0) * 0.8 = 0.2.
    auto inner = std::make_shared<IoUCostCalculator>();
    FuseScoreDecorator dec(inner);

    auto a = makeT(0.f, 0.f, 50.f, 80.f, /*score=*/0.9f);
    auto b = makeT(0.f, 0.f, 50.f, 80.f, /*score=*/0.8f);  // score on b (detection)

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    dec.calculate(av, bv, m);

    // Raw IoU cost = 0.  Fused = 1 - (1-0)*0.8 = 0.2.
    EXPECT_NEAR(m(0, 0), 0.2f, 1e-4f);
}

/** @brief Score = 1.0 leaves the cost unchanged. */
TEST(FuseScoreDecoratorTest, ScoreOnePreservesCost)
{
    auto inner = std::make_shared<IoUCostCalculator>();
    FuseScoreDecorator dec(inner);

    // Partially overlapping boxes: cost will be some value c.
    auto a = makeT(  0.f, 0.f, 100.f, 100.f, 0.9f);
    auto b = makeT( 50.f, 0.f, 100.f, 100.f, 1.0f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix raw_m, fused_m;
    inner->calculate(av, bv, raw_m);
    dec.calculate(av, bv, fused_m);

    // fused = 1 - (1 - c) * 1.0 = c  → same as raw.
    EXPECT_NEAR(fused_m(0, 0), raw_m(0, 0), 1e-5f);
}

/** @brief Sentinel cells (≥ 1e3) produced by the inner calculator are preserved. */
TEST(FuseScoreDecoratorTest, SentinelCellsPreserved)
{
    auto inner = std::make_shared<IoUCostCalculator>(4.0f);
    FuseScoreDecorator dec(inner);

    // Area ratio >> 4 → sentinel in the inner matrix.
    auto big   = makeT(0.f, 0.f, 300.f, 300.f, 0.9f);
    auto small_ = makeT(0.f, 0.f,  10.f,  10.f, 0.5f);

    std::vector<std::shared_ptr<Tracklet>> av{big}, bv{small_};
    CostMatrix m;
    dec.calculate(av, bv, m);

    EXPECT_GE(m(0, 0), 1e3f);
}

/** @brief calculate_single applies the same formula as the batch path. */
TEST(FuseScoreDecoratorTest, SingleMatchesBatch)
{
    auto inner = std::make_shared<IoUCostCalculator>();
    FuseScoreDecorator dec(inner);

    auto a = makeT(  0.f, 0.f, 100.f, 100.f, 0.9f);
    auto b = makeT( 30.f, 0.f, 100.f, 100.f, 0.7f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    dec.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), dec.calculate_single(a, b), 1e-5f);
}
