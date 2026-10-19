/**************************************************************************/
/* cosine_cost_calculator.cpp                                             */
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
 * @file cosine_cost_calculator.cpp
 * @brief Defines the cosine appearance cost calculator.
 * @author Mustafa Garip
 */

#include <SushiTrack/association/cosine_cost_calculator.hpp>

#include <algorithm>
#include <vector>

#include <SushiTrack/tracking/tracklet.hpp>

namespace SushiTrack
{
    /**
     * @brief Computes association costs using visual feature cosine distance.
     * @param a Input collection of source tracklets for association.
     * @param b Input collection of candidate tracklets for association.
     * @param out_matrix Reference to the cost matrix for result population.
     */
    void CosineCostCalculator::calculate(const std::vector<TrackletPtr>& __restrict a,
                                        const std::vector<TrackletPtr>& __restrict b,
                                        CostMatrix& __restrict out_matrix)
    {
        out_matrix.resize(a.size(), b.size(), 1.0f);
        for (size_t i = 0; i < a.size(); ++i)
        {
            for (size_t j = 0; j < b.size(); ++j)
            {
                if (!a[i]->hasFeature() || !b[j]->hasFeature())
                {
                    out_matrix(i, j) = 1.0f;
                    continue;
                }
                float max_sim = a[i]->getMaxFeatureSimilarity(b[j]->getCurrFeat());
                out_matrix(i, j) = std::max(0.0f, 1.0f - max_sim);
            }
        }
    }

    /**
     * @brief Computes the association cost for a single pair using cosine distance.
     * @param a Source tracklet for association.
     * @param b Candidate tracklet for association.
     * @return Calculated visual feature distance.
     */
    float CosineCostCalculator::calculate_single(const TrackletPtr& a, const TrackletPtr& b) const
    {
        if (!a->hasFeature() || !b->hasFeature()) return 1.0f;
        return std::max(0.0f, 1.0f - a->getMaxFeatureSimilarity(b->getCurrFeat()));
    }
} // namespace SushiTrack
