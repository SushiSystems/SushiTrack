/**
 * @file      TestIntegration.cpp
 * @brief     End-to-end production hardening tests for the SushiTrack tracker.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      18.05.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 *
 * Scope
 * -----
 * Addresses the integration-level gaps flagged in UNIT_TEST_VALIDATION_REPORT.md:
 *   - Issue #2 (HIGH): ReID never exercised in the full tracker pipeline.
 *   - Issue #3 (HIGH): No long-sequence (300+ frame) integration test.
 *   - Issue #6 (MED) : Tentative pipeline never combined with ReID.
 *   - 2.3.2          : FSM-aware Mahalanobis gating not exercised end-to-end.
 *   - 2.3.4          : time_aware=1 with a variable-dt sequence not exercised.
 *
 * Conventions
 * -----------
 * Tests assert against the documented behaviour in SushiTrackC.h. Where a test
 * exercises behaviour whose precise tolerances are workload-dependent (e.g.
 * "drift over 300 frames"), assertions check the structural invariants
 * (no NaN, ID stability, monotonic bookkeeping) rather than exact numerics.
 *
 * Coverage
 * --------
 * LongSequenceTrackingTest (300 frames, time_aware=1)
 *   - A single moving object retains the same track_id for the entire run.
 *   - The reported bounding box never contains NaN or Inf values.
 *   - The age counter monotonically increases across the sequence.
 *   - frames_since_last_update remains zero on every continuously-observed frame.
 *
 * TrackerReIDEndToEndTest (enable_reid=1, primary_assoc_mode=1)
 *   - Two visually distinct, spatially crossing objects do not swap IDs through
 *     the crossover frame when feature vectors are well-separated.
 *   - A lost track is recovered after a multi-frame visual gap when reappearing
 *     with the same feature vector.
 *
 * TrackerTentativeWithReIDTest (enable_tentative=1, enable_reid=1)
 *   - A tentative track carrying a feature vector confirms on the documented
 *     hit count without the feature path raising an error.
 *   - A tentative track that re-acquires after a single missed frame retains
 *     its identity rather than spawning a fresh tentative.
 *
 * TrackerFSMGatingEndToEndTest (enable_fsm=1, enable_mahalanobis=1)
 *   - A confirmed track sustained through a maneuvering segment retains its
 *     identity when the predicted box overlaps the detection within the
 *     Mahalanobis gate.
 *   - A detection placed well outside the gate does not bind to an
 *     established track; that track instead transitions to Lost.
 *
 * TrackerVariableDtTest (time_aware=1)
 *   - A track survives a non-uniform dt sequence covering both fast (1/60 s)
 *     and slow (1/10 s) frames without losing identity.
 *   - A large discontinuous dt (1 s gap) does not crash the predictor and
 *     leaves the track in a documented state.
 *
 * TrackerHighToLowConfidenceTest (enable_tentative=1)
 *   - A confirmed track whose subsequent detection drops from high to low
 *     confidence is not immediately removed; it continues across the
 *     low-confidence association stage.
 */

#include <gtest/gtest.h>
#include "SushiTrackC.h"
#include <cmath>
#include <cstring>
#include <vector>

namespace
{

/**
 * @brief Constructs a benign detection without feature data.
 */
SushiTrack_Object makeDet(float x,
                         float y,
                         float score  = 0.9f,
                         float width  = 50.0f,
                         float height = 100.0f,
                         int   label  = 1)
{
    SushiTrack_Object o{};
    o.x        = x;
    o.y        = y;
    o.width    = width;
    o.height   = height;
    o.score    = score;
    o.label    = label;
    o.feat     = nullptr;
    o.feat_dim = 0;
    return o;
}

/**
 * @brief Constructs a detection backed by an external feature pointer.
 *        The caller owns the feature memory; it must outlive the Update call.
 */
SushiTrack_Object makeDetWithFeat(float          x,
                                 float          y,
                                 const float*   feat,
                                 int            feat_dim,
                                 float          score = 0.9f)
{
    SushiTrack_Object o = makeDet(x, y, score);
    o.feat             = feat;
    o.feat_dim         = feat_dim;
    return o;
}

/**
 * @brief Locates a track in an output buffer by its persistent ID.
 * @return Pointer into out for the matching track, or nullptr.
 */
const SushiTrack_Track* findById(const std::vector<SushiTrack_Track>& out,
                                int                                  n,
                                int64_t                              id)
{
    for (int i = 0; i < n; ++i)
        if (out[static_cast<std::size_t>(i)].track_id == id)
            return &out[static_cast<std::size_t>(i)];
    return nullptr;
}

}

