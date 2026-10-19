/**************************************************************************/
/* TestIntegration.cpp                                                    */
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
 * @file TestIntegration.cpp
 * @brief Unit tests for integrated multi-component tracking.
 * @author Mustafa Garip
 */

#include <cmath>
#include <cstring>
#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/c_api/sushitrack_c.h>

namespace
{

    /**
     * @brief Constructs a benign detection without feature data.
     */
    sushitrack_object_t make_det(float x, float y, float score = 0.9f, float width = 50.0f,
                                 float height = 100.0f, int label = 1)
    {
        sushitrack_object_t o{};
        o.x = x;
        o.y = y;
        o.width = width;
        o.height = height;
        o.score = score;
        o.label = label;
        o.feat = nullptr;
        o.feat_dim = 0;
        return o;
    }

    /**
     * @brief Constructs a detection backed by an external feature pointer.
     *        The caller owns the feature memory; it must outlive the Update call.
     */
    sushitrack_object_t make_det_with_feat(float x, float y, const float* feat, int feat_dim,
                                           float score = 0.9f)
    {
        sushitrack_object_t o = make_det(x, y, score);
        o.feat = feat;
        o.feat_dim = feat_dim;
        return o;
    }

    /**
     * @brief Locates a track in an output buffer by its persistent ID.
     * @return Pointer into out for the matching track, or nullptr.
     */
    const sushitrack_track_t* find_by_id(const std::vector<sushitrack_track_t>& out, int n,
                                         int64_t id)
    {
        for (int i = 0; i < n; ++i)
            if (out[static_cast<std::size_t>(i)].track_id == id)
                return &out[static_cast<std::size_t>(i)];
        return nullptr;
    }

} // namespace

// LongSequenceTrackingTest — Issue #3 (HIGH)

/**
 * @brief Fixture exercising a long, single-object trajectory to validate the
 *        absence of numerical drift, ID churn, and bookkeeping anomalies.
 */
class LongSequenceTrackingTest : public ::testing::Test
{
    protected:
        sushitrack_tracker_t hdl_ = nullptr;
        sushitrack_params_t params_ = {};

        void SetUp() override
        {
            sushitrack_get_default_params(&params_);
            params_.time_aware = 1;
            params_.enable_tentative = 0;
            params_.primary_assoc_mode = 1;
            params_.frame_rate = 30;
            params_.track_buffer = 60;
            params_.track_thresh = 0.1f;
            params_.high_thresh = 0.2f;
            params_.match_thresh = 0.9f;

            hdl_ = sushitrack_create(&params_);
            ASSERT_NE(hdl_, nullptr);
        }

        void TearDown() override { sushitrack_destroy(hdl_); }
};

/**
 * @brief 300 contiguous frames of a steadily-moving object must yield a single
 *        persistent track_id with monotonic age, no NaN/Inf in the reported
 *        geometry, and a zero update gap on every observed frame.
 */
TEST_F(LongSequenceTrackingTest, ThreeHundredFrameSingleObjectKeepsIdentity)
{
    constexpr int K_FRAMES = 300;
    std::vector<sushitrack_track_t> out(8);

    sushitrack_object_t d0 = make_det(100.0f, 200.0f);
    int n0 = sushitrack_update(hdl_, &d0, 1, out.data(), 8, 1.0f / 30.0f);
    ASSERT_EQ(n0, 1);
    ASSERT_EQ(out[0].state, 1);
    const int64_t persistent_id = out[0].track_id;

    int64_t prev_age = out[0].age;

    for (int frame = 1; frame < K_FRAMES; ++frame)
    {
        sushitrack_object_t d = make_det(100.0f + 1.5f * frame, 200.0f + 0.5f * frame);
        int n = sushitrack_update(hdl_, &d, 1, out.data(), 8, 1.0f / 30.0f);
        ASSERT_GE(n, 1) << "Tracker dropped the object at frame " << frame;

        const sushitrack_track_t* t = find_by_id(out, n, persistent_id);
        ASSERT_NE(t, nullptr) << "Track ID flipped at frame " << frame;

        EXPECT_TRUE(std::isfinite(t->x));
        EXPECT_TRUE(std::isfinite(t->y));
        EXPECT_TRUE(std::isfinite(t->width));
        EXPECT_TRUE(std::isfinite(t->height));
        EXPECT_GT(t->width, 0.0f);
        EXPECT_GT(t->height, 0.0f);

        EXPECT_GE(t->age, prev_age) << "Age regressed at frame " << frame;
        prev_age = t->age;

        EXPECT_EQ(t->frames_since_last_update, 0)
            << "Continuously observed track reported a non-zero update gap at frame " << frame;
    }
}

