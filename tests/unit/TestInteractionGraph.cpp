/**
 * @file      TestInteractionGraph.cpp
 * @brief     Unit tests for BottomYInteractionGraph: build, depth ordering, queries, and threshold sensitivity.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      12.05.2026
 * @version   v1.1.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 *
 * Coverage
 * --------
 * - Empty build leaves no occluder for any ID.
 * - contains() returns true for IDs in the last build, false for absent IDs.
 * - getFrontOccluder() returns kNoOccluder for a lone tracklet.
 * - Two overlapping boxes: the one with the higher bottom-Y is the front occluder.
 * - Two non-overlapping boxes: no occluder recorded (DIoU below threshold).
 * - occluderOverlap() returns 0 for an unknown occluder_id.
 * - occluderOverlap() returns > 0 when the candidate overlaps the stored box.
 * - occluderOverlap() returns <= 0 when the candidate is far from the stored box.
 * - occluderOverlap() returns > 0 when the candidate is fully contained in the stored box.
 * - getKinematicState() returns Cruising for unknown IDs.
 * - getKinematicState() returns the correct state captured during build.
 * - Build clears data from the previous frame.
 * - Tracklet with ID=0 (unactivated tentative) is skipped during build.
 * - Three overlapping boxes: the frontmost (highest bottom-Y) occludes both others.
 * - At threshold=0.0, a partially overlapping pair (DIoU ~ 0.45) produces an occluder link.
 * - At threshold=0.5, a barely-touching pair (DIoU ~ 0.01) produces no occluder link.
 * - Mixed build with active and tentative tracklets: only active IDs are present.
 * - Rebuild with a relocated tracklet updates the occluder relationship.
 */

#include <gtest/gtest.h>
#include "SushiTrack/InteractionGraph.h"
#include "SushiTrack/Tracklet.h"
#include "SushiTrack/KalmanFilter.h"
#include "SushiTrack/IInteractionGraph.h"

#include <memory>
#include <vector>

using namespace SushiTrack;

/**
 * @brief Creates an activated Tracklet at (x, y, w, h) with the given track_id.
 *
 * @param x        Top-left x coordinate.
 * @param y        Top-left y coordinate.
 * @param w        Width.
 * @param h        Height.
 * @param track_id Identifier assigned via activate().
 * @return Shared pointer to the activated Tracklet.
 */
static std::shared_ptr<Tracklet> makeTrack(float x, float y, float w, float h,
                                           size_t track_id)
{
    auto t = std::make_shared<Tracklet>(
        std::make_unique<KalmanFilter>(),
        Rect<float>(x, y, w, h),
        /*label=*/1, /*score=*/0.9f
    );
    t->activate(std::make_unique<KalmanFilter>(), /*frame_id=*/1, track_id);
    return t;
}

/**
 * @brief After build() on an empty collection, getFrontOccluder returns kNoOccluder.
 */
TEST(BottomYInteractionGraphTest, EmptyBuildNoOccluder)
{
    BottomYInteractionGraph g(0.0f);
    g.build({});

    EXPECT_EQ(g.getFrontOccluder(1), IInteractionGraph::kNoOccluder);
    EXPECT_FALSE(g.contains(1));
}

/**
 * @brief contains() returns true for a tracklet present in the last build.
 */
TEST(BottomYInteractionGraphTest, ContainsTrueForBuiltId)
{
    BottomYInteractionGraph g(0.0f);
    auto t = makeTrack(0.f, 0.f, 50.f, 50.f, 10);
    g.build({t});
    EXPECT_TRUE(g.contains(10));
}

/**
 * @brief contains() returns false for an ID not in the last build.
 */
TEST(BottomYInteractionGraphTest, ContainsFalseForAbsentId)
{
    BottomYInteractionGraph g(0.0f);
    auto t = makeTrack(0.f, 0.f, 50.f, 50.f, 10);
    g.build({t});
    EXPECT_FALSE(g.contains(99));
}

