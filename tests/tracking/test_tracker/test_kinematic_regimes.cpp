/**************************************************************************/
/* test_kinematic_regimes.cpp                                             */
/**************************************************************************/
/*                          This file is part of:                         */
/*                               SushiTrack                               */
/*               https://github.com/SushiSystems/SushiTrack               */
/*                         https://sushisystems.io                        */
/**************************************************************************/
/* Copyright (c) 2026-present Mustafa Garip & Sushi Systems               */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License"); you    */
/* may not use this file except in compliance with the License. You may   */
/* obtain a copy of the License at                                        */
/*                                                                        */
/*     https://www.apache.org/licenses/LICENSE-2.0                        */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                         */
/**************************************************************************/

/**
 * @file test_kinematic_regimes.cpp
 * @brief Tests the kinematic regimes the tracker reports.
 * @author Mustafa Garip
 */

#include <algorithm>
#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/c_api/sushitrack_c.h>

#include <c_detection_builders.hpp>

using namespace CDetections;

namespace
{
    /** @brief Kinematic regime codes as reported in sushitrack_track_t::kinematic_state. */
    enum KinematicCode
    {
        K_STATIONARY = 0,
        K_CRUISING = 1,
        K_MANEUVERING = 2,
        K_COASTING = 3
    };

    /** @brief The number of frames a scene runs and the regime its last frame must report. */
    struct RegimeRow
    {
            int frames;
            int regime;
    };
} // namespace

/**
 * @brief IMM fixture: kalman_type=2 on the otherwise frame-counted core path.
 *
 * The IMM mode bank produces the regime that is mapped to the kinematic_state output: near
 * zero speed or a near-constant-position mass reads Stationary, a CA and CT mass above 0.5
 * reads Maneuvering, anything else Cruising, and a Lost track reads Coasting.
 */
class Small_TrackerKinematicStateTest : public ::testing::Test
{
    protected:
        sushitrack_tracker_t hdl_ = nullptr;
        sushitrack_params_t params_ = {};
        std::vector<sushitrack_track_t> out_ = std::vector<sushitrack_track_t>(8);

        void SetUp() override
        {
            sushitrack_get_default_params(&params_);
            params_.time_aware = 0;
            params_.enable_tentative = 0;
            params_.kalman_type = 2;
            params_.primary_assoc_mode = 1;
            params_.frame_rate = 25;
            params_.track_buffer = 30;
            params_.track_thresh = 0.1f;
            params_.high_thresh = 0.5f;
            params_.match_thresh = 0.9f;

            hdl_ = sushitrack_create(&params_);
            ASSERT_NE(hdl_, nullptr);
        }

        void TearDown() override
        {
            if (hdl_)
                sushitrack_destroy(hdl_);
        }

        /**
         * @brief Feeds one detection at (@p x, 200) and returns the regime of the first track,
         *        or -1 when the frame left no track.
         */
        int feed(float x)
        {
            const sushitrack_object_t detection = make_det(x, 200.0f);
            const int n = update_frame(hdl_, detection, out_.data(), 8, 0.04f);
            return n >= 1 ? out_[0].kinematic_state : -1;
        }

        /**
         * @brief Feeds @p frames frames of a box that never moves and returns the regime of
         *        the last one.
         */
        int hold_still(int frames)
        {
            int regime = -1;
            for (int i = 0; i < frames; ++i)
                regime = feed(100.0f);
            return regime;
        }

        /**
         * @brief Feeds @p frames frames of a box that speeds up by 0.25 px per frame each frame
         *        up to 5 px per frame and then holds, and returns the regime of the last one.
         *
         * The ramp acceleration, about 156 px/s^2 at 25 fps, stays under the 250 px/s^2
         * maneuver threshold, and the hold speed of 125 px/s sits above the 40 px/s floor.
         */
        int ramp_to_cruise(int frames)
        {
            float x = 100.0f;
            float v = 0.0f;
            int regime = -1;
            for (int i = 0; i < frames; ++i)
            {
                v = std::min(5.0f, v + 0.25f);
                x += v;
                regime = feed(x);
            }
            return regime;
        }