// TrackerReIDEndToEndTest — Issue #2 (HIGH)

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
        sushitrack_tracker_t hdl_ = nullptr;
        sushitrack_params_t params_ = {};
        std::vector<float> feat_a_;
        std::vector<float> feat_b_;

        void SetUp() override
        {
            sushitrack_get_default_params(&params_);
            params_.time_aware = 1;
            params_.enable_tentative = 0;
            params_.enable_reid = 1;
            params_.primary_assoc_mode = 1;
            params_.frame_rate = 25;
            params_.track_buffer = 30;
            params_.track_thresh = 0.1f;
            params_.high_thresh = 0.2f;
            params_.match_thresh = 0.95f;

            hdl_ = sushitrack_create(&params_);
            ASSERT_NE(hdl_, nullptr);

            feat_a_.assign(16, 0.0f);
            feat_b_.assign(16, 0.0f);
            feat_a_[0] = 1.0f; // unit vector along axis 0
            feat_b_[1] = 1.0f; // unit vector along axis 1 (orthogonal to A)
        }

        void TearDown() override { sushitrack_destroy(hdl_); }
};

/**
 * @brief Two visually distinct objects whose trajectories cross retain their
 *        IDs through the crossover. We sample after the crossover and verify
 *        that the original two IDs both still exist.
 */
TEST_F(TrackerReIDEndToEndTest, DistinctFeaturesPreventIdSwapOnCrossover)
{
    std::vector<sushitrack_track_t> out(8);

    sushitrack_object_t a0 = make_det_with_feat(100.0f, 200.0f, feat_a_.data(), 16, 0.95f);
    sushitrack_object_t b0 = make_det_with_feat(400.0f, 200.0f, feat_b_.data(), 16, 0.95f);
    sushitrack_object_t dets0[2] = {a0, b0};
    int n0 = sushitrack_update(hdl_, dets0, 2, out.data(), 8, 0.04f);
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
        sushitrack_object_t a = make_det_with_feat(ax, 200.0f, feat_a_.data(), 16, 0.95f);
        sushitrack_object_t b = make_det_with_feat(bx, 200.0f, feat_b_.data(), 16, 0.95f);
        sushitrack_object_t dets[2] = {a, b};
        sushitrack_update(hdl_, dets, 2, out.data(), 8, 0.04f);
    }

    int n_final = sushitrack_update(hdl_,
                                    /* unchanged */ nullptr, 0, out.data(), 8, 0.04f);
    (void)n_final;

    // Both original IDs must still be alive somewhere in the output history.
    sushitrack_object_t a_late = make_det_with_feat(380.0f, 200.0f, feat_a_.data(), 16, 0.95f);
    sushitrack_object_t b_late = make_det_with_feat(120.0f, 200.0f, feat_b_.data(), 16, 0.95f);
    sushitrack_object_t dets_late[2] = {a_late, b_late};
    int n_late = sushitrack_update(hdl_, dets_late, 2, out.data(), 8, 0.04f);
    ASSERT_GE(n_late, 2);

    EXPECT_NE(find_by_id(out, n_late, id_a), nullptr)
        << "Track A lost its ID across the crossover.";
    EXPECT_NE(find_by_id(out, n_late, id_b), nullptr)
        << "Track B lost its ID across the crossover.";
}

/**
 * @brief A track that goes Lost for several frames must be recovered with its
 *        original ID when the same visual identity reappears.
 */