/**
 * @brief A lone tracklet has no neighbour to compare against; kNoOccluder returned.
 */
TEST(BottomYInteractionGraphTest, SingleTrackletNoOccluder)
{
    BottomYInteractionGraph g(0.0f);
    auto t = makeTrack(100.f, 100.f, 80.f, 80.f, 1);
    g.build({t});
    EXPECT_EQ(g.getFrontOccluder(1), IInteractionGraph::kNoOccluder);
}

/**
 * @brief Of two overlapping boxes, the one with the greater bottom-Y edge is
 *        in front and assigned as the occluder of the one with the smaller bottom-Y.
 *
 * Box A: (0,0,100,200) bottom=200 — front occluder.
 * Box B: (0,0,100,50)  bottom=50  — occluded by A.
 */
TEST(BottomYInteractionGraphTest, LowerBottomIsInFront)
{
    BottomYInteractionGraph g(0.0f);

    auto big    = makeTrack(0.f, 0.f, 100.f, 200.f, 1);
    auto small_ = makeTrack(0.f, 0.f, 100.f,  50.f, 2);

    g.build({big, small_});

    EXPECT_EQ(g.getFrontOccluder(2), 1u);
    EXPECT_EQ(g.getFrontOccluder(1), IInteractionGraph::kNoOccluder);
}

/**
 * @brief Two non-overlapping boxes with a threshold of 0 produce no occluder
 *        entries because DIoU is negative for fully separated boxes.
 */
TEST(BottomYInteractionGraphTest, NonOverlappingBelowThresholdNoOccluder)
{
    BottomYInteractionGraph g(0.0f);

    auto a = makeTrack(  0.f, 0.f, 50.f, 50.f, 1);
    auto b = makeTrack(500.f, 0.f, 50.f, 50.f, 2);

    g.build({a, b});

    EXPECT_EQ(g.getFrontOccluder(1), IInteractionGraph::kNoOccluder);
    EXPECT_EQ(g.getFrontOccluder(2), IInteractionGraph::kNoOccluder);
}

/**
 * @brief occluderOverlap() returns 0 for an occluder_id not in the graph.
 */
TEST(BottomYInteractionGraphTest, OccluderOverlapUnknownIdReturnsZero)
{
    BottomYInteractionGraph g(0.0f);
    g.build({});
    EXPECT_FLOAT_EQ(g.occluderOverlap(99, 0.f, 0.f, 50.f, 50.f), 0.f);
}

/**
 * @brief occluderOverlap() returns > 0 when the candidate box overlaps the
 *        stored occluder box.
 */
TEST(BottomYInteractionGraphTest, OccluderOverlapPositiveOnOverlap)
{
    BottomYInteractionGraph g(0.0f);
    auto big    = makeTrack(0.f, 0.f, 100.f, 200.f, 1);
    auto small_ = makeTrack(0.f, 0.f, 100.f,  50.f, 2);
    g.build({big, small_});

    float ov = g.occluderOverlap(1, 0.f, 0.f, 80.f, 100.f);
    EXPECT_GT(ov, 0.f);
}

/**
 * @brief occluderOverlap() returns <= 0 when the candidate is far from the
 *        occluder's stored position.
 */
TEST(BottomYInteractionGraphTest, OccluderOverlapZeroOnSeparation)
{
    BottomYInteractionGraph g(0.0f);
    auto big    = makeTrack(0.f, 0.f, 100.f, 200.f, 1);
    auto small_ = makeTrack(0.f, 0.f, 100.f,  50.f, 2);
    g.build({big, small_});

    float ov = g.occluderOverlap(1, 5000.f, 5000.f, 50.f, 50.f);
    EXPECT_LE(ov, 0.f);
}

/**
 * @brief getKinematicState() returns Cruising for an ID not in the graph.
 */
TEST(BottomYInteractionGraphTest, KinematicStateUnknownIdReturnsCruising)
{
    BottomYInteractionGraph g(0.0f);
    g.build({});
    EXPECT_EQ(g.getKinematicState(42), KinematicState::Cruising);
}

