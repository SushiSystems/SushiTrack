/**
 * @file      DistanceStrategies.h
 * @brief     Association cost calculation strategies for multi-object tracking.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      28.04.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */

#pragma once

#include "SushiTrack/ICostCalculator.h"
#include "SushiTrack/IGateValidator.h"
#include <algorithm>
#include <memory>

namespace SushiTrack
{
    /**
     * @brief Intersection over Union (IoU) cost calculator.
     * 
     * Calculates the overlap ratio between two sets of bounding boxes. The 
     * resulting cost matrix contains values inversely proportional to the 
     * intersection area relative to the union area.
     */
    class IoUCostCalculator : public ICostCalculator
    {
        public:
            /**
             * @brief Initializes the IoU calculator with area constraints.
             * 
             * @param max_area_ratio Maximum allowed ratio between box areas for association.
             * @param iou_plus_one Controls the inversion formula for cost calculation.
             */
            IoUCostCalculator(float max_area_ratio = 4.0f, bool iou_plus_one = false)
                : max_area_ratio_(max_area_ratio), iou_plus_one_(iou_plus_one) {}

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
            float calculate_single(const TrackletPtr& a, const TrackletPtr& b) const override;
        private:
            float max_area_ratio_;
            bool  iou_plus_one_;
    };

    /**
     * @brief Distance-IoU (DIoU) cost calculator.
     * 
     * Calculates association costs based on the intersection-over-union ratio 
     * and the normalized distance between bounding box centroids.
     */
    class DIoUCostCalculator : public ICostCalculator
    {
        public:
            /**
             * @brief Initializes the DIoU calculator.
             * 
             * @param max_area_ratio Maximum allowed ratio between box areas.
             * @param iou_plus_one Controls the inversion formula for cost calculation.
             */
            DIoUCostCalculator(float max_area_ratio = 4.0f, bool iou_plus_one = false)
                : max_area_ratio_(max_area_ratio), iou_plus_one_(iou_plus_one) {}

            /**
             * @brief Populates the cost matrix using center-distance aware overlap metrics.
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
        private:
            float max_area_ratio_;
            bool  iou_plus_one_;
            std::vector<float> row_areas_;             ///< Cached areas of source tracklet bounding boxes.
            std::vector<float> col_areas_;             ///< Cached areas of candidate tracklet bounding boxes.
    };

    /**
     * @brief Generalized Intersection over Union (GIoU) cost calculator.
     * 
     * Calculates costs based on the IoU and the ratio of empty space 
     * in the smallest enclosing bounding box.
     */
    class GIoUCostCalculator : public ICostCalculator
    {
        public:
            /**
             * @brief Initializes the GIoU calculator.
             * 
             * @param max_area_ratio Maximum allowed ratio between box areas.
             * @param iou_plus_one Controls the inversion formula for cost calculation.
             */
            GIoUCostCalculator(float max_area_ratio = 4.0f, bool iou_plus_one = false)
                : max_area_ratio_(max_area_ratio), iou_plus_one_(iou_plus_one) {}

            /**
             * @brief Populates the cost matrix using GIoU metrics.
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
        private:
            float max_area_ratio_;
            bool  iou_plus_one_;
            std::vector<float> row_areas_;
            std::vector<float> col_areas_;
    };

    /**
     * @brief SCYLLA-IoU (SIoU) cost calculator.
     * 
     * Calculates association costs by incorporating angle, distance, and shape 
     * penalties into the standard IoU formulation.
     */
    class SIoUCostCalculator : public ICostCalculator
    {
        public:
            /**
             * @brief Initializes the SIoU calculator.
             * 
             * @param max_area_ratio Maximum allowed ratio between box areas.
             * @param iou_plus_one Controls the inversion formula for cost calculation.
             */
            SIoUCostCalculator(float max_area_ratio = 4.0f, bool iou_plus_one = false)
                : max_area_ratio_(max_area_ratio), iou_plus_one_(iou_plus_one) {}

