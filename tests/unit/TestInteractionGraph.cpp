/**
 * @file      TestInteractionGraph.cpp
 * @brief     Unit tests for BottomYInteractionGraph: build, depth ordering, and queries.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      12.05.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 *
 * Coverage
 * --------
 * - Empty build leaves no occluder for any ID.
 * - contains() returns true for IDs in the last build, false for absent IDs.
 * - getFrontOccluder() returns kNoOccluder for a lone tracklet.
 * - Two overlapping boxes: the one with the higher bottom-Y is identified as
 *   the front (depth-ordering heuristic).
 * - Two non-overlapping boxes: no occluder recorded (below DIoU threshold).
 * - occluderOverlap() returns 0 for an unknown occluder_id.
 * - occluderOverlap() returns > 0 when the candidate overlaps the stored box.
 * - occluderOverlap() returns 0 when the candidate does not overlap.
 * - getKinematicState() returns Cruising for unknown IDs.
 * - getKinematicState() returns the correct state set on the Tracklet.
 * - Build clears data from the previous frame.
 * - Tracklet with ID=0 (tentative) is skipped during build.
 */

#include <gtest/gtest.h>
#include "SushiTrack/InteractionGraph.h"
#include "SushiTrack/Tracklet.h"
#include "SushiTrack/KalmanFilter.h"
#include "SushiTrack/IInteractionGraph.h"

#include <memory>
#include <vector>

using namespace SushiTrack;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/**
 * @brief Creates an activated Tracklet at (x, y, w, h) with the given track_id.
 *
 * @param x        Top-left x.
 * @param y        Top-left y.
 * @param w        Width.
 * @param h        Height.
 * @param track_id Identifier assigned via activate().
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

// ---------------------------------------------------------------------------
// Empty build
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// contains()
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// Single tracklet — no occluder
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// Depth ordering — two overlapping boxes
// ---------------------------------------------------------------------------

/**
 * @brief Of two overlapping boxes, the one with the greater bottom-Y edge is
 *        in front and assigned as the occluder of the one with the smaller
 *        bottom-Y edge.
 *
 * Box A: top-left (0,0) 100×200 → bottom = 200
 * Box B: top-left (0,0) 100×50  → bottom =  50
 * A is lower in the image (closer to camera); B should report A as its occluder.
 * A should have no occluder recorded (A is in front).
 */
TEST(BottomYInteractionGraphTest, LowerBottomIsInFront)
{
    // DIoU threshold = 0 so any overlap registers.
    BottomYInteractionGraph g(0.0f);

    auto big  = makeTrack(0.f, 0.f, 100.f, 200.f, 1);  // bottom = 200
    auto small_ = makeTrack(0.f, 0.f, 100.f,  50.f, 2);  // bottom =  50

    g.build({big, small_});

    // Track 2 (small) should see track 1 (big) as its occluder.
    EXPECT_EQ(g.getFrontOccluder(2), 1);
    // Track 1 (big, in front) has no occluder.
    EXPECT_EQ(g.getFrontOccluder(1), IInteractionGraph::kNoOccluder);
}

// ---------------------------------------------------------------------------
// Non-overlapping boxes — below DIoU threshold
// ---------------------------------------------------------------------------

/**
 * @brief Two non-overlapping boxes with a threshold of 0 produce no occluder
 *        entries because DIoU is negative for non-overlapping boxes.
 */
TEST(BottomYInteractionGraphTest, NonOverlappingBelowThresholdNoOccluder)
{
    // Even with threshold = 0 (register any DIoU >= 0), fully separated
    // boxes have DIoU < 0 and are not registered.
    BottomYInteractionGraph g(0.0f);

    auto a = makeTrack(  0.f, 0.f, 50.f, 50.f, 1);
    auto b = makeTrack(500.f, 0.f, 50.f, 50.f, 2);

    g.build({a, b});

    EXPECT_EQ(g.getFrontOccluder(1), IInteractionGraph::kNoOccluder);
    EXPECT_EQ(g.getFrontOccluder(2), IInteractionGraph::kNoOccluder);
}

// ---------------------------------------------------------------------------
// occluderOverlap()
// ---------------------------------------------------------------------------

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
    auto big   = makeTrack(0.f, 0.f, 100.f, 200.f, 1);
    auto small_ = makeTrack(0.f, 0.f, 100.f,  50.f, 2);
    g.build({big, small_});

    // Query overlap between occluder (id=1, box 0,0,100,200) and an
    // overlapping candidate box.
    float ov = g.occluderOverlap(1, 0.f, 0.f, 80.f, 100.f);
    EXPECT_GT(ov, 0.f);
}

/**
 * @brief occluderOverlap() returns 0 (or negative) when the candidate is
 *        far from the occluder's stored position.
 */
TEST(BottomYInteractionGraphTest, OccluderOverlapZeroOnSeparation)
{
    BottomYInteractionGraph g(0.0f);
    auto big   = makeTrack(0.f, 0.f, 100.f, 200.f, 1);
    auto small_ = makeTrack(0.f, 0.f, 100.f,  50.f, 2);
    g.build({big, small_});

    // Candidate far away from stored occluder.
    float ov = g.occluderOverlap(1, 5000.f, 5000.f, 50.f, 50.f);
    EXPECT_LE(ov, 0.f);
}

// ---------------------------------------------------------------------------
// getKinematicState()
// ---------------------------------------------------------------------------

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
 * @brief getKinematicState() returns the state captured during build for a
 *        known tracklet.
 *
 * The default TrackFSM state after activation is Cruising (or the FSM may
 * not be attached). The Tracklet with no FSM returns Cruising from
 * getLastSeenState(), which is what BottomYInteractionGraph::build captures.
 */
TEST(BottomYInteractionGraphTest, KinematicStateMatchesCapturedAtBuild)
{
    BottomYInteractionGraph g(0.0f);
    auto t = makeTrack(0.f, 0.f, 100.f, 100.f, 5);
    g.build({t});

    // No FSM attached → state stored is Cruising.
    EXPECT_EQ(g.getKinematicState(5), KinematicState::Cruising);
}

// ---------------------------------------------------------------------------
// Build clears previous state
// ---------------------------------------------------------------------------

/**
 * @brief A second build() with a new tracklet collection replaces the previous
 *        graph; IDs from the first build are no longer present.
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

// ---------------------------------------------------------------------------
// Tentative tracklet (ID=0) skipped
// ---------------------------------------------------------------------------

/**
 * @brief A Tracklet whose getTrackId() returns 0 (unconfirmed tentative)
 *        is ignored by build(), so contains(0) returns false.
 */
TEST(BottomYInteractionGraphTest, TentativeZeroIdIsSkipped)
{
    BottomYInteractionGraph g(0.0f);

    // Construct without activating → track_id_ stays 0.
    auto tentative = std::make_shared<Tracklet>(
        std::make_unique<KalmanFilter>(),
        Rect<float>(0.f, 0.f, 50.f, 50.f),
        /*label=*/1, /*score=*/0.9f
    );
    // Confirm track_id == 0.
    ASSERT_EQ(tentative->getTrackId(), 0u);

    g.build({tentative});

    EXPECT_FALSE(g.contains(0));
}
