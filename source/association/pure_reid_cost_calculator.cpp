/**************************************************************************/
/* pure_reid_cost_calculator.cpp                                          */
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
 * @file pure_reid_cost_calculator.cpp
 * @brief Defines the appearance-only cost calculator for track recovery.
 * @author Mustafa Garip
 */

#include <SushiTrack/association/pure_reid_cost_calculator.hpp>

#include <vector>

#include <SushiTrack/logging/logger.hpp>
#include <SushiTrack/tracking/tracklet.hpp>

namespace SushiTrack
{
    /**
     * @brief Computes association costs strictly based on visual feature vectors.
     * @param a Input collection of source tracklets for association.
     * @param b Input collection of candidate tracklets for association.
     * @param out_matrix Reference to the cost matrix for result population.
     */
    void PureReIDCostCalculator::calculate(const std::vector<TrackletPtr>& __restrict a,
                                           const std::vector<TrackletPtr>& __restrict b,
                                           CostMatrix& __restrict out_matrix)
    {
        out_matrix.resize(a.size(), b.size(), 1e4f);
        if (a.empty() || b.empty())
            return;

        meas_workspace_.clear();
        score_workspace_.clear();
        if (gate_validator_)
        {
            meas_workspace_.reserve(b.size());
            score_workspace_.reserve(b.size());
            for (const auto& det : b)
            {
                meas_workspace_.push_back(det->get_measurement());
                score_workspace_.push_back(det->get_score());
            }
        }

        size_t gated_pairs = 0;

        for (size_t i = 0; i < a.size(); ++i)
        {
            if (!a[i]->has_feature())
                continue;

            std::vector<bool> in_gate;
            if (gate_validator_)
            {
                in_gate = gate_validator_->get_gate_flags(a[i]->get_mean(), a[i]->get_covariance(),
                                                          meas_workspace_, gating_thresh_,
                                                          score_workspace_);
            }

            for (size_t j = 0; j < b.size(); ++j)
            {
                if (!b[j]->has_feature())
                    continue;

                if (gate_validator_ && !in_gate[j])
                {
                    ++gated_pairs;
                    out_matrix(i, j) = 1e4f;
                    continue;
                }

                float reid_sim = a[i]->get_max_feature_similarity(b[j]->get_curr_feat());
                float reid_dist = 1.0f - reid_sim;

                if (reid_dist < reid_thresh_)
                    out_matrix(i, j) = reid_dist;
            }
        }

        if (gated_pairs > 0)
        {
            SUSHITRACK_LOG_TRACE(K_SUSHITRACK_LOG_CATEGORY,
                                 "PureReID: gated out {} of {} pairs (Mahalanobis).", gated_pairs,
                                 a.size() * b.size());
        }
    }

    /**
     * @brief Computes association cost for a single pair using strictly visual features.
     * @param a Source tracklet for association.
     * @param b Candidate tracklet for association.
     * @return Calculated association cost based on visual features.
     */
    float PureReIDCostCalculator::calculate_single(const TrackletPtr& a, const TrackletPtr& b) const
    {
        if (!a->has_feature() || !b->has_feature())
        {
            return 1e4f;
        }

        if (gate_validator_)
        {
            KalmanTypes::DetectBoxVector meas_workspace = {b->get_measurement()};
            std::vector<float> score_workspace = {b->get_score()};
            std::vector<bool> in_gate =
                gate_validator_->get_gate_flags(a->get_mean(), a->get_covariance(), meas_workspace,
                                                gating_thresh_, score_workspace);
            if (!in_gate[0])
            {
                return 1e4f;
            }
        }

        float reid_sim = a->get_max_feature_similarity(b->get_curr_feat());
        float reid_dist = 1.0f - reid_sim;

        if (reid_dist < reid_thresh_)
        {
            return reid_dist;
        }

        return 1e4f;
    }
} // namespace SushiTrack