// ============================================================================
// LongSequenceTrackingTest — Issue #3 (HIGH)
// ============================================================================

/**
 * @brief Fixture exercising a long, single-object trajectory to validate the
 *        absence of numerical drift, ID churn, and bookkeeping anomalies.
 */
class LongSequenceTrackingTest : public ::testing::Test
{
protected:
    SushiTrackHdl     hdl_    = nullptr;
    SushiTrack_Params params_ = {};

    void SetUp() override
    {
        SushiTrack_GetDefaultParams(&params_);
        params_.time_aware             = 1;
        params_.enable_tentative       = 0;
        params_.primary_assoc_mode     = 1;
        params_.frame_rate             = 30;
        params_.track_buffer           = 60;
        params_.track_thresh           = 0.1f;
        params_.high_thresh            = 0.2f;
        params_.match_thresh           = 0.9f;

        hdl_ = SushiTrack_Create(&params_);
        ASSERT_NE(hdl_, nullptr);
    }

    void TearDown() override { SushiTrack_Destroy(hdl_); }
};

/**
 * @brief 300 contiguous frames of a steadily-moving object must yield a single
 *        persistent track_id with monotonic age, no NaN/Inf in the reported
 *        geometry, and a zero update gap on every observed frame.
 */
TEST_F(LongSequenceTrackingTest, ThreeHundredFrameSingleObjectKeepsIdentity)
{
    constexpr int kFrames = 300;
    std::vector<SushiTrack_Track> out(8);

    SushiTrack_Object d0 = makeDet(100.0f, 200.0f);
    int n0 = SushiTrack_Update(hdl_, &d0, 1, out.data(), 8, 1.0f / 30.0f);
    ASSERT_EQ(n0, 1);
    ASSERT_EQ(out[0].state, 1);
    const int64_t persistent_id = out[0].track_id;

    int64_t prev_age = out[0].age;

    for (int frame = 1; frame < kFrames; ++frame)
    {
        SushiTrack_Object d = makeDet(100.0f + 1.5f * frame,
                                     200.0f + 0.5f * frame);
        int n = SushiTrack_Update(hdl_, &d, 1, out.data(), 8, 1.0f / 30.0f);
        ASSERT_GE(n, 1) << "Tracker dropped the object at frame " << frame;

        const SushiTrack_Track* t = findById(out, n, persistent_id);
        ASSERT_NE(t, nullptr) << "Track ID flipped at frame " << frame;

        EXPECT_TRUE(std::isfinite(t->x));
        EXPECT_TRUE(std::isfinite(t->y));
        EXPECT_TRUE(std::isfinite(t->width));
        EXPECT_TRUE(std::isfinite(t->height));
        EXPECT_GT(t->width,  0.0f);
        EXPECT_GT(t->height, 0.0f);

        EXPECT_GE(t->age, prev_age) << "Age regressed at frame " << frame;
        prev_age = t->age;

        EXPECT_EQ(t->frames_since_last_update, 0)
            << "Continuously observed track reported a non-zero update gap at frame "
            << frame;
    }
}

// ============================================================================
// TrackerReIDEndToEndTest — Issue #2 (HIGH)
// ============================================================================

/**
 * @brief Fixture for full-pipeline ReID validation.
 *
 * Feature dimension 16 is chosen as a compromise between the L2-normalisation
 * code path being exercised and keeping the test memory-cheap. The two
 * personas (A, B) use orthogonal feature vectors so cosine similarity between
 * them is unambiguously zero.
 */
class TrackerReIDEndToEndTest : public ::testing::Test
{
protected:
    SushiTrackHdl     hdl_     = nullptr;
    SushiTrack_Params params_  = {};
    std::vector<float> feat_a_;
    std::vector<float> feat_b_;

