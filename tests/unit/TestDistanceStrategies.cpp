/****************************************************************/
/* TestDistanceStrategies.cpp                                   */
/* SushiTrack - https://github.com/SushiSystems/SushiTrack      */
/* Copyright (c) 2026 Sushi Systems                             */
/* Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.    */
/* Commercial use requires a licence from Sushi Systems.        */
/****************************************************************/

/**
 * @file TestDistanceStrategies.cpp
 * @brief Unit tests for distance and association cost metrics.
 * @author Mustafa Garip
 */

#include <gtest/gtest.h>
#include <SushiTrack/distance_strategies.hpp>
#include <SushiTrack/tracklet.hpp>
#include <SushiTrack/kalman_filter.hpp>
#include <SushiTrack/rect.hpp>
#include <SushiTrack/cost_matrix.hpp>

#include <memory>
#include <vector>
#include <cmath>
#include <SushiTrack/i_gate_validator.hpp>

using namespace SushiTrack;

// Helpers

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

// IoUCostCalculator

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

// DIoUCostCalculator

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

// CosineCostCalculator

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

// FuseScoreDecorator

/** @brief A detection score < 1 lowers the effective cost below the raw IoU cost. */
TEST(FuseScoreDecoratorTest, HighScoreLowersCost)
{
    // Inner calculator: IoU on identical boxes → cost = 0. Decorated cost = 1 - (1 - 0) * 0.8 = 0.2.
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

// GatedIoUCostCalculator

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

// FusedCostCalculator

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

// ReIDFirstCostCalculator

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

// PureReIDCostCalculator

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

// GIoUCostCalculator

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

// SIoUCostCalculator

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

// ObservationMomentumDecorator

namespace
{
    /** @brief Inner calculator returning a fixed cost, isolating the momentum term. */
    class ConstCost : public ICostCalculator
    {
        public:
            explicit ConstCost(float c) : c_(c) {}
            void calculate(const std::vector<TrackletPtr>& __restrict a,
                           const std::vector<TrackletPtr>& __restrict b,
                           CostMatrix& __restrict m) override
            {
                m.resize(a.size(), b.size(), c_);
            }
            float calculate_single(const TrackletPtr&, const TrackletPtr&) const override { return c_; }
        private:
            float c_;
    };

    /** @brief Builds a track cruising in +x at the given per-frame step; no IMM, so it reports Cruising. */
    std::shared_ptr<Tracklet> makeCruiser(float start_x, float y, float step, int steps)
    {
        auto t = makeT(start_x, y, 50.f, 80.f);
        t->activate(std::make_unique<KalmanFilter>(), 1, 1);
        float x = start_x;
        for (int f = 2; f <= steps + 1; ++f)
        {
            t->predict(1.0f);
            x += step;
            auto obs = makeT(x, y, 50.f, 80.f);
            t->update(*obs, static_cast<size_t>(f), true, false);
        }
        return t;
    }

    /** @brief Detection whose box centre sits exactly at (cx, cy). */
    std::shared_ptr<Tracklet> detAtCenter(float cx, float cy)
    {
        return makeT(cx - 25.f, cy - 40.f, 50.f, 80.f);
    }

    /** @brief Centre of a track's last raw observation. */
    void lastObsCenter(const std::shared_ptr<Tracklet>& t, float& cx, float& cy)
    {
        const auto& lo = t->getLastObservation();
        cx = lo.x() + lo.width()  * 0.5f;
        cy = lo.y() + lo.height() * 0.5f;
    }
}

/** @brief A detection straight ahead along travel has its cost reduced by the full weight. */
TEST(ObservationMomentumDecoratorTest, AlignedDetectionGetsLowerCost)
{
    auto t = makeCruiser(100.f, 200.f, 10.f, 5);
    float cx, cy; lastObsCenter(t, cx, cy);
    auto ahead = detAtCenter(cx + 100.f, cy);

    ObservationMomentumDecorator dec(std::make_shared<ConstCost>(0.5f), 0.2f, 0.5f, 5);
    std::vector<std::shared_ptr<Tracklet>> av{t}, bv{ahead};
    CostMatrix m;
    dec.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), 0.3f, 1e-3f);
}

/** @brief A detection directly behind travel has its cost raised by the full weight. */
TEST(ObservationMomentumDecoratorTest, OpposingDetectionGetsHigherCost)
{
    auto t = makeCruiser(100.f, 200.f, 10.f, 5);
    float cx, cy; lastObsCenter(t, cx, cy);
    auto behind = detAtCenter(cx - 100.f, cy);

    ObservationMomentumDecorator dec(std::make_shared<ConstCost>(0.5f), 0.2f, 0.5f, 5);
    std::vector<std::shared_ptr<Tracklet>> av{t}, bv{behind};
    CostMatrix m;
    dec.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), 0.7f, 1e-3f);
}

/** @brief A detection perpendicular to travel leaves the cost unchanged (cos = 0). */
TEST(ObservationMomentumDecoratorTest, PerpendicularDetectionUnchanged)
{
    auto t = makeCruiser(100.f, 200.f, 10.f, 5);
    float cx, cy; lastObsCenter(t, cx, cy);
    auto side = detAtCenter(cx, cy + 100.f);

    ObservationMomentumDecorator dec(std::make_shared<ConstCost>(0.5f), 0.2f, 0.5f, 5);
    std::vector<std::shared_ptr<Tracklet>> av{t}, bv{side};
    CostMatrix m;
    dec.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), 0.5f, 1e-3f);
}