            /**
             * @brief Populates the cost matrix using SIoU metrics.
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
        private:
            float max_area_ratio_;
            bool  iou_plus_one_;
            std::vector<float> row_areas_;
            std::vector<float> col_areas_;
    };

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

    /**
     * @brief Prioritized association strategy using visual features.
     *
     * Evaluates visual similarity and applies bounding box overlap constraints 
     * as a secondary filter or fallback.
     */
    class ReIDFirstCostCalculator : public ICostCalculator
    {
        public:
            /**
             * @brief Initializes the prioritized ReID strategy.
             * 
             * @param reid_confidence_thresh Minimum similarity for direct association.
             * @param iou_weight Weighting factor for fallback overlap calculations.
             * @param reid_direct_assignment_weight Scaling factor for high-confidence direct visual matches.
             * @param iou_calc Implementation for spatial overlap calculation.
             * @param cosine_calc Implementation for visual similarity calculation.
             */
            ReIDFirstCostCalculator(
                float reid_confidence_thresh = 0.3f,
                float iou_weight = 0.3f,
                float reid_direct_assignment_weight = 0.2f,
                std::shared_ptr<ICostCalculator> iou_calc = nullptr,
                std::shared_ptr<ICostCalculator> cosine_calc = nullptr)
                : reid_conf_thresh_(reid_confidence_thresh),
                  iou_weight_(iou_weight),
                  reid_direct_assignment_weight_(reid_direct_assignment_weight),
                  iou_calc_(iou_calc),
                  cosine_calc_(cosine_calc) {}

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
            float calculate_single(const TrackletPtr& a, const TrackletPtr& b) const override;

        private:
            float reid_conf_thresh_;
            float iou_weight_;
            float reid_direct_assignment_weight_;
            std::shared_ptr<ICostCalculator> iou_calc_;
            std::shared_ptr<ICostCalculator> cosine_calc_;

    };

    /**
     * @brief Visual-only association strategy for track recovery.
     *
     * Calculates costs strictly based on feature vector similarity.
     */
    class PureReIDCostCalculator : public ICostCalculator
    {
        public:
            /**
             * @brief Initializes the visual-only matcher.
             * 
             * @param reid_threshold Minimum visual similarity threshold.
             * @param gating_thresh Statistical threshold for the Mahalanobis gate.
             * @param gate_validator Implementation for motion-based gating.
             */
            PureReIDCostCalculator(float reid_threshold = 0.4f,
                                   float gating_thresh = 9.4877f,
                                   std::shared_ptr<IGateValidator> gate_validator = nullptr)
                : reid_thresh_(reid_threshold),
                  gating_thresh_(gating_thresh),
                  gate_validator_(std::move(gate_validator)) {}

            /**
             * @brief Populates the cost matrix using strictly visual feature vectors.
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

        private:
            float reid_thresh_;
            float gating_thresh_;
            std::shared_ptr<IGateValidator> gate_validator_;
            KalmanTypes::DetectBoxVector meas_workspace_; ///< Pre-allocated buffer for measurement vectors to reduce heap churn.
            std::vector<float>           score_workspace_; ///< Pre-allocated buffer for per-detection scores (NSA gating).
    };

    /**
     * @brief Integrated cost calculator using spatial, temporal, and visual metrics.
     * 
     * Combines bounding box overlap, visual similarity, and motion-based 
     * Mahalanobis distance gating.
     */
    class FusedCostCalculator : public ICostCalculator
    {
        public:
            /**
             * @brief Initializes the fused cost calculator.
             * 
             * @param iou_weight Weighting factor for spatial overlap.
             * @param reid_thresh Visual similarity threshold.
             * @param gating_thresh Motion-based validation threshold.
             * @param gate_validator Implementation for motion-based gating.
             * @param iou_calc Implementation for spatial overlap calculation.
             * @param cosine_calc Implementation for visual similarity calculation.
             * @param reid_direct_assignment_weight Scaling factor for high-confidence direct visual matches.
             * @param proximity_thresh Minimum raw IoU a pair must satisfy before the
             *        statistical gate is consulted. Pairs whose overlap falls below
             *        this value are rejected outright with a sentinel cost. A value
             *        of zero disables the filter, preserving the legacy behaviour.
             */
            FusedCostCalculator(float iou_weight,
                                float reid_thresh, float gating_thresh,
                                std::shared_ptr<IGateValidator> gate_validator,
                                std::shared_ptr<ICostCalculator> iou_calc,
                                std::shared_ptr<ICostCalculator> cosine_calc,
                                float reid_direct_assignment_weight = 0.0f,
                                float proximity_thresh = 0.0f);

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
            float calculate_single(const TrackletPtr& a, const TrackletPtr& b) const override;