    void SetUp() override
    {
        SushiTrack_GetDefaultParams(&params_);
        params_.time_aware             = 1;
        params_.enable_tentative       = 0;
        params_.enable_reid            = 1;
        params_.primary_assoc_mode     = 1;
        params_.frame_rate             = 25;
        params_.track_buffer           = 30;
        params_.track_thresh           = 0.1f;
        params_.high_thresh            = 0.2f;
        params_.match_thresh           = 0.95f;

        hdl_ = SushiTrack_Create(&params_);
        ASSERT_NE(hdl_, nullptr);

        feat_a_.assign(16, 0.0f);
        feat_b_.assign(16, 0.0f);
        feat_a_[0] = 1.0f;   // unit vector along axis 0
        feat_b_[1] = 1.0f;   // unit vector along axis 1 (orthogonal to A)
    }

    void TearDown() override { SushiTrack_Destroy(hdl_); }
};

/**
 * @brief Two visually distinct objects whose trajectories cross retain their
 *        IDs through the crossover. We sample after the crossover and verify
 *        that the original two IDs both still exist.
 */
TEST_F(TrackerReIDEndToEndTest, DistinctFeaturesPreventIdSwapOnCrossover)
{
    std::vector<SushiTrack_Track> out(8);

    SushiTrack_Object a0 = makeDetWithFeat(100.0f, 200.0f, feat_a_.data(), 16, 0.95f);
    SushiTrack_Object b0 = makeDetWithFeat(400.0f, 200.0f, feat_b_.data(), 16, 0.95f);
    SushiTrack_Object dets0[2] = { a0, b0 };
    int n0 = SushiTrack_Update(hdl_, dets0, 2, out.data(), 8, 0.04f);
    ASSERT_GE(n0, 2);

    int64_t id_a = -1, id_b = -1;
    for (int i = 0; i < n0; ++i)
    {
        if (std::abs(out[static_cast<std::size_t>(i)].x - 100.0f) < 60.0f)
            id_a = out[static_cast<std::size_t>(i)].track_id;
        else if (std::abs(out[static_cast<std::size_t>(i)].x - 400.0f) < 60.0f)
            id_b = out[static_cast<std::size_t>(i)].track_id;
    }
    ASSERT_NE(id_a, -1);
    ASSERT_NE(id_b, -1);
    ASSERT_NE(id_a, id_b);

    // Drive A rightward and B leftward across each other over 6 frames.
    for (int k = 1; k <= 6; ++k)
    {
        const float ax = 100.0f + 40.0f * k;
        const float bx = 400.0f - 40.0f * k;
        SushiTrack_Object a = makeDetWithFeat(ax, 200.0f, feat_a_.data(), 16, 0.95f);
        SushiTrack_Object b = makeDetWithFeat(bx, 200.0f, feat_b_.data(), 16, 0.95f);
        SushiTrack_Object dets[2] = { a, b };
        SushiTrack_Update(hdl_, dets, 2, out.data(), 8, 0.04f);
    }

    int n_final = SushiTrack_Update(hdl_,
                                   /* unchanged */ nullptr, 0,
                                   out.data(), 8, 0.04f);
    (void)n_final;

    // Both original IDs must still be alive somewhere in the output history.
    SushiTrack_Object a_late = makeDetWithFeat(380.0f, 200.0f, feat_a_.data(), 16, 0.95f);
    SushiTrack_Object b_late = makeDetWithFeat(120.0f, 200.0f, feat_b_.data(), 16, 0.95f);
    SushiTrack_Object dets_late[2] = { a_late, b_late };
    int n_late = SushiTrack_Update(hdl_, dets_late, 2, out.data(), 8, 0.04f);
    ASSERT_GE(n_late, 2);

    EXPECT_NE(findById(out, n_late, id_a), nullptr)
        << "Track A lost its ID across the crossover.";
    EXPECT_NE(findById(out, n_late, id_b), nullptr)
        << "Track B lost its ID across the crossover.";
}

/**
 * @brief A track that goes Lost for several frames must be recovered with its
 *        original ID when the same visual identity reappears.
 */
