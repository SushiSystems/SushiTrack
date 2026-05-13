/**
 * @file      TestDistanceStrategies.cpp
 * @brief     Unit tests for IoU, DIoU, Cosine, FuseScoreDecorator, GatedIoU, Fused, ReIDFirst, and PureReID cost calculators.
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
#include "SushiTrack/IGateValidator.h"

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

// ===========================================================================
// GatedIoUCostCalculator
// ===========================================================================

/**
 * @brief NullGateValidator passes every cell; identical boxes yield IoU cost ≈ 0.
 */
TEST(GatedIoUCostCalculatorTest, NullGatePassesAllCellsFillsIoUCost)
{
    auto iou  = std::make_shared<IoUCostCalculator>();
    GatedIoUCostCalculator calc(9.4877f, NullGateValidator::getInstance(), iou);

    auto a = makeT(100.f, 100.f, 50.f, 80.f);
    auto b = makeT(100.f, 100.f, 50.f, 80.f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), 0.f, 1e-4f);
}

/**
 * @brief NullGateValidator passes every cell; non-overlapping boxes yield cost = 1.
 */
TEST(GatedIoUCostCalculatorTest, NullGateNonOverlappingProducesCostOne)
{
    auto iou = std::make_shared<IoUCostCalculator>();
    GatedIoUCostCalculator calc(9.4877f, NullGateValidator::getInstance(), iou);

    auto a = makeT(  0.f, 0.f, 50.f, 50.f);
    auto b = makeT(200.f, 0.f, 50.f, 50.f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_FLOAT_EQ(m(0, 0), 1.f);
}

/**
 * @brief A detection placed 5000 px from the activated tracklet fails the
 *        Mahalanobis gate and produces a sentinel value.
 */
TEST(GatedIoUCostCalculatorTest, FailedGateProducesSentinel)
{
    auto iou = std::make_shared<IoUCostCalculator>();
    GatedIoUCostCalculator calc(9.4877f, std::make_shared<KalmanFilter>(), iou);

    auto a = makeT(100.f, 100.f, 50.f, 80.f);
    a->activate(std::make_unique<KalmanFilter>(), 1, 1);
    a->predict();

    auto b = makeT(5100.f, 100.f, 50.f, 80.f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_GE(m(0, 0), 1e3f);
}

/**
 * @brief An empty detection collection produces an empty cost matrix.
 */
TEST(GatedIoUCostCalculatorTest, EmptyCollectionProducesEmptyMatrix)
{
    auto iou = std::make_shared<IoUCostCalculator>();
    GatedIoUCostCalculator calc(9.4877f, NullGateValidator::getInstance(), iou);

    auto a = makeT(0.f, 0.f, 50.f, 50.f);
    std::vector<std::shared_ptr<Tracklet>> av{a}, bv;
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_TRUE(m.empty());
}

/**
 * @brief calculate_single returns the same value as the corresponding batch cell.
 */
TEST(GatedIoUCostCalculatorTest, SingleMatchesBatch)
{
    auto iou = std::make_shared<IoUCostCalculator>();
    GatedIoUCostCalculator calc(9.4877f, NullGateValidator::getInstance(), iou);

    auto a = makeT(  0.f, 0.f, 100.f, 100.f);
    auto b = makeT( 30.f, 0.f, 100.f, 100.f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), calc.calculate_single(a, b), 1e-5f);
}

// ===========================================================================
// FusedCostCalculator
// ===========================================================================

/**
 * @brief NullGate + identical features → cosine_dist = 0 < reid_thresh → direct
 *        assignment path → cost near 0.
 */
TEST(FusedCostCalculatorTest, NullGateIdenticalFeaturesDirectAssignment)
{
    auto iou    = std::make_shared<IoUCostCalculator>();
    auto cosine = std::make_shared<CosineCostCalculator>();
    FusedCostCalculator calc(0.5f, 0.3f, 9.4877f,
                             NullGateValidator::getInstance(),
                             iou, cosine, /*reid_direct_assignment_weight=*/0.5f);

    auto a = makeFeatT(100.f, 100.f, 4, 0);
    auto b = makeFeatT(100.f, 100.f, 4, 0);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), 0.f, 1e-4f);
}

/**
 * @brief A detection 5000 px from an activated tracklet fails the real KF gate
 *        and receives a sentinel value.
 */
