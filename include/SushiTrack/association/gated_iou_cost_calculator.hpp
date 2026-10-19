/**************************************************************************/
/* gated_iou_cost_calculator.hpp                                          */
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
 * @file gated_iou_cost_calculator.hpp
 * @brief Declares the IoU cost calculator gated by motion.
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
     * @brief Spatial overlap calculator with motion-based gating.
     *
     * Calculates IoU costs and applies penalties based on Mahalanobis distance
     * validation gates.
     */
    class GatedIoUCostCalculator final : public CostCalculator
    {
        public:
            /**
             * @brief Initializes the gated IoU calculator.
             *
             * @param gating_thresh Statistical threshold for the Mahalanobis gate.
             * @param gate_validator Implementation for motion-based gating.
             * @param iou_calc Implementation for spatial overlap calculation.
             * @param motion_cost_weight Weighting factor for motion-based distance penalty.
             */
            GatedIoUCostCalculator(float gating_thresh,
                                   std::shared_ptr<GateValidator> gate_validator,
                                   std::shared_ptr<CostCalculator> iou_calc,
                                   float motion_cost_weight = 0.0f);

            /**
             * @brief Populates the cost matrix with spatial overlap and motion constraints.
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
            float gating_thresh_;
            float motion_cost_weight_;
            std::shared_ptr<GateValidator> gate_validator_;
            std::shared_ptr<CostCalculator> iou_calc_;

            KalmanTypes::DetectBoxVector
                meas_workspace_; ///< Pre-allocated buffer for measurement vectors to reduce heap churn.
            std::vector<float>
                score_workspace_; ///< Pre-allocated buffer for per-detection scores (NSA gating).
    };
} // namespace SushiTrack