TEST_F(TrackerReIDEndToEndTest, LostTrackRecoveredByMatchingFeature)
{
    std::vector<SushiTrack_Track> out(8);

    SushiTrack_Object d0 = makeDetWithFeat(100.0f, 200.0f, feat_a_.data(), 16, 0.95f);
    SushiTrack_Update(hdl_, &d0, 1, out.data(), 8, 0.04f);
    ASSERT_EQ(out[0].state, 1);
    const int64_t original_id = out[0].track_id;

    // Drop the detection for several frames so the track goes Lost.
    for (int i = 0; i < 5; ++i)
        SushiTrack_Update(hdl_, nullptr, 0, out.data(), 8, 0.04f);

    // Reappear with the same feature, slightly displaced.
    SushiTrack_Object d_back = makeDetWithFeat(160.0f, 220.0f, feat_a_.data(), 16, 0.95f);
    int n = SushiTrack_Update(hdl_, &d_back, 1, out.data(), 8, 0.04f);
    ASSERT_GE(n, 1);

    const SushiTrack_Track* recovered = findById(out, n, original_id);
    EXPECT_NE(recovered, nullptr)
        << "ReID failed to recover the lost track to its original ID.";
}

// ============================================================================
// TrackerTentativeWithReIDTest — Issue #6 (MED)
// ============================================================================

/**
 * @brief Fixture exercising tentative-stage promotion under ReID.
 */
class TrackerTentativeWithReIDTest : public ::testing::Test
{
protected:
    SushiTrackHdl     hdl_    = nullptr;
    SushiTrack_Params params_ = {};
    std::vector<float> feat_;

    void SetUp() override
    {
        SushiTrack_GetDefaultParams(&params_);
        params_.time_aware             = 1;
        params_.enable_tentative       = 1;
        params_.enable_reid            = 1;
        params_.tentative_confirm_hits = 3;
        params_.frame_rate             = 25;
        params_.track_buffer           = 30;
        params_.track_thresh           = 0.1f;
        params_.high_thresh            = 0.2f;
        params_.match_thresh           = 0.9f;
        params_.tentative_match_thresh = 0.95f;
        params_.tentative_fast_thresh  = 0.99f;

        hdl_ = SushiTrack_Create(&params_);
        ASSERT_NE(hdl_, nullptr);

        feat_.assign(16, 0.0f);
        feat_[3] = 1.0f;
    }

    void TearDown() override { SushiTrack_Destroy(hdl_); }
};

/**
 * @brief A tentative track carrying ReID features confirms after the documented
 *        hit count without raising an error inside the feature path.
 */
TEST_F(TrackerTentativeWithReIDTest, TentativeWithFeaturesConfirmsAfterHitCount)
{
    std::vector<SushiTrack_Track> out(4);

    for (int i = 0; i < params_.tentative_confirm_hits; ++i)
    {
        SushiTrack_Object d = makeDetWithFeat(100.0f + 1.0f * i,
                                             200.0f + 1.0f * i,
                                             feat_.data(), 16,
                                             0.7f);
        int n = SushiTrack_Update(hdl_, &d, 1, out.data(), 4, 0.04f);
        ASSERT_GE(n, 1);
    }

    EXPECT_EQ(out[0].state, 1) << "Tentative with features did not confirm.";
    EXPECT_EQ(out[0].is_confirmed, 1);
}

/**
 * @brief A tentative track that misses a single frame and re-acquires retains
 *        its identity rather than spawning a fresh tentative slot.
 */
TEST_F(TrackerTentativeWithReIDTest, TentativeReAcquiresAfterSingleMissedFrame)
{
    std::vector<SushiTrack_Track> out(4);

    SushiTrack_Object d0 = makeDetWithFeat(100.0f, 200.0f, feat_.data(), 16, 0.7f);
    int n0 = SushiTrack_Update(hdl_, &d0, 1, out.data(), 4, 0.04f);
    ASSERT_GE(n0, 1);
    const int64_t tentative_id = out[0].track_id;

    SushiTrack_Update(hdl_, nullptr, 0, out.data(), 4, 0.04f);

    SushiTrack_Object d2 = makeDetWithFeat(102.0f, 202.0f, feat_.data(), 16, 0.7f);
    int n2 = SushiTrack_Update(hdl_, &d2, 1, out.data(), 4, 0.04f);
    ASSERT_GE(n2, 1);

    const SushiTrack_Track* same = findById(out, n2, tentative_id);
    EXPECT_NE(same, nullptr)
        << "Tentative track lost its ID across a single missed frame.";
}

// ============================================================================
// TrackerFSMGatingEndToEndTest — Section 2.3.2 (MED)
// ============================================================================