TEST(FusedCostCalculatorTest, NullGateSentinelPreservedWhenGateFails)
{
    auto iou    = std::make_shared<IoUCostCalculator>();
    auto cosine = std::make_shared<CosineCostCalculator>();
    FusedCostCalculator calc(0.5f, 0.3f, 9.4877f,
                             std::make_shared<KalmanFilter>(),
                             iou, cosine);

    auto a = makeT(100.f, 100.f, 50.f, 80.f);
    a->activate(std::make_unique<KalmanFilter>(), 1, 1);
    a->predict();

    auto b = makeT(5100.f, 100.f, 50.f, 80.f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_GE(m(0, 0), 1e3f);
}

/**
 * @brief An empty detection collection produces an empty cost matrix.
 */
TEST(FusedCostCalculatorTest, EmptyCollectionProducesEmptyMatrix)
{
    auto iou    = std::make_shared<IoUCostCalculator>();
    auto cosine = std::make_shared<CosineCostCalculator>();
    FusedCostCalculator calc(0.5f, 0.3f, 9.4877f,
                             NullGateValidator::getInstance(),
                             iou, cosine);

    auto a = makeFeatT(0.f, 0.f, 4, 0);
    std::vector<std::shared_ptr<Tracklet>> av{a}, bv;
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_TRUE(m.empty());
}

/**
 * @brief calculate_single returns the same value as the corresponding batch cell.
 */
TEST(FusedCostCalculatorTest, SingleMatchesBatch)
{
    auto iou    = std::make_shared<IoUCostCalculator>();
    auto cosine = std::make_shared<CosineCostCalculator>();
    FusedCostCalculator calc(0.5f, 0.3f, 9.4877f,
                             NullGateValidator::getInstance(),
                             iou, cosine, 0.5f);

    auto a = makeFeatT(  0.f, 0.f, 4, 1);
    auto b = makeFeatT( 30.f, 0.f, 4, 1);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), calc.calculate_single(a, b), 1e-5f);
}

// ===========================================================================
// ReIDFirstCostCalculator
// ===========================================================================

/**
 * @brief Both tracklets carry identical features; reid_dist = 0 < thresh → direct
 *        assignment path → cost = 0 * reid_direct_assignment_weight = 0.
 */
TEST(ReIDFirstCostCalculatorTest, DirectAssignmentWhenBothHaveFeaturesAndCloseReID)
{
    auto iou    = std::make_shared<IoUCostCalculator>();
    auto cosine = std::make_shared<CosineCostCalculator>();
    ReIDFirstCostCalculator calc(0.3f, 0.3f, 0.2f, iou, cosine);

    auto a = makeFeatT(100.f, 100.f, 4, 0);
    auto b = makeFeatT(100.f, 100.f, 4, 0);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), 0.f, 1e-4f);
}

/**
 * @brief Neither tracklet has a feature vector; the blended path runs with
 *        reid_dist = 1 and IoU cost, producing a cost in [0, 1].
 */
TEST(ReIDFirstCostCalculatorTest, BlendedPathWhenNoFeatures)
{
    auto iou    = std::make_shared<IoUCostCalculator>();
    auto cosine = std::make_shared<CosineCostCalculator>();
    ReIDFirstCostCalculator calc(0.3f, 0.3f, 0.2f, iou, cosine);

    auto a = makeT(100.f, 100.f, 50.f, 80.f);
    auto b = makeT(100.f, 100.f, 50.f, 80.f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_GE(m(0, 0), 0.f);
    EXPECT_LE(m(0, 0), 1.f);
}

/**
 * @brief Tracklet A has a feature but B does not; the blended path applies
 *        with reid_dist = 1 (missing feature penalty), giving cost ≥ 0.
 */
TEST(ReIDFirstCostCalculatorTest, MissingFeatureOnBProducesFallback)
{
    auto iou    = std::make_shared<IoUCostCalculator>();
    auto cosine = std::make_shared<CosineCostCalculator>();
    ReIDFirstCostCalculator calc(0.3f, 0.3f, 0.2f, iou, cosine);

    auto a = makeFeatT(100.f, 100.f, 4, 0);
    auto b = makeT(100.f, 100.f, 50.f, 80.f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_GE(m(0, 0), 0.f);
}

/**
 * @brief An empty detection collection produces an empty cost matrix.
 */
TEST(ReIDFirstCostCalculatorTest, EmptyCollectionProducesEmptyMatrix)
{
    auto iou    = std::make_shared<IoUCostCalculator>();
    auto cosine = std::make_shared<CosineCostCalculator>();
    ReIDFirstCostCalculator calc(0.3f, 0.3f, 0.2f, iou, cosine);

    auto a = makeFeatT(0.f, 0.f, 4, 0);
    std::vector<std::shared_ptr<Tracklet>> av{a}, bv;
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_TRUE(m.empty());
}

/**
 * @brief calculate_single returns the same value as the corresponding batch cell.
 */
TEST(ReIDFirstCostCalculatorTest, SingleMatchesBatch)
{
    auto iou    = std::make_shared<IoUCostCalculator>();
    auto cosine = std::make_shared<CosineCostCalculator>();
    ReIDFirstCostCalculator calc(0.3f, 0.3f, 0.2f, iou, cosine);

    auto a = makeFeatT(  0.f, 0.f, 4, 2);
    auto b = makeFeatT( 30.f, 0.f, 4, 2);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), calc.calculate_single(a, b), 1e-5f);
}