        private:
            float iou_weight_;
            float reid_thresh_;
            float gating_thresh_;
            std::shared_ptr<IGateValidator> gate_validator_;
            std::shared_ptr<ICostCalculator> iou_calc_;
            std::shared_ptr<ICostCalculator> cosine_calc_;
            float reid_direct_assignment_weight_;
            float proximity_thresh_; ///< Raw-IoU pre-filter applied before statistical gating; zero disables it.

            KalmanTypes::DetectBoxVector meas_workspace_; ///< Pre-allocated buffer for measurement vectors to reduce heap churn.
            std::vector<float>           score_workspace_; ///< Pre-allocated buffer for per-detection scores (NSA gating).
    };

    /**
     * @brief Score-fusion decorator for any IoU-based cost calculator.
     *
     * Wraps an inner ICostCalculator and rescales the produced cost matrix so
     * that detections with a higher confidence score receive proportionally
     * lower association cost. Mirrors the fuse_score behaviour from the
     * reference ByteTrack pipeline:
     *
     *     fused_cost(i, j) = 1 - (1 - inner_cost(i, j)) * detection_score(j)
     *
     * Sentinel cells (>= 1e3) produced by the inner calculator for invalid
     * pairs (gated out, area-ratio violation, etc.) are preserved as-is so
     * that the assignment solver continues to reject them.
     *
     * This decorator is intentionally only applied to the primary
     * high-confidence association stage; low-confidence detections already
     * have low scores and would be unfairly penalised twice.
     */
    class FuseScoreDecorator : public ICostCalculator
    {
        public:
            /**
             * @brief Wraps an inner cost calculator.
             *
             * @param inner Cost calculator whose output matrix will be rescaled.
             */
            explicit FuseScoreDecorator(std::shared_ptr<ICostCalculator> inner)
                : inner_(std::move(inner)) {}

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
            float calculate_single(const TrackletPtr& a, const TrackletPtr& b) const override;

        private:
            std::shared_ptr<ICostCalculator> inner_;
    };

    /**
     * @brief Observation-centric momentum decorator for the primary association stage.
     *
     * Wraps an inner cost calculator and applies a bounded subtractive
     * re-ranking term that rewards detections lying in the direction a track was
     * observed to be moving and penalises those moving against it. The direction
     * is derived from raw observations (Tracklet::recordObservation), not from
     * Kalman velocity, so it complements the filter-based gating in the inner
     * metric instead of duplicating it.
     *
     * The term is deliberately conservative — a tie-breaker, not a primary
     * signal — to avoid the identity-switch failure mode of an unbounded momentum
     * bonus on a pipeline that already fuses score, DIoU, and Mahalanobis gating:
     *   - bounded: clamped to [-vdc_weight, +vdc_weight], so it can only re-order
     *     candidates inside the gate and never rescues a gated-out (sentinel) pair.
     *   - regime-gated: applied only to tracks reported as Cruising (steady
     *     linear motion). Stationary, Maneuvering, and Coasting tracks are skipped,
     *     since their observed direction is meaningless or unreliable — exactly the
     *     cases where naive momentum injects switches. Without an IMM regime every
     *     track reports Cruising and the speed/age gates below carry the load.
     *   - speed-gated: tracks slower than min_velocity (px per observed frame)
     *     contribute nothing; their direction is dominated by noise.
     *   - freshness-gated: applied only when the last observation is at most
     *     max_age frames old, so a stale pre-occlusion direction cannot pull a
     *     recovered identity toward where it used to be heading.
     */
    class ObservationMomentumDecorator : public ICostCalculator
    {
        public:
            /**
             * @brief Wraps an inner cost calculator with the momentum re-ranking term.
             *
             * @param inner Cost calculator whose output matrix is re-ranked in place.
             * @param vdc_weight Strength and bound of the momentum contribution.
             * @param min_velocity Minimum observed speed (px per observed frame) for the term to apply.
             * @param max_age Maximum frames since the last observation for the term to apply.
             * @param maneuver_weight_scale Fraction of vdc_weight applied to Maneuvering tracks; 0 restricts the term to Cruising.
             */
            ObservationMomentumDecorator(std::shared_ptr<ICostCalculator> inner,
                                         float vdc_weight,
                                         float min_velocity,
                                         int max_age,
                                         float maneuver_weight_scale = 0.0f)
                : inner_(std::move(inner)),
                  vdc_weight_(vdc_weight),
                  min_velocity_(min_velocity),
                  max_age_(max_age),
                  maneuver_weight_scale_(std::clamp(maneuver_weight_scale, 0.0f, 1.0f)) {}

