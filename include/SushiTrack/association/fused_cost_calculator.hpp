/**************************************************************************/
/* fused_cost_calculator.hpp                                              */
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
 * @file fused_cost_calculator.hpp
 * @brief Declares the cost calculator that fuses overlap, appearance and motion gating.
 * @author Mustafa Garip
 */

#pragma once

#include <memory>
#include <vector>

#include <SushiTrack/association/cost_calculator.hpp>
#include <SushiTrack/filtering/gate_validator.hpp>
#include <SushiTrack/filtering/kalman_types.hpp>

namespace SushiTrack
{
    /**
     * @brief Integrated cost calculator using spatial, temporal, and visual metrics.
     *
     * Combines bounding box overlap, visual similarity, and motion-based
     * Mahalanobis distance gating.
     */
    class FusedCostCalculator final : public CostCalculator
    {
        public:
            /**
             * @brief Initializes the fused cost calculator.
             *
             * Combines spatial overlap, visual similarity, and motion gating.
             */
            FusedCostCalculator(float iou_weight, float reid_thresh, float gating_thresh,
                                std::shared_ptr<GateValidator> gate_validator,
                                std::shared_ptr<CostCalculator> iou_calc,
                                std::shared_ptr<CostCalculator> cosine_calc,
                                float reid_direct_assignment_weight = 0.0f);

            /**
             * @brief Populates the cost matrix by aggregating multiple metrics.
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
            float iou_weight_;
            float reid_thresh_;
            float gating_thresh_;
            std::shared_ptr<GateValidator> gate_validator_;
            std::shared_ptr<CostCalculator> iou_calc_;
            std::shared_ptr<CostCalculator> cosine_calc_;
            float reid_direct_assignment_weight_;

            KalmanTypes::DetectBoxVector
                meas_workspace_; ///< Pre-allocated buffer for measurement vectors to reduce heap churn.
            std::vector<float>
                score_workspace_; ///< Pre-allocated buffer for per-detection scores (NSA gating).
    };
} // namespace SushiTrack