// ===========================================================================
// PureReIDCostCalculator
// ===========================================================================

/**
 * @brief NullGate + identical features → reid_dist = 0 < thresh = 0.4 → cost near 0.
 */
TEST(PureReIDCostCalculatorTest, NullGateIdenticalFeaturesPassesThreshold)
{
    PureReIDCostCalculator calc(0.4f, 9.4877f, NullGateValidator::getInstance());

    auto a = makeFeatT(100.f, 100.f, 4, 0);
    auto b = makeFeatT(100.f, 100.f, 4, 0);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), 0.f, 1e-4f);
}

/**
 * @brief NullGate + orthogonal features → reid_dist = 1 ≥ thresh = 0.4 → sentinel.
 */
TEST(PureReIDCostCalculatorTest, NullGateOrthogonalFeaturesExceedsThreshold)
{
    PureReIDCostCalculator calc(0.4f, 9.4877f, NullGateValidator::getInstance());

    auto a = makeFeatT(100.f, 100.f, 4, 0);
    auto b = makeFeatT(100.f, 100.f, 4, 1);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_GE(m(0, 0), 1e3f);
}

/**
 * @brief A detection 5000 px from an activated tracklet fails the Mahalanobis
 *        gate and receives a sentinel value regardless of feature similarity.
 */
TEST(PureReIDCostCalculatorTest, FailedGateProducesSentinel)
{
    PureReIDCostCalculator calc(0.4f, 9.4877f, std::make_shared<KalmanFilter>());

    auto a = makeFeatT(100.f, 100.f, 4, 0);
    a->activate(std::make_unique<KalmanFilter>(), 1, 1);
    a->predict();

    auto b = makeFeatT(5100.f, 100.f, 4, 0);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_GE(m(0, 0), 1e3f);
}

/**
 * @brief An empty detection collection produces an empty cost matrix.
 */
TEST(PureReIDCostCalculatorTest, EmptyCollectionProducesEmptyMatrix)
{
    PureReIDCostCalculator calc(0.4f, 9.4877f, NullGateValidator::getInstance());

    auto a = makeFeatT(0.f, 0.f, 4, 0);
    std::vector<std::shared_ptr<Tracklet>> av{a}, bv;
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_TRUE(m.empty());
}

/**
 * @brief calculate_single returns the same value as the corresponding batch cell
 *        for a pair that passes the threshold.
 */
TEST(PureReIDCostCalculatorTest, SingleMatchesBatch)
{
    PureReIDCostCalculator calc(0.4f, 9.4877f, NullGateValidator::getInstance());

    auto a = makeFeatT(  0.f, 0.f, 4, 3);
    auto b = makeFeatT( 30.f, 0.f, 4, 3);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), calc.calculate_single(a, b), 1e-5f);
}

/**
 * @brief FuseScoreDecorator with detection score = 0 always produces cost = 1:
 *        fused = 1 - (1 - inner_cost) * 0 = 1, regardless of the inner cost.
 */
