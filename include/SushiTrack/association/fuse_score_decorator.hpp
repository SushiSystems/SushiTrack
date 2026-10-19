/**************************************************************************/
/* fuse_score_decorator.hpp                                               */
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
 * @file fuse_score_decorator.hpp
 * @brief Declares the decorator that scales costs by detection score.
 * @author Mustafa Garip
 */

#pragma once

#include <memory>
#include <vector>

#include <SushiTrack/association/cost_calculator.hpp>

namespace SushiTrack
{
    /**
     * @brief Score-fusion decorator for any IoU-based cost calculator.
     *
     * Rescales the cost matrix so detections with higher confidence scores
     * receive proportionally lower association cost:
     * fused_cost(i, j) = 1 - (1 - inner_cost(i, j)) * detection_score(j).
     */
    class FuseScoreDecorator final : public CostCalculator
    {
        public:
            /**
             * @brief Wraps an inner cost calculator.
             *
             * @param inner Cost calculator whose output matrix will be rescaled.
             */
            explicit FuseScoreDecorator(std::shared_ptr<CostCalculator> inner)
                : inner_(std::move(inner))
            {
            }

            /**
             * @brief Computes inner costs and applies score fusion in-place.
             *
             * @param a Collection of source tracklets.
             * @param b Collection of target tracklets (observations).
             * @param out_matrix Output cost matrix to be populated.
             */
            void calculate(const std::vector<TrackletPtr>& __restrict a,
                           const std::vector<TrackletPtr>& __restrict b,
                           CostMatrix& __restrict out_matrix) override;
            /**
             * @brief Computes the association cost for a single pair.
             *
             * @param a Source tracklet for association.
             * @param b Candidate tracklet for association.
             * @return float Calculated association cost.
             */
            [[nodiscard]] float calculate_single(const TrackletPtr& a,
                                                 const TrackletPtr& b) const override;

        private:
            std::shared_ptr<CostCalculator> inner_;
    };
} // namespace SushiTrack
