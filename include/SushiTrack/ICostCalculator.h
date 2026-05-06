/**
 * @file      ICostCalculator.h
 * @brief     Interface definition for similarity and distance calculation components.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      28.04.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */

#pragma once

#include "SushiTrack/Tracklet.h"
#include "SushiTrack/CostMatrix.h"
#include <memory>
#include <vector>

namespace SushiTrack
{

    /**
     * @brief Abstract interface for similarity and distance metrics.
     * 
     * Defines the functional requirements for components calculating 
     * association costs between tracklets and observations.
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
             * @brief Computes association costs for all possible tracklet pairs.
             * 
             * Populates the output cost matrix where each element (i, j) 
             * represents the computed distance between the i-th source tracklet 
             * and the j-th target tracklet.
             * 
             * @param a Collection of source tracklets.
             * @param b Collection of target tracklets (observations).
             * @param out_matrix Output cost matrix to be populated.
             */
            virtual void calculate(const std::vector<TrackletPtr>& __restrict a,
                                const std::vector<TrackletPtr>& __restrict b,
                                CostMatrix& __restrict out_matrix) = 0;

            /**
             * @brief Computes association cost for a single pair of tracklets.
             * 
             * @param a Source tracklet.
             * @param b Target tracklet.
             * @return Computed cost.
             */
            virtual float calculate_single(const TrackletPtr& a, const TrackletPtr& b) const = 0;
    };

} // namespace SushiTrack
