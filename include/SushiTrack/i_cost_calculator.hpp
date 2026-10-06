/**************************************************************************/
/* i_cost_calculator.hpp                                                  */
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
 * @file i_cost_calculator.hpp
 * @brief Interface definition for similarity and distance calculation components.
 * @author Mustafa Garip
 */

#pragma once

#include <SushiTrack/cost_matrix.hpp>

#include <memory>
#include <vector>

namespace SushiTrack
{
    class Tracklet;

    /**
     * @brief Abstract interface for similarity and distance metrics.
     */
    class ICostCalculator
    {
    public:
        using TrackletPtr = std::shared_ptr<Tracklet>;

        /**
         * @brief Virtual destructor for the interface.
         */
        virtual ~ICostCalculator() = default;

        /**
         * @brief Computes association costs for all candidate tracklet pairs.
         * @param a Collection of source tracklets.
         * @param b Collection of target tracklets (observations).
         * @param out_matrix Output cost matrix populated in place.
         */
        virtual void calculate(const std::vector<TrackletPtr>& __restrict a,
                               const std::vector<TrackletPtr>& __restrict b,
                               CostMatrix& __restrict out_matrix) = 0;

        /**
         * @brief Computes association cost for a single pair of tracklets.
         * @param a Source tracklet.
         * @param b Target tracklet.
         * @return Computed cost value.
         */
        virtual float calculate_single(const TrackletPtr& a, const TrackletPtr& b) const = 0;
    };

} // namespace SushiTrack
