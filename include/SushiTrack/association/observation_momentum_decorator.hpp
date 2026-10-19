/**************************************************************************/
/* observation_momentum_decorator.hpp                                     */
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
 * @file observation_momentum_decorator.hpp
 * @brief Declares the decorator that re-ranks costs by observed motion direction.
 * @author Mustafa Garip
 */

#pragma once

#include <algorithm>
#include <memory>
#include <vector>

#include <SushiTrack/association/cost_calculator.hpp>

namespace SushiTrack
{
    /**
     * @brief Observation-centric momentum decorator for association stage.
     *
     * Applies a bounded subtractive re-ranking term rewarding detections
     * aligned with recent observed motion direction.
     */
    class ObservationMomentumDecorator final : public CostCalculator
    {
        public:
            /**
             * @brief Wraps an inner cost calculator with momentum re-ranking.
             * @param inner Cost calculator whose output matrix is re-ranked.
             * @param vdc_weight Strength and bound of the momentum contribution.
             * @param min_velocity Minimum observed speed for the term to apply.
             * @param max_age Maximum frames since last observation to apply.
             * @param maneuver_weight_scale Weight scale applied to Maneuvering tracks.
             */
            ObservationMomentumDecorator(std::shared_ptr<CostCalculator> inner, float vdc_weight,
                                         float min_velocity, int max_age,
                                         float maneuver_weight_scale = 0.0f)
                : inner_(std::move(inner)),
                  vdc_weight_(vdc_weight),
                  min_velocity_(min_velocity),
                  max_age_(max_age),
                  maneuver_weight_scale_(std::clamp(maneuver_weight_scale, 0.0f, 1.0f))
            {
            }

            /**
             * @brief Computes inner costs and applies the momentum re-ranking in place.
             *
             * @param a Collection of source tracklets.
             * @param b Collection of target tracklets (observations).
             * @param out_matrix Output cost matrix to be populated and re-ranked.
             */
            void calculate(const std::vector<TrackletPtr>& __restrict a,
                           const std::vector<TrackletPtr>& __restrict b,
                           CostMatrix& __restrict out_matrix) override;
            /**
             * @brief Computes the momentum-adjusted association cost for a single pair.
             *
             * @param a Source tracklet for association.
             * @param b Candidate tracklet for association.
             * @return float Re-ranked association cost.
             */
            [[nodiscard]] float calculate_single(const TrackletPtr& a,
                                                 const TrackletPtr& b) const override;

        private:
            /**
             * @brief Bounded directional bonus for one track/detection pair.
             *
             * @param track Source tracklet supplying the observed direction.
             * @param det Candidate detection.
             * @param weight Effective bound for this track (full for Cruising, scaled for Maneuvering).
             * @return Value in [-weight, +weight]; positive when aligned with travel.
             */
            [[nodiscard]] float momentum_bonus(const TrackletPtr& track, const TrackletPtr& det,
                                               float weight) const;

            /**
             * @brief Per-track effective momentum weight, or 0 when the track must be skipped.
             *
             * Returns vdc_weight for Cruising, vdc_weight * maneuver_weight_scale for
             * Maneuvering (0 disables that case), and 0 for Stationary/Coasting and any
             * track failing the velocity/age/observation gates.
             */
            [[nodiscard]] float effective_weight(const TrackletPtr& track) const;

            std::shared_ptr<CostCalculator> inner_;
            float vdc_weight_;
            float min_velocity_;
            int max_age_;
            float maneuver_weight_scale_;
    };
} // namespace SushiTrack
