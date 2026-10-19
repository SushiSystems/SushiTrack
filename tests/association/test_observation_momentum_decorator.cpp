/**************************************************************************/
/* test_observation_momentum_decorator.cpp                                */
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
 * @file test_observation_momentum_decorator.cpp
 * @brief Tests ObservationMomentumDecorator.
 * @author Mustafa Garip
 */

#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/association/cost_calculator.hpp>
#include <SushiTrack/association/cost_matrix.hpp>
#include <SushiTrack/association/observation_momentum_decorator.hpp>
#include <SushiTrack/filtering/kalman_filter.hpp>
#include <SushiTrack/tracking/tracker.hpp>
#include <SushiTrack/tracking/tracklet.hpp>

#include <tracklet_builders.hpp>

using namespace SushiTrack;

// ObservationMomentumDecorator

namespace
{
    /** @brief Inner calculator returning a fixed cost, isolating the momentum term. */
    class ConstCost : public CostCalculator
    {
        public:
            explicit ConstCost(float c) : c_(c) {}
            void calculate(const std::vector<TrackletPtr>& __restrict a,
                           const std::vector<TrackletPtr>& __restrict b,
                           CostMatrix& __restrict m) override
            {
                m.resize(a.size(), b.size(), c_);
            }
            float calculate_single(const TrackletPtr&, const TrackletPtr&) const override
            {
                return c_;
            }

        private:
            float c_;
    };

    /** @brief Builds a track cruising in +x at the given per-frame step; no IMM, so it reports Cruising. */
    std::shared_ptr<Tracklet> make_cruiser(float start_x, float y, float step, int steps)
    {
        auto t = make_t(start_x, y, 50.f, 80.f);
        t->activate(std::make_unique<KalmanFilter>(), 1, 1);
        float x = start_x;
        for (int f = 2; f <= steps + 1; ++f)
        {
            t->predict(1.0f);
            x += step;
            auto obs = make_t(x, y, 50.f, 80.f);
            t->update(*obs, static_cast<size_t>(f), true, false);
        }
        return t;
    }

    /** @brief Detection whose box centre sits exactly at (cx, cy). */
    std::shared_ptr<Tracklet> det_at_center(float cx, float cy)
    {
        return make_t(cx - 25.f, cy - 40.f, 50.f, 80.f);
    }

    /** @brief Centre of a track's last raw observation. */
    void last_obs_center(const std::shared_ptr<Tracklet>& t, float& cx, float& cy)
    {
        const auto& lo = t->get_last_observation();
        cx = lo.x() + lo.width() * 0.5f;
        cy = lo.y() + lo.height() * 0.5f;
    }
} // namespace

/** @brief A detection straight ahead along travel has its cost reduced by the full weight. */
TEST(Small_ObservationMomentumDecoratorTest, AlignedDetectionGetsLowerCost)
{
    auto t = make_cruiser(100.f, 200.f, 10.f, 5);
    float cx, cy;
    last_obs_center(t, cx, cy);
    auto ahead = det_at_center(cx + 100.f, cy);

    ObservationMomentumDecorator dec(std::make_shared<ConstCost>(0.5f), 0.2f, 0.5f, 5);
    std::vector<std::shared_ptr<Tracklet>> av{t}, bv{ahead};
    CostMatrix m;
    dec.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), 0.3f, 1e-3f);
}

/** @brief A detection directly behind travel has its cost raised by the full weight. */
TEST(Small_ObservationMomentumDecoratorTest, OpposingDetectionGetsHigherCost)
{
    auto t = make_cruiser(100.f, 200.f, 10.f, 5);
    float cx, cy;
    last_obs_center(t, cx, cy);
    auto behind = det_at_center(cx - 100.f, cy);

    ObservationMomentumDecorator dec(std::make_shared<ConstCost>(0.5f), 0.2f, 0.5f, 5);
    std::vector<std::shared_ptr<Tracklet>> av{t}, bv{behind};
    CostMatrix m;
    dec.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), 0.7f, 1e-3f);
}

/** @brief A detection perpendicular to travel leaves the cost unchanged (cos = 0). */
TEST(Small_ObservationMomentumDecoratorTest, PerpendicularDetectionUnchanged)
{
    auto t = make_cruiser(100.f, 200.f, 10.f, 5);
    float cx, cy;
    last_obs_center(t, cx, cy);
    auto side = det_at_center(cx, cy + 100.f);

    ObservationMomentumDecorator dec(std::make_shared<ConstCost>(0.5f), 0.2f, 0.5f, 5);
    std::vector<std::shared_ptr<Tracklet>> av{t}, bv{side};
    CostMatrix m;
    dec.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), 0.5f, 1e-3f);
}