/** @brief Sentinel cells from the inner calculator are passed through untouched. */
TEST(ObservationMomentumDecoratorTest, SentinelPairsArePreserved)
{
    auto t = makeCruiser(100.f, 200.f, 10.f, 5);
    float cx, cy; lastObsCenter(t, cx, cy);
    auto ahead = detAtCenter(cx + 100.f, cy);

    ObservationMomentumDecorator dec(std::make_shared<ConstCost>(1e4f), 0.2f, 0.5f, 5);
    std::vector<std::shared_ptr<Tracklet>> av{t}, bv{ahead};
    CostMatrix m;
    dec.calculate(av, bv, m);

    EXPECT_FLOAT_EQ(m(0, 0), 1e4f);
}

/** @brief A track slower than min_velocity contributes no momentum. */
TEST(ObservationMomentumDecoratorTest, SlowTrackIsGatedOff)
{
    auto t = makeCruiser(100.f, 200.f, 0.2f, 5);
    float cx, cy; lastObsCenter(t, cx, cy);
    auto ahead = detAtCenter(cx + 100.f, cy);

    ObservationMomentumDecorator dec(std::make_shared<ConstCost>(0.5f), 0.2f, 0.5f, 5);
    std::vector<std::shared_ptr<Tracklet>> av{t}, bv{ahead};
    CostMatrix m;
    dec.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), 0.5f, 1e-4f);
}

/** @brief A track whose last observation is older than max_age contributes no momentum. */
TEST(ObservationMomentumDecoratorTest, StaleTrackIsGatedOff)
{
    auto t = makeCruiser(100.f, 200.f, 10.f, 5);
    t->predict(1.0f);
    t->predict(1.0f);   // frames-since-observation now 2, beyond max_age = 1

    float cx, cy; lastObsCenter(t, cx, cy);
    auto ahead = detAtCenter(cx + 100.f, cy);

    ObservationMomentumDecorator dec(std::make_shared<ConstCost>(0.5f), 0.2f, 0.5f, 1);
    std::vector<std::shared_ptr<Tracklet>> av{t}, bv{ahead};
    CostMatrix m;
    dec.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), 0.5f, 1e-4f);
}

/** @brief A track with only one observation has no direction and contributes no momentum. */
TEST(ObservationMomentumDecoratorTest, TrackWithoutVelocityIsGatedOff)
{
    auto t = makeT(100.f, 200.f, 50.f, 80.f);
    t->activate(std::make_unique<KalmanFilter>(), 1, 1);

    float cx, cy; lastObsCenter(t, cx, cy);
    auto ahead = detAtCenter(cx + 100.f, cy);

    ObservationMomentumDecorator dec(std::make_shared<ConstCost>(0.5f), 0.2f, 0.5f, 5);
    std::vector<std::shared_ptr<Tracklet>> av{t}, bv{ahead};
    CostMatrix m;
    dec.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), 0.5f, 1e-4f);
}

/** @brief calculate_single agrees with the batch path. */
TEST(ObservationMomentumDecoratorTest, SingleMatchesBatch)
{
    auto t = makeCruiser(100.f, 200.f, 10.f, 5);
    float cx, cy; lastObsCenter(t, cx, cy);
    auto ahead = detAtCenter(cx + 100.f, cy);

    ObservationMomentumDecorator dec(std::make_shared<ConstCost>(0.5f), 0.2f, 0.5f, 5);
    std::vector<std::shared_ptr<Tracklet>> av{t}, bv{ahead};
    CostMatrix m;
    dec.calculate(av, bv, m);

    EXPECT_NEAR(dec.calculate_single(t, ahead), m(0, 0), 1e-5f);
}

/**
 * @brief maneuver_weight_scale only governs Maneuvering tracks; a Cruising track
 *        always receives the full weight, so the scale must not alter its result.
 */
TEST(ObservationMomentumDecoratorTest, ManeuverScaleDoesNotAffectCruisingTrack)
{
    auto t = makeCruiser(100.f, 200.f, 10.f, 5);  // no IMM -> reports Cruising
    float cx, cy; lastObsCenter(t, cx, cy);
    auto ahead = detAtCenter(cx + 100.f, cy);
    std::vector<std::shared_ptr<Tracklet>> av{t}, bv{ahead};

    ObservationMomentumDecorator off(std::make_shared<ConstCost>(0.5f), 0.2f, 0.5f, 5, 0.0f);
    ObservationMomentumDecorator on (std::make_shared<ConstCost>(0.5f), 0.2f, 0.5f, 5, 0.5f);
    CostMatrix mo, mn;
    off.calculate(av, bv, mo);
    on.calculate(av, bv, mn);

    EXPECT_FLOAT_EQ(mo(0, 0), mn(0, 0));
    EXPECT_NEAR(mn(0, 0), 0.3f, 1e-3f);  // full 0.2 bonus, not scaled
}
