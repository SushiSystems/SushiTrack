/**************************************************************************/
/* reid_first_cost_calculator.hpp                                         */
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
 * @file reid_first_cost_calculator.hpp
 * @brief Declares the cost calculator that tries appearance before overlap.
 * @author Mustafa Garip
 */

#pragma once

#include <memory>
#include <vector>

#include <SushiTrack/association/cost_calculator.hpp>

namespace SushiTrack
{
    /**
     * @brief Prioritized association strategy using visual features.
     *
     * Evaluates visual similarity and applies bounding box overlap constraints
     * as a secondary filter or fallback.
     */
    class ReIDFirstCostCalculator final : public CostCalculator
    {
        public:
            /**
             * @brief Initializes the prioritized ReID strategy.
             * @param reid_confidence_thresh Minimum similarity for direct association.
             * @param iou_weight Weighting factor for fallback overlap calculations.
             * @param reid_direct_assignment_weight Scaling factor for direct matches.
             * @param iou_calc Implementation for spatial overlap calculation.
             * @param cosine_calc Implementation for visual similarity calculation.
             */
            explicit ReIDFirstCostCalculator(float reid_confidence_thresh = 0.3f,
                                             float iou_weight = 0.3f,
                                             float reid_direct_assignment_weight = 0.2f,
                                             std::shared_ptr<CostCalculator> iou_calc = nullptr,
                                             std::shared_ptr<CostCalculator> cosine_calc = nullptr)
                : reid_conf_thresh_(reid_confidence_thresh),
                  iou_weight_(iou_weight),
                  reid_direct_assignment_weight_(reid_direct_assignment_weight),
                  iou_calc_(iou_calc),
                  cosine_calc_(cosine_calc)
            {
            }

            /**
             * @brief Integrates appearance and overlap metrics with appearance priority.
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
            float reid_conf_thresh_;
            float iou_weight_;
            float reid_direct_assignment_weight_;
            std::shared_ptr<CostCalculator> iou_calc_;
            std::shared_ptr<CostCalculator> cosine_calc_;
    };
} // namespace SushiTrack