/** @brief Sentinel cells from the inner calculator are passed through untouched. */
TEST(Small_ObservationMomentumDecoratorTest, SentinelPairsArePreserved)
{
    auto t = make_cruiser(100.f, 200.f, 10.f, 5);
    float cx, cy;
    last_obs_center(t, cx, cy);
    auto ahead = det_at_center(cx + 100.f, cy);

    ObservationMomentumDecorator dec(std::make_shared<ConstCost>(1e4f), 0.2f, 0.5f, 5);
    std::vector<std::shared_ptr<Tracklet>> av{t}, bv{ahead};
    CostMatrix m;
    dec.calculate(av, bv, m);

    EXPECT_FLOAT_EQ(m(0, 0), 1e4f);
}

/** @brief A track slower than min_velocity contributes no momentum. */
TEST(Small_ObservationMomentumDecoratorTest, SlowTrackIsGatedOff)
{
    auto t = make_cruiser(100.f, 200.f, 0.2f, 5);
    float cx, cy;
    last_obs_center(t, cx, cy);
    auto ahead = det_at_center(cx + 100.f, cy);

    ObservationMomentumDecorator dec(std::make_shared<ConstCost>(0.5f), 0.2f, 0.5f, 5);
    std::vector<std::shared_ptr<Tracklet>> av{t}, bv{ahead};
    CostMatrix m;
    dec.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), 0.5f, 1e-4f);
}

/** @brief A track whose last observation is older than max_age contributes no momentum. */
TEST(Small_ObservationMomentumDecoratorTest, StaleTrackIsGatedOff)
{
    auto t = make_cruiser(100.f, 200.f, 10.f, 5);
    t->predict(1.0f);
    t->predict(1.0f); // frames-since-observation now 2, beyond max_age = 1

    float cx, cy;
    last_obs_center(t, cx, cy);
    auto ahead = det_at_center(cx + 100.f, cy);

    ObservationMomentumDecorator dec(std::make_shared<ConstCost>(0.5f), 0.2f, 0.5f, 1);
    std::vector<std::shared_ptr<Tracklet>> av{t}, bv{ahead};
    CostMatrix m;
    dec.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), 0.5f, 1e-4f);
}

/** @brief A track with only one observation has no direction and contributes no momentum. */
TEST(Small_ObservationMomentumDecoratorTest, TrackWithoutVelocityIsGatedOff)
{
    auto t = make_t(100.f, 200.f, 50.f, 80.f);
    t->activate(std::make_unique<KalmanFilter>(), 1, 1);

    float cx, cy;
    last_obs_center(t, cx, cy);
    auto ahead = det_at_center(cx + 100.f, cy);

    ObservationMomentumDecorator dec(std::make_shared<ConstCost>(0.5f), 0.2f, 0.5f, 5);
    std::vector<std::shared_ptr<Tracklet>> av{t}, bv{ahead};
    CostMatrix m;
    dec.calculate(av, bv, m);

    EXPECT_NEAR(m(0, 0), 0.5f, 1e-4f);
}

/** @brief calculate_single agrees with the batch path. */
TEST(Small_ObservationMomentumDecoratorTest, SingleMatchesBatch)
{
    auto t = make_cruiser(100.f, 200.f, 10.f, 5);
    float cx, cy;
    last_obs_center(t, cx, cy);
    auto ahead = det_at_center(cx + 100.f, cy);

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
TEST(Small_ObservationMomentumDecoratorTest, ManeuverScaleDoesNotAffectCruisingTrack)
{
    auto t = make_cruiser(100.f, 200.f, 10.f, 5); // no IMM -> reports Cruising
    float cx, cy;
    last_obs_center(t, cx, cy);
    auto ahead = det_at_center(cx + 100.f, cy);
    std::vector<std::shared_ptr<Tracklet>> av{t}, bv{ahead};

    ObservationMomentumDecorator off(std::make_shared<ConstCost>(0.5f), 0.2f, 0.5f, 5, 0.0f);
    ObservationMomentumDecorator on(std::make_shared<ConstCost>(0.5f), 0.2f, 0.5f, 5, 0.5f);
    CostMatrix mo, mn;
    off.calculate(av, bv, mo);
    on.calculate(av, bv, mn);

    EXPECT_FLOAT_EQ(mo(0, 0), mn(0, 0));
    EXPECT_NEAR(mn(0, 0), 0.3f, 1e-3f); // full 0.2 bonus, not scaled
}