/**
 * @brief getKinematicState() returns the state captured during build for a known tracklet.
 *
 * A Tracklet with no FSM attached returns Cruising from getKinematicState(), which is
 * what BottomYInteractionGraph::build captures.
 */
TEST(BottomYInteractionGraphTest, KinematicStateMatchesCapturedAtBuild)
{
    BottomYInteractionGraph g(0.0f);
    auto t = makeTrack(0.f, 0.f, 100.f, 100.f, 5);
    g.build({t});
    EXPECT_EQ(g.getKinematicState(5), KinematicState::Cruising);
}

/**
 * @brief A second build() with a new tracklet collection replaces the previous graph;
 *        IDs from the first build are no longer present.
 */
TEST(BottomYInteractionGraphTest, BuildClearsPreviousState)
{
    BottomYInteractionGraph g(0.0f);

    auto old_t = makeTrack(0.f, 0.f, 50.f, 50.f, 10);
    g.build({old_t});
    ASSERT_TRUE(g.contains(10));

    auto new_t = makeTrack(0.f, 0.f, 50.f, 50.f, 20);
    g.build({new_t});

    EXPECT_FALSE(g.contains(10));
    EXPECT_TRUE(g.contains(20));
}

/**
 * @brief A Tracklet whose getTrackId() returns 0 (unconfirmed tentative) is ignored
 *        by build(), so contains(0) returns false.
 */
TEST(BottomYInteractionGraphTest, TentativeZeroIdIsSkipped)
{
    BottomYInteractionGraph g(0.0f);

    auto tentative = std::make_shared<Tracklet>(
        std::make_unique<KalmanFilter>(),
        Rect<float>(0.f, 0.f, 50.f, 50.f),
        /*label=*/1, /*score=*/0.9f
    );
    ASSERT_EQ(tentative->getTrackId(), 0u);

    g.build({tentative});

    EXPECT_FALSE(g.contains(0));
}

/**
 * @brief With three boxes all starting at the same origin, the box with the greatest
 *        bottom-Y edge (h=300) is the front occluder for both the middle (h=150) and
 *        back (h=50) boxes.  The frontmost box itself has no occluder.
 *
 * Track 1: (0,0,100,300) bottom=300 — frontmost.
 * Track 2: (0,0,100,150) bottom=150 — middle.
 * Track 3: (0,0,100,50)  bottom=50  — backmost.
 */
TEST(BottomYInteractionGraphTest, ThreeOverlappingTracklets_FrontOccludesMiddleAndBack)
{
    BottomYInteractionGraph g(0.0f);

    auto front  = makeTrack(0.f, 0.f, 100.f, 300.f, 1);
    auto middle = makeTrack(0.f, 0.f, 100.f, 150.f, 2);
    auto back   = makeTrack(0.f, 0.f, 100.f,  50.f, 3);

    g.build({front, middle, back});

    EXPECT_EQ(g.getFrontOccluder(3), 2u);
    EXPECT_EQ(g.getFrontOccluder(2), 1u);
    EXPECT_EQ(g.getFrontOccluder(1), IInteractionGraph::kNoOccluder);
}

/**
 * @brief At threshold=0.0, a contained box pair (DIoU ~ 0.45 > 0) produces an
 *        occluder link from the taller box to the shorter box.
 *
 * Box A: (0,0,100,200) bottom=200 — front occluder.
 * Box B: (0,0,100,100) bottom=100 — occluded by A.
 */
TEST(BottomYInteractionGraphTest, LowThresholdAllowsPartialOverlapLink)
{
    BottomYInteractionGraph g(0.0f);

    auto a = makeTrack(0.f, 0.f, 100.f, 200.f, 1);
    auto b = makeTrack(0.f, 0.f, 100.f, 100.f, 2);

    g.build({a, b});

    EXPECT_EQ(g.getFrontOccluder(2), 1u);
}