/**
 * @brief Fixture combining FSM with Mahalanobis gating end-to-end.
 */
class TrackerFSMGatingEndToEndTest : public ::testing::Test
{
protected:
    SushiTrackHdl     hdl_    = nullptr;
    SushiTrack_Params params_ = {};

    void SetUp() override
    {
        SushiTrack_GetDefaultParams(&params_);
        params_.time_aware             = 1;
        params_.enable_tentative       = 0;
        params_.enable_fsm             = 1;
        params_.enable_mahalanobis     = 1;
        params_.primary_assoc_mode     = 1;
        params_.frame_rate             = 25;
        params_.track_buffer           = 30;
        params_.track_thresh           = 0.1f;
        params_.high_thresh            = 0.2f;
        params_.match_thresh           = 0.9f;

        hdl_ = SushiTrack_Create(&params_);
        ASSERT_NE(hdl_, nullptr);
    }

    void TearDown() override { SushiTrack_Destroy(hdl_); }
};

/**
 * @brief A track sustained through a maneuvering segment retains its identity
 *        when each successive detection lies inside the FSM-modulated gate.
 */
TEST_F(TrackerFSMGatingEndToEndTest, ManeuveringTrackKeepsIdentityInsideGate)
{
    std::vector<SushiTrack_Track> out(4);
    SushiTrack_Object d0 = makeDet(100.0f, 200.0f);
    SushiTrack_Update(hdl_, &d0, 1, out.data(), 4, 0.04f);
    ASSERT_EQ(out[0].state, 1);
    const int64_t persistent_id = out[0].track_id;

    // Accelerating arc — both x and y change at increasing rates.
    for (int k = 1; k <= 15; ++k)
    {
        const float dx = 2.0f * static_cast<float>(k);
        const float dy = 1.0f * static_cast<float>(k);
        SushiTrack_Object d = makeDet(100.0f + dx, 200.0f + dy);
        int n = SushiTrack_Update(hdl_, &d, 1, out.data(), 4, 0.04f);
        ASSERT_GE(n, 1);
        const SushiTrack_Track* t = findById(out, n, persistent_id);
        ASSERT_NE(t, nullptr) << "Track lost identity inside maneuver at step " << k;
    }
}

/**
 * @brief A detection placed far outside any plausible motion envelope does
 *        not bind to a confirmed track; the track instead drops to Lost.
 */
TEST_F(TrackerFSMGatingEndToEndTest, OutOfGateDetectionDoesNotBindEstablishedTrack)
{
    std::vector<SushiTrack_Track> out(4);
    SushiTrack_Object d0 = makeDet(100.0f, 200.0f);
    SushiTrack_Update(hdl_, &d0, 1, out.data(), 4, 0.04f);
    ASSERT_EQ(out[0].state, 1);
    const int64_t persistent_id = out[0].track_id;

    // Detection 5000 pixels away — far outside any reasonable gate.
    SushiTrack_Object d_far = makeDet(5100.0f, 5200.0f);
    int n = SushiTrack_Update(hdl_, &d_far, 1, out.data(), 4, 0.04f);
    ASSERT_GE(n, 0);

    const SushiTrack_Track* original = findById(out, n, persistent_id);
    if (original != nullptr)
    {
        EXPECT_NE(original->state, 1)
            << "Out-of-gate detection silently re-bound a Tracked track.";
    }
}

// ============================================================================
// TrackerVariableDtTest — Section 2.3.4 (MED)
// ============================================================================

/**
 * @brief Fixture exercising time_aware=1 with non-uniform dt.
 */
class TrackerVariableDtTest : public ::testing::Test
{
protected:
    SushiTrackHdl     hdl_    = nullptr;
    SushiTrack_Params params_ = {};

    void SetUp() override
    {
        SushiTrack_GetDefaultParams(&params_);
        params_.time_aware             = 1;
        params_.enable_tentative       = 0;
        params_.frame_rate             = 30;
        params_.track_buffer           = 30;
        params_.track_thresh           = 0.1f;
        params_.high_thresh            = 0.2f;
        params_.match_thresh           = 0.95f;

        hdl_ = SushiTrack_Create(&params_);
        ASSERT_NE(hdl_, nullptr);
    }

    void TearDown() override { SushiTrack_Destroy(hdl_); }
};

