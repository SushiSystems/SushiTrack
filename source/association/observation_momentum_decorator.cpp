/**************************************************************************/
/* observation_momentum_decorator.cpp                                     */
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
 * @file observation_momentum_decorator.cpp
 * @brief Defines the decorator that re-ranks costs by observed motion direction.
 * @author Mustafa Garip
 */

#include <SushiTrack/association/observation_momentum_decorator.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

#include <SushiTrack/tracking/kinematic_state.hpp>
#include <SushiTrack/tracking/tracklet.hpp>

namespace SushiTrack
{
    /**
     * @brief Computes bounded directional bonus weight for one track/detection pair.
     * @param track Source tracklet supplying observed direction.
     * @return Bounded bonus weight.
     */
    float ObservationMomentumDecorator::effectiveWeight(const TrackletPtr& track) const
    {
        if (vdc_weight_ <= 0.0f) return 0.0f;
        if (!track->hasObsVelocity() || !track->hasLastObservation()) return 0.0f;
        if (track->getObsSpeed() < min_velocity_) return 0.0f;
        if (track->getFramesSinceObservation() > max_age_) return 0.0f;

        switch (track->getKinematicState())
        {
            case KinematicState::Cruising:    return vdc_weight_;
            case KinematicState::Maneuvering: return vdc_weight_ * maneuver_weight_scale_;
            default:                          return 0.0f;  // Stationary/Coasting: stored direction is meaningless.
        }
    }

    float ObservationMomentumDecorator::momentumBonus(const TrackletPtr& track, const TrackletPtr& det, float weight) const
    {
        if (weight <= 0.0f) return 0.0f;

        const auto& lo = track->getLastObservation();
        const float ocx = lo.x() + lo.width()  * 0.5f;
        const float ocy = lo.y() + lo.height() * 0.5f;

        const auto& dr = det->getRect();
        const float dcx = dr.x() + dr.width()  * 0.5f;
        const float dcy = dr.y() + dr.height() * 0.5f;

        const float ddx = dcx - ocx;
        const float ddy = dcy - ocy;
        const float dn  = std::sqrt(ddx * ddx + ddy * ddy);
        if (dn < 1e-3f) return 0.0f;

        const float cos_theta = (track->getObsVelocityX() * ddx + track->getObsVelocityY() * ddy) / dn;
        return std::clamp(weight * cos_theta, -weight, weight);
    }

    /**
     * @brief Computes inner costs and applies the momentum re-ranking in place.
     * @param a Collection of source tracklets.
     * @param b Collection of target tracklets (observations).
     * @param out_matrix Output cost matrix populated by inner calculator and re-ranked.
     */
    void ObservationMomentumDecorator::calculate(const std::vector<TrackletPtr>& __restrict a,
                                                 const std::vector<TrackletPtr>& __restrict b,
                                                 CostMatrix& __restrict out_matrix)
    {
        inner_->calculate(a, b, out_matrix);
        if (a.empty() || b.empty() || vdc_weight_ <= 0.0f) return;

        constexpr float kSentinel = 1e3f;

        for (size_t i = 0; i < a.size(); ++i)
        {
            // Per-row gate first so skipped tracks avoid the detection loop.
            const float weight = effectiveWeight(a[i]);
            if (weight <= 0.0f) continue;

            for (size_t j = 0; j < b.size(); ++j)
            {
                float c = out_matrix(i, j);
                if (c >= kSentinel) continue;
                out_matrix(i, j) = c - momentumBonus(a[i], b[j], weight);
            }
        }
    }

    /**
     * @brief Computes the momentum-adjusted association cost for a single pair.
     * @param a Source tracklet for association.
     * @param b Candidate tracklet for association.
     * @return Re-ranked association cost.
     */
    float ObservationMomentumDecorator::calculate_single(const TrackletPtr& a, const TrackletPtr& b) const
    {
        float c = inner_->calculate_single(a, b);
        if (c >= 1e3f) return c;
        return c - momentumBonus(a, b, effectiveWeight(a));
    }
} // namespace SushiTrack