        /**
         * @brief Settles a box for six frames, then jumps it 15 px per frame for eight, and
         *        returns how many of those eight frames reported Maneuvering.
         *
         * The step is small enough that each detection still overlaps the predicted box, so the
         * track stays associated while the Kalman velocity ramps up over several frames.
         */
        int onset_frames_in_maneuvering()
        {
            hold_still(6);
            float x = 100.0f;
            int maneuvering = 0;
            for (int i = 0; i < 8; ++i)
            {
                x += 15.0f;
                if (feed(x) == K_MANEUVERING)
                    ++maneuvering;
            }
            return maneuvering;
        }

        /**
         * @brief Confirms a box that moves 4 px per frame for eight frames, withholds
         *        @p missed frames and returns the regime of the track the last one left, or -1
         *        when no track is left.
         */
        int regime_after_misses(int missed)
        {
            float x = 100.0f;
            for (int i = 0; i < 8; ++i)
            {
                feed(x);
                x += 4.0f;
            }
            const int64_t id = out_[0].track_id;
            int n = 0;
            for (int i = 0; i < missed; ++i)
                n = update_frame(hdl_, nullptr, 0, out_.data(), 8, 0.04f);
            const sushitrack_track_t* track = find_by_id(out_, n, id);
            return track != nullptr ? track->kinematic_state : -1;
        }
};

/**
 * @brief The fixture, run over the frames at which a motionless box is read.
 */
class Small_TrackerStationaryRegimeTest : public Small_TrackerKinematicStateTest,
                                          public ::testing::WithParamInterface<RegimeRow>
{
};

/**
 * @brief A box held at one position reads Stationary from its second frame on.
 *
 * The first frame only seeds the track, which keeps the default Cruising regime. Every later
 * frame is an update whose measurement equals the estimate, so the velocity stays exactly zero,
 * below 0.02 of the box height per step, and the tracklet reads Stationary whatever the IMM mode
 * mass is.
 */
TEST_P(Small_TrackerStationaryRegimeTest, MotionlessObjectReadsStationary)
{
    EXPECT_EQ(hold_still(GetParam().frames), GetParam().regime);
}

INSTANTIATE_TEST_SUITE_P(Frames, Small_TrackerStationaryRegimeTest,
                         ::testing::Values(RegimeRow{2, K_STATIONARY}, RegimeRow{10, K_STATIONARY}));

/**
 * @brief An object that accelerates gently to a steady speed and holds it reports Cruising:
 *        faster than the stationary floor, never over the maneuver ceiling.
 */
TEST_F(Small_TrackerKinematicStateTest, SteadyMotionReportsCruising)
{
    EXPECT_EQ(ramp_to_cruise(45), K_CRUISING);
}

/**
 * @brief A motionless object that jerks into 15 px per frame reports Maneuvering while the
 *        acceleration lasts.
 */
TEST_F(Small_TrackerKinematicStateTest, AbruptAccelerationReportsManeuvering)
{
    EXPECT_GE(onset_frames_in_maneuvering(), 1)
        << "An abrupt high-acceleration event never entered the Maneuvering regime.";
}

/**
 * @brief The fixture, run over the number of frames a confirmed track is withheld.
 */
class Small_TrackerCoastingRegimeTest : public Small_TrackerKinematicStateTest,
                                        public ::testing::WithParamInterface<int>
{
};

/**
 * @brief A confirmed track that loses its detection reads Coasting on its Lost output.
 *
 * The first missed frame calls mark_as_lost, which refreshes the regime with is_lost set, and
 * every later missed frame predicts a Lost track, which sets it again. The IMM predictor reports
 * a non-negative manoeuvre probability, so the regime is not the single-model default.
 */
TEST_P(Small_TrackerCoastingRegimeTest, UnobservedTrackReportsCoasting)
{
    EXPECT_EQ(regime_after_misses(GetParam()), K_COASTING);
}

INSTANTIATE_TEST_SUITE_P(Misses, Small_TrackerCoastingRegimeTest, ::testing::Values(1, 3));
