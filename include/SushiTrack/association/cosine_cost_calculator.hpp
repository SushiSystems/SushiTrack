/**************************************************************************/
/* cosine_cost_calculator.hpp                                             */
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
 * @file cosine_cost_calculator.hpp
 * @brief Declares the cosine appearance cost calculator.
 * @author Mustafa Garip
 */

#pragma once

#include <vector>

#include <SushiTrack/association/i_cost_calculator.hpp>

namespace SushiTrack
{
    /**
     * @brief Visual similarity cost calculator using cosine distance.
     * 
     * Calculates costs based on the angular distance between high-dimensional 
     * feature vectors.
     */
    class CosineCostCalculator : public ICostCalculator
    {
        public:
            /**
             * @brief Populates the cost matrix using visual feature similarity.
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
            float calculate_single(const TrackletPtr& a, const TrackletPtr& b) const override;
    };
} // namespace SushiTrack