            /**
             * @brief Computes inner costs and applies the momentum re-ranking in place.
             *
             * @param a Collection of source tracklets.
             * @param b Collection of target tracklets (observations).
             * @param out_matrix Output cost matrix to be populated and re-ranked.
             */
            void calculate(const std::vector<TrackletPtr>& __restrict a,
                        const std::vector<TrackletPtr>& __restrict b,
                        CostMatrix& __restrict out_matrix) override;
            /**
             * @brief Computes the momentum-adjusted association cost for a single pair.
             *
             * @param a Source tracklet for association.
             * @param b Candidate tracklet for association.
             * @return float Re-ranked association cost.
             */
            float calculate_single(const TrackletPtr& a, const TrackletPtr& b) const override;

        private:
            /**
             * @brief Bounded directional bonus for one track/detection pair.
             *
             * @param track Source tracklet supplying the observed direction.
             * @param det Candidate detection.
             * @param weight Effective bound for this track (full for Cruising, scaled for Maneuvering).
             * @return Value in [-weight, +weight]; positive when aligned with travel.
             */
            float momentumBonus(const TrackletPtr& track, const TrackletPtr& det, float weight) const;

            /**
             * @brief Per-track effective momentum weight, or 0 when the track must be skipped.
             *
             * Returns vdc_weight for Cruising, vdc_weight * maneuver_weight_scale for
             * Maneuvering (0 disables that case), and 0 for Stationary/Coasting and any
             * track failing the velocity/age/observation gates.
             */
            float effectiveWeight(const TrackletPtr& track) const;

            std::shared_ptr<ICostCalculator> inner_;
            float vdc_weight_;
            float min_velocity_;
            int   max_age_;
            float maneuver_weight_scale_;
    };

    /**
     * @brief Spatial overlap calculator with motion-based gating.
     *
     * Calculates IoU costs and applies penalties based on Mahalanobis distance
     * validation gates.
     */
    class GatedIoUCostCalculator : public ICostCalculator
    {
        public:
            /**
             * @brief Initializes the gated IoU calculator.
             * 
             * @param gating_thresh Statistical threshold for the Mahalanobis gate.
             * @param gate_validator Implementation for motion-based gating.
             * @param iou_calc Implementation for spatial overlap calculation.
             * @param motion_cost_weight Weighting factor for motion-based distance penalty.
             * @param proximity_thresh Minimum raw IoU a pair must satisfy before the
             *        statistical gate is consulted. Pairs whose overlap falls below
             *        this value are rejected outright with a sentinel cost. A value
             *        of zero disables the filter, preserving the legacy behaviour.
             */
            GatedIoUCostCalculator(float gating_thresh,
                                std::shared_ptr<IGateValidator> gate_validator,
                                std::shared_ptr<ICostCalculator> iou_calc,
                                float motion_cost_weight = 0.0f,
                                float proximity_thresh = 0.0f);

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
            float calculate_single(const TrackletPtr& a, const TrackletPtr& b) const override;

        private:
            float gating_thresh_;
            float motion_cost_weight_;
            float proximity_thresh_; ///< Raw-IoU pre-filter applied before statistical gating; zero disables it.
            std::shared_ptr<IGateValidator> gate_validator_;
            std::shared_ptr<ICostCalculator> iou_calc_;

            KalmanTypes::DetectBoxVector meas_workspace_; ///< Pre-allocated buffer for measurement vectors to reduce heap churn.
            std::vector<float>           score_workspace_; ///< Pre-allocated buffer for per-detection scores (NSA gating).
    };

} // namespace SushiTrack