TEST(FuseScoreDecoratorTest, ScoreZeroProducesCostOne)
{
    auto inner = std::make_shared<IoUCostCalculator>();
    FuseScoreDecorator dec(inner);

    auto a = makeT(0.f, 0.f, 50.f, 80.f, 0.9f);
    auto b = makeT(0.f, 0.f, 50.f, 80.f, 0.0f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    dec.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), 1.f, 1e-5f);
}

/**
 * @brief DIoU cost for a misaligned contained box is strictly GREATER
 * than the corresponding plain IoU cost because the center-distance 
 * penalty reduces the DIoU score, thus increasing the cost.
 */
TEST(DIoUCostCalculatorTest, ContainedMisalignedBoxCostGreaterThanIoUCost)
{
    IoUCostCalculator  iou_calc;
    DIoUCostCalculator diou_calc;

    auto outer = makeT(0.f, 0.f, 100.f, 100.f);
    auto inner = makeT(10.f, 10.f, 50.f, 50.f);

    std::vector<std::shared_ptr<Tracklet>> av{outer}, bv{inner};
    CostMatrix iou_m, diou_m;

    iou_calc.calculate(av, bv, iou_m);
    diou_calc.calculate(av, bv, diou_m);

    EXPECT_GT(diou_m(0, 0), iou_m(0, 0))
        << "DIoU cost should be higher than IoU cost due to center distance penalty";
}

/**
 * @brief CosineCostCalculator returns cost = 1 when tracklet B carries no
 *        feature vector (missing feature on the detection side).
 */
TEST(CosineCostCalculatorTest, MissingFeatureOnBProducesCostOne)
{
    CosineCostCalculator calc;
    auto a = makeFeatT(0.f, 0.f, 3, 0);
    auto b = makeT(0.f, 0.f, 50.f, 80.f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_FLOAT_EQ(m(0, 0), 1.f);
}

// ===========================================================================
// GIoUCostCalculator
// ===========================================================================

/** @brief Identical boxes — cost ≈ 0. */
TEST(GIoUCostCalculatorTest, IdenticalBoxesCostIsZero)
{
    GIoUCostCalculator calc;
    auto a = makeT(50.f, 50.f, 80.f, 80.f);
    auto b = makeT(50.f, 50.f, 80.f, 80.f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), 0.f, 1e-4f);
}

/** @brief Separated boxes (no overlap) give a cost > 0. */
TEST(GIoUCostCalculatorTest, SeparatedBoxesCostPositive)
{
    GIoUCostCalculator calc;
    auto a = makeT(  0.f, 0.f, 50.f, 50.f);
    auto b = makeT(300.f, 0.f, 50.f, 50.f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_GT(m(0, 0), 0.f);
}

/** @brief calculate_single agrees with batch for a partial overlap pair. */
TEST(GIoUCostCalculatorTest, SingleMatchesBatch)
{
    GIoUCostCalculator calc;
    auto a = makeT(0.f, 0.f, 100.f, 100.f);
    auto b = makeT(40.f, 40.f, 100.f, 100.f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), calc.calculate_single(a, b), 1e-5f);
}

// ===========================================================================
// SIoUCostCalculator
// ===========================================================================

/** @brief Identical boxes — cost ≈ 0. */
TEST(SIoUCostCalculatorTest, IdenticalBoxesCostIsZero)
{
    SIoUCostCalculator calc;
    auto a = makeT(50.f, 50.f, 80.f, 80.f);
    auto b = makeT(50.f, 50.f, 80.f, 80.f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), 0.f, 1e-4f);
}

/** @brief Separated boxes (no overlap) give a cost > 0. */
TEST(SIoUCostCalculatorTest, SeparatedBoxesCostPositive)
{
    SIoUCostCalculator calc;
    auto a = makeT(  0.f, 0.f, 50.f, 50.f);
    auto b = makeT(300.f, 0.f, 50.f, 50.f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_GT(m(0, 0), 0.f);
}

/** @brief calculate_single agrees with batch for a partial overlap pair. */
TEST(SIoUCostCalculatorTest, SingleMatchesBatch)
{
    SIoUCostCalculator calc;
    auto a = makeT(0.f, 0.f, 100.f, 100.f);
    auto b = makeT(40.f, 40.f, 100.f, 100.f);

    std::vector<std::shared_ptr<Tracklet>> av{a}, bv{b};
    CostMatrix m;
    calc.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), calc.calculate_single(a, b), 1e-5f);
}