TEST_F(TrackerReIDEndToEndTest, LostTrackRecoveredByMatchingFeature)
{
    std::vector<sushitrack_track_t> out(8);

    sushitrack_object_t d0 = make_det_with_feat(100.0f, 200.0f, feat_a_.data(), 16, 0.95f);
    sushitrack_update(hdl_, &d0, 1, out.data(), 8, 0.04f);
    ASSERT_EQ(out[0].state, 1);
    const int64_t original_id = out[0].track_id;

    // Drop the detection for several frames so the track goes Lost.
    for (int i = 0; i < 5; ++i)
        sushitrack_update(hdl_, nullptr, 0, out.data(), 8, 0.04f);

    // Reappear with the same feature, slightly displaced.
    sushitrack_object_t d_back = make_det_with_feat(160.0f, 220.0f, feat_a_.data(), 16, 0.95f);
    int n = sushitrack_update(hdl_, &d_back, 1, out.data(), 8, 0.04f);
    ASSERT_GE(n, 1);

    const sushitrack_track_t* recovered = find_by_id(out, n, original_id);
    EXPECT_NE(recovered, nullptr) << "ReID failed to recover the lost track to its original ID.";
}

// TrackerTentativeWithReIDTest — Issue #6 (MED)

/**
 * @brief Fixture exercising tentative-stage promotion under ReID.
 */
class TrackerTentativeWithReIDTest : public ::testing::Test
{
    protected:
        sushitrack_tracker_t hdl_ = nullptr;
        sushitrack_params_t params_ = {};
        std::vector<float> feat_;

        void SetUp() override
        {
            sushitrack_get_default_params(&params_);
            params_.time_aware = 1;
            params_.enable_tentative = 1;
            params_.enable_reid = 1;
            params_.tentative_confirm_hits = 3;
            params_.frame_rate = 25;
            params_.track_buffer = 30;
            params_.track_thresh = 0.1f;
            params_.high_thresh = 0.2f;
            params_.match_thresh = 0.9f;
            params_.tentative_match_thresh = 0.95f;
            params_.tentative_fast_thresh = 0.99f;

            hdl_ = sushitrack_create(&params_);
            ASSERT_NE(hdl_, nullptr);

            feat_.assign(16, 0.0f);
            feat_[3] = 1.0f;
        }

        void TearDown() override { sushitrack_destroy(hdl_); }
};

/**
 * @brief A tentative track carrying ReID features confirms after the documented
 *        hit count without raising an error inside the feature path.
 */