/**
 * @brief At threshold=0.5, a barely-touching box pair (DIoU ~ 0.01) produces no
 *        occluder link for either tracklet.
 *
 * Box A: (0,0,50,50) and Box B: (49,0,50,50) share a 1-pixel-wide strip.
 * Intersection = 1×50 = 50, union = 50×50 + 50×50 − 50 = 4950.  IoU ≈ 0.01;
 * DIoU is lower still after the centroid-distance penalty.
 */
TEST(BottomYInteractionGraphTest, HighThresholdSuppressesLowOverlapPair)
{
    BottomYInteractionGraph g(0.5f);

    auto a = makeTrack( 0.f, 0.f, 50.f, 50.f, 1);
    auto b = makeTrack(49.f, 0.f, 50.f, 50.f, 2);

    g.build({a, b});

    EXPECT_EQ(g.getFrontOccluder(1), IInteractionGraph::kNoOccluder);
    EXPECT_EQ(g.getFrontOccluder(2), IInteractionGraph::kNoOccluder);
}

/**
 * @brief occluderOverlap() returns a positive value when the candidate box is
 *        fully contained inside the stored occluder box.
 *
 * Occluder for track 1 is (0,0,100,200).  Candidate (10,10,80,80) lies entirely
 * within that region; the resulting DIoU must be positive.
 */
TEST(BottomYInteractionGraphTest, OccluderOverlapReturnsPositiveForContainedBox)
{
    BottomYInteractionGraph g(0.0f);

    auto front  = makeTrack(0.f, 0.f, 100.f, 200.f, 1);
    auto behind = makeTrack(0.f, 0.f, 100.f,  50.f, 2);

    g.build({front, behind});

    float ov = g.occluderOverlap(1, 10.f, 10.f, 80.f, 80.f);
    EXPECT_GT(ov, 0.f);
}

/**
 * @brief When a build includes both activated tracklets and a non-activated
 *        (tentative, ID=0) tracklet, contains() reflects only the activated ones.
 *
 * Track A: activated, id=5 — present.
 * Track B: not activated, id=0 — absent.
 * Track C: activated, id=7 — present.
 */
TEST(BottomYInteractionGraphTest, BuildWithMixedActiveAndTentativeIds)
{
    BottomYInteractionGraph g(0.0f);

    auto a = makeTrack(  0.f, 0.f, 50.f, 50.f, 5);
    auto b = std::make_shared<Tracklet>(
        std::make_unique<KalmanFilter>(),
        Rect<float>(100.f, 0.f, 50.f, 50.f),
        /*label=*/1, /*score=*/0.9f
    );
    auto c = makeTrack(200.f, 0.f, 50.f, 50.f, 7);

    ASSERT_EQ(b->getTrackId(), 0u);

    g.build({a, b, c});

    EXPECT_TRUE(g.contains(5));
    EXPECT_FALSE(g.contains(0));
    EXPECT_TRUE(g.contains(7));
}

/**
 * @brief After a second build in which the previously front tracklet has moved far
 *        from the back tracklet, getFrontOccluder() for the back tracklet returns
 *        kNoOccluder.  The relocated front tracklet is still present in the graph.
 *
 * Frame 1: A(0,0,100,200) overlaps B(0,0,100,50) — B's occluder is A.
 * Frame 2: A moves to (5000,0,100,200), far from B — B's occluder becomes kNoOccluder.
 */
TEST(BottomYInteractionGraphTest, RebuildUpdatesOccluderRelationships)
{
    BottomYInteractionGraph g(0.0f);

    auto a1 = makeTrack(   0.f, 0.f, 100.f, 200.f, 1);
    auto b  = makeTrack(   0.f, 0.f, 100.f,  50.f, 2);

    g.build({a1, b});
    ASSERT_EQ(g.getFrontOccluder(2), 1u);

    auto a2 = makeTrack(5000.f, 0.f, 100.f, 200.f, 1);

    g.build({a2, b});

    EXPECT_EQ(g.getFrontOccluder(2), IInteractionGraph::kNoOccluder);
    EXPECT_TRUE(g.contains(1));
}
