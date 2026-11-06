/**************************************************************************/
/* iou_cost_calculator.hpp                                                */
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
 * @file iou_cost_calculator.hpp
 * @brief Declares the IoU cost calculator.
 * @author Mustafa Garip
 */

#pragma once

#include <vector>

#include <SushiTrack/association/cost_calculator.hpp>

namespace SushiTrack
{
    /**
     * @brief Intersection over Union (IoU) cost calculator.
     *
     * Calculates the overlap ratio between two sets of bounding boxes. The
     * resulting cost matrix contains values inversely proportional to the
     * intersection area relative to the union area.
     */
    class IoUCostCalculator final : public CostCalculator
    {
        public:
            /**
             * @brief Initializes the IoU calculator with area constraints.
             *
             * @param max_area_ratio Maximum allowed ratio between box areas for association.
             * @param iou_plus_one Controls the inversion formula for cost calculation.
             */
            explicit IoUCostCalculator(float max_area_ratio = 4.0f, bool iou_plus_one = false)
                : max_area_ratio_(max_area_ratio), iou_plus_one_(iou_plus_one)
            {
            }

            /**
             * @brief Populates the cost matrix using overlap-based metrics.
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
            float max_area_ratio_;
            bool iou_plus_one_;
    };
} // namespace SushiTrack