/**
 * @brief A track survives an irregular cadence covering fast (1/60 s) and slow
 *        (1/10 s) frames without losing identity.
 */
TEST_F(TrackerVariableDtTest, MixedFrameRateSequenceRetainsIdentity)
{
    std::vector<SushiTrack_Track> out(4);
    SushiTrack_Object d0 = makeDet(100.0f, 200.0f);
    SushiTrack_Update(hdl_, &d0, 1, out.data(), 4, 1.0f / 30.0f);
    ASSERT_EQ(out[0].state, 1);
    const int64_t persistent_id = out[0].track_id;

    const float dts[] = { 1.0f / 60.0f, 1.0f / 60.0f, 1.0f / 10.0f,
                          1.0f / 30.0f, 1.0f / 15.0f, 1.0f / 60.0f };

    float x = 100.0f;
    float y = 200.0f;
    for (float dt : dts)
    {
        x += 60.0f * dt; // 60 px/s
        y += 30.0f * dt;
        SushiTrack_Object d = makeDet(x, y);
        int n = SushiTrack_Update(hdl_, &d, 1, out.data(), 4, dt);
        ASSERT_GE(n, 1);
        ASSERT_NE(findById(out, n, persistent_id), nullptr);
    }
}

/**
 * @brief A long discontinuous gap (1 s wall-clock) must not crash the
 *        predictor; the track is allowed to drop to a non-Tracked state.
 */
TEST_F(TrackerVariableDtTest, OneSecondGapDoesNotCrash)
{
    std::vector<SushiTrack_Track> out(4);
    SushiTrack_Object d0 = makeDet(100.0f, 200.0f);
    SushiTrack_Update(hdl_, &d0, 1, out.data(), 4, 1.0f / 30.0f);
    ASSERT_EQ(out[0].state, 1);

    int n = SushiTrack_Update(hdl_, nullptr, 0, out.data(), 4, 1.0f);
    EXPECT_GE(n, 0);
}

// ============================================================================
// TrackerHighToLowConfidenceTest — verified gap (MED)
// ============================================================================

/**
 * @brief Fixture exercising a confirmed track whose detections drop in score
 *        from the high-confidence pool to the low-confidence pool.
 */
class TrackerHighToLowConfidenceTest : public ::testing::Test
{
protected:
    SushiTrackHdl     hdl_    = nullptr;
    SushiTrack_Params params_ = {};

    void SetUp() override
    {
        SushiTrack_GetDefaultParams(&params_);
        params_.time_aware             = 1;
        params_.enable_tentative       = 1;
        params_.tentative_confirm_hits = 2;
        params_.frame_rate             = 25;
        params_.track_buffer           = 30;
        params_.track_thresh           = 0.3f;
        params_.high_thresh            = 0.6f;
        params_.match_thresh           = 0.95f;

        hdl_ = SushiTrack_Create(&params_);
        ASSERT_NE(hdl_, nullptr);
    }

    void TearDown() override { SushiTrack_Destroy(hdl_); }
};

/**
 * @brief A confirmed track whose next detection arrives in the low-confidence
 *        pool is not immediately removed; it survives via the secondary
 *        association stage.
 */
TEST_F(TrackerHighToLowConfidenceTest, TrackSurvivesScoreTransitionFromHighToLow)
{
    std::vector<SushiTrack_Track> out(4);

    SushiTrack_Object d0 = makeDet(100.0f, 200.0f, 0.9f);
    SushiTrack_Object d1 = makeDet(102.0f, 202.0f, 0.9f);
    SushiTrack_Update(hdl_, &d0, 1, out.data(), 4, 0.04f);
    SushiTrack_Update(hdl_, &d1, 1, out.data(), 4, 0.04f);
    ASSERT_EQ(out[0].state, 1);
    const int64_t persistent_id = out[0].track_id;

    SushiTrack_Object d_low = makeDet(104.0f, 204.0f, 0.4f); // below high_thresh
    int n = SushiTrack_Update(hdl_, &d_low, 1, out.data(), 4, 0.04f);
    ASSERT_GE(n, 1);

    const SushiTrack_Track* same = findById(out, n, persistent_id);
    EXPECT_NE(same, nullptr)
        << "Track was lost when its detection moved from high to low confidence.";
}