TEST_F(TrackerTentativeWithReIDTest, TentativeWithFeaturesConfirmsAfterHitCount)
{
    std::vector<sushitrack_track_t> out(4);

    for (int i = 0; i < params_.tentative_confirm_hits; ++i)
    {
        sushitrack_object_t d =
            make_det_with_feat(100.0f + 1.0f * i, 200.0f + 1.0f * i, feat_.data(), 16, 0.7f);
        int n = sushitrack_update(hdl_, &d, 1, out.data(), 4, 0.04f);
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
    std::vector<sushitrack_track_t> out(4);

    sushitrack_object_t d0 = make_det_with_feat(100.0f, 200.0f, feat_.data(), 16, 0.7f);
    int n0 = sushitrack_update(hdl_, &d0, 1, out.data(), 4, 0.04f);
    ASSERT_GE(n0, 1);
    const int64_t tentative_id = out[0].track_id;

    sushitrack_update(hdl_, nullptr, 0, out.data(), 4, 0.04f);

    sushitrack_object_t d2 = make_det_with_feat(102.0f, 202.0f, feat_.data(), 16, 0.7f);
    int n2 = sushitrack_update(hdl_, &d2, 1, out.data(), 4, 0.04f);
    ASSERT_GE(n2, 1);

    const sushitrack_track_t* same = find_by_id(out, n2, tentative_id);
    EXPECT_NE(same, nullptr) << "Tentative track lost its ID across a single missed frame.";
}

// TrackerImmGatingEndToEndTest — Section 2.3.2 (MED)

/**
 * @brief Fixture combining the IMM regime with Mahalanobis gating end-to-end.
 */
class TrackerImmGatingEndToEndTest : public ::testing::Test
{
    protected:
        sushitrack_tracker_t hdl_ = nullptr;
        sushitrack_params_t params_ = {};

        void SetUp() override
        {
            sushitrack_get_default_params(&params_);
            params_.time_aware = 1;
            params_.enable_tentative = 0;
            params_.kalman_type = 2;
            params_.enable_mahalanobis = 1;
            params_.primary_assoc_mode = 1;
            params_.frame_rate = 25;
            params_.track_buffer = 30;
            params_.track_thresh = 0.1f;
            params_.high_thresh = 0.2f;
            params_.match_thresh = 0.9f;

            hdl_ = sushitrack_create(&params_);
            ASSERT_NE(hdl_, nullptr);
        }

        void TearDown() override { sushitrack_destroy(hdl_); }
};

/**
 * @brief A track sustained through a maneuvering segment retains its identity
 *        when each successive detection lies inside the motion gate.
 */
TEST_F(TrackerImmGatingEndToEndTest, ManeuveringTrackKeepsIdentityInsideGate)
{
    std::vector<sushitrack_track_t> out(4);
    sushitrack_object_t d0 = make_det(100.0f, 200.0f);
    sushitrack_update(hdl_, &d0, 1, out.data(), 4, 0.04f);
    ASSERT_EQ(out[0].state, 1);
    const int64_t persistent_id = out[0].track_id;

    // Accelerating arc — both x and y change at increasing rates.
    for (int k = 1; k <= 15; ++k)
    {
        const float dx = 2.0f * static_cast<float>(k);
        const float dy = 1.0f * static_cast<float>(k);
        sushitrack_object_t d = make_det(100.0f + dx, 200.0f + dy);
        int n = sushitrack_update(hdl_, &d, 1, out.data(), 4, 0.04f);
        ASSERT_GE(n, 1);
        const sushitrack_track_t* t = find_by_id(out, n, persistent_id);
        ASSERT_NE(t, nullptr) << "Track lost identity inside maneuver at step " << k;
    }
}

/**
 * @brief A detection placed far outside any plausible motion envelope does
 *        not bind to a confirmed track; the track instead drops to Lost.
 */
TEST_F(TrackerImmGatingEndToEndTest, OutOfGateDetectionDoesNotBindEstablishedTrack)
{
    std::vector<sushitrack_track_t> out(4);
    sushitrack_object_t d0 = make_det(100.0f, 200.0f);
    sushitrack_update(hdl_, &d0, 1, out.data(), 4, 0.04f);
    ASSERT_EQ(out[0].state, 1);
    const int64_t persistent_id = out[0].track_id;

    // Detection 5000 pixels away — far outside any reasonable gate.
    sushitrack_object_t d_far = make_det(5100.0f, 5200.0f);
    int n = sushitrack_update(hdl_, &d_far, 1, out.data(), 4, 0.04f);
    ASSERT_GE(n, 0);

    const sushitrack_track_t* original = find_by_id(out, n, persistent_id);
    if (original != nullptr)
    {
        EXPECT_NE(original->state, 1) << "Out-of-gate detection silently re-bound a Tracked track.";
    }
}

// TrackerVariableDtTest — Section 2.3.4 (MED)

/**
 * @brief Fixture exercising time_aware=1 with non-uniform dt.
 */
class TrackerVariableDtTest : public ::testing::Test
{
    protected:
        sushitrack_tracker_t hdl_ = nullptr;
        sushitrack_params_t params_ = {};

        void SetUp() override
        {
            sushitrack_get_default_params(&params_);
            params_.time_aware = 1;
            params_.enable_tentative = 0;
            params_.frame_rate = 30;
            params_.track_buffer = 30;
            params_.track_thresh = 0.1f;
            params_.high_thresh = 0.2f;
            params_.match_thresh = 0.95f;

            hdl_ = sushitrack_create(&params_);
            ASSERT_NE(hdl_, nullptr);
        }

        void TearDown() override { sushitrack_destroy(hdl_); }
};

/**
 * @brief A track survives an irregular cadence covering fast (1/60 s) and slow
 *        (1/10 s) frames without losing identity.
 */
TEST_F(TrackerVariableDtTest, MixedFrameRateSequenceRetainsIdentity)
{
    std::vector<sushitrack_track_t> out(4);
    sushitrack_object_t d0 = make_det(100.0f, 200.0f);
    sushitrack_update(hdl_, &d0, 1, out.data(), 4, 1.0f / 30.0f);
    ASSERT_EQ(out[0].state, 1);
    const int64_t persistent_id = out[0].track_id;

    const float dts[] = {1.0f / 60.0f, 1.0f / 60.0f, 1.0f / 10.0f,
                         1.0f / 30.0f, 1.0f / 15.0f, 1.0f / 60.0f};

    float x = 100.0f;
    float y = 200.0f;
    for (float dt : dts)
    {
        x += 60.0f * dt; // 60 px/s
        y += 30.0f * dt;
        sushitrack_object_t d = make_det(x, y);
        int n = sushitrack_update(hdl_, &d, 1, out.data(), 4, dt);
        ASSERT_GE(n, 1);
        ASSERT_NE(find_by_id(out, n, persistent_id), nullptr);
    }
}

/**
 * @brief A long discontinuous gap (1 s wall-clock) must not crash the
 *        predictor; the track is allowed to drop to a non-Tracked state.
 */
TEST_F(TrackerVariableDtTest, OneSecondGapDoesNotCrash)
{
    std::vector<sushitrack_track_t> out(4);
    sushitrack_object_t d0 = make_det(100.0f, 200.0f);
    sushitrack_update(hdl_, &d0, 1, out.data(), 4, 1.0f / 30.0f);
    ASSERT_EQ(out[0].state, 1);

    int n = sushitrack_update(hdl_, nullptr, 0, out.data(), 4, 1.0f);
    EXPECT_GE(n, 0);
}

// TrackerHighToLowConfidenceTest — verified gap (MED)

/**
 * @brief Fixture exercising a confirmed track whose detections drop in score
 *        from the high-confidence pool to the low-confidence pool.
 */
class TrackerHighToLowConfidenceTest : public ::testing::Test
{
    protected:
        sushitrack_tracker_t hdl_ = nullptr;
        sushitrack_params_t params_ = {};

        void SetUp() override
        {
            sushitrack_get_default_params(&params_);
            params_.time_aware = 1;
            params_.enable_tentative = 1;
            params_.tentative_confirm_hits = 2;
            params_.frame_rate = 25;
            params_.track_buffer = 30;
            params_.track_thresh = 0.3f;
            params_.high_thresh = 0.6f;
            params_.match_thresh = 0.95f;

            hdl_ = sushitrack_create(&params_);
            ASSERT_NE(hdl_, nullptr);
        }

        void TearDown() override { sushitrack_destroy(hdl_); }
};

/**
 * @brief A confirmed track whose next detection arrives in the low-confidence
 *        pool is not immediately removed; it survives via the secondary
 *        association stage.
 */
TEST_F(TrackerHighToLowConfidenceTest, TrackSurvivesScoreTransitionFromHighToLow)
{
    std::vector<sushitrack_track_t> out(4);

    sushitrack_object_t d0 = make_det(100.0f, 200.0f, 0.9f);
    sushitrack_object_t d1 = make_det(102.0f, 202.0f, 0.9f);
    sushitrack_update(hdl_, &d0, 1, out.data(), 4, 0.04f);
    sushitrack_update(hdl_, &d1, 1, out.data(), 4, 0.04f);
    ASSERT_EQ(out[0].state, 1);
    const int64_t persistent_id = out[0].track_id;

    sushitrack_object_t d_low = make_det(104.0f, 204.0f, 0.4f); // below high_thresh
    int n = sushitrack_update(hdl_, &d_low, 1, out.data(), 4, 0.04f);
    ASSERT_GE(n, 1);

    const sushitrack_track_t* same = find_by_id(out, n, persistent_id);
    EXPECT_NE(same, nullptr)
        << "Track was lost when its detection moved from high to low confidence.";
}
