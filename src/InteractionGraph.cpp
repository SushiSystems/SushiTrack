/**
 * @file      InteractionGraph.cpp
 * @brief     Implementation of the bottom-Y depth-ordering interaction graph.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      11.05.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */

#include "SushiTrack/InteractionGraph.h"
#include "SushiTrack/Tracklet.h"

#include <algorithm>
#include <cmath>

namespace SushiTrack
{
    namespace
    {
        /**
         * @brief Computes the Distance-IoU similarity of two axis-aligned
         *        bounding boxes expressed in top-left/width/height format.
         *
         * Distance-IoU augments the standard IoU with a centre-distance
         * penalty that is normalised by the squared diagonal of the smallest
         * enclosing rectangle. The penalty is subtracted from the IoU so the
         * resulting similarity remains directionally consistent with plain
         * IoU on overlapping boxes (a value of one is achieved only when the
         * two boxes coincide), while strongly penalising configurations in
         * which two boxes overlap slightly but their centroids are far apart
         * in the enclosing rectangle. The latter case occurs frequently
         * during partial occlusions of elongated objects (vehicles, pallets)
         * where IoU is misleadingly forgiving.
         *
         * @param ax Top-left X of the first rectangle.
         * @param ay Top-left Y of the first rectangle.
         * @param aw Width of the first rectangle.
         * @param ah Height of the first rectangle.
         * @param bx Top-left X of the second rectangle.
         * @param by Top-left Y of the second rectangle.
         * @param bw Width of the second rectangle.
         * @param bh Height of the second rectangle.
         * @return DIoU similarity in the range [-1, 1]. The activation
         *         threshold used by the interaction graph is interpreted in
         *         the same direction as for plain IoU.
         */
        inline float diouTlwh(float ax, float ay, float aw, float ah,
                              float bx, float by, float bw, float bh)
        {
            const float ax2 = ax + aw;
            const float ay2 = ay + ah;
            const float bx2 = bx + bw;
            const float by2 = by + bh;

            const float iw = std::min(ax2, bx2) - std::max(ax, bx);
            const float ih = std::min(ay2, by2) - std::max(ay, by);
            float iou = 0.0f;
            if (iw > 0.0f && ih > 0.0f)
            {
                const float inter = iw * ih;
                const float uni   = aw * ah + bw * bh - inter;
                if (uni > 1e-6f) iou = inter / uni;
            }

            const float cx_a = ax + aw * 0.5f;
            const float cy_a = ay + ah * 0.5f;
            const float cx_b = bx + bw * 0.5f;
            const float cy_b = by + bh * 0.5f;
            const float dx = cx_a - cx_b;
            const float dy = cy_a - cy_b;
            const float d2 = dx * dx + dy * dy;

            const float enc_w = std::max(ax2, bx2) - std::min(ax, bx);
            const float enc_h = std::max(ay2, by2) - std::min(ay, by);
            const float c2 = enc_w * enc_w + enc_h * enc_h;
            if (c2 <= 1e-6f) return iou;

            return iou - d2 / c2;
        }

        /**
         * @brief Ranks a kinematic state by perceived camera proximity.
         *
         * Used as the FSM-driven tiebreaker for depth ordering when two
         * overlapping tracklets have near-equal bottom-Y coordinates. The
         * intuition is that Stationary and Maneuvering tracks are more
         * likely to correspond to foreground objects: a Stationary track
         * tends to represent a parked or standing target the camera is
         * close to, and a Maneuvering track exhibits the kind of fast
         * apparent motion that disambiguates near objects from background.
         * Cruising and Coasting tracks default to a lower rank so they
         * concede precedence when the geometric ordering is ambiguous.
         *
         * Higher rank means more likely to be in front.
         *
         * @param s Kinematic state to score.
         * @return Integer rank in the closed range [0, 3].
         */
        inline int kinematicDepthRank(KinematicState s)
        {
            switch (s)
            {
                case KinematicState::Stationary:  return 3;
                case KinematicState::Maneuvering: return 2;
                case KinematicState::Cruising:    return 1;
                case KinematicState::Coasting:    return 0;
            }
            return 1;
        }
    }

    /**
     * @brief Initializes the graph with the DIoU activation threshold.
     *
     * @param iou_thresh Minimum DIoU similarity required to record an
     *                   interaction between two tracklets. The parameter
     *                   name is preserved for configuration compatibility
     *                   even though the underlying metric is now DIoU.
     */
    BottomYInteractionGraph::BottomYInteractionGraph(float iou_thresh)
        : iou_thresh_(std::max(-1.0f, iou_thresh))
    {
    }

    /**
     * @brief Builds the per-frame interaction graph.
     *
     * The algorithm proceeds in two passes:
     *   1. Each tracklet that exposes a positive track identifier is
     *      recorded with its predicted bounding box and the kinematic state
     *      that was active at the time of the build. Tracklets with a zero
     *      identifier (unassigned tentatives) are skipped because they
     *      cannot be referenced by an occluder snapshot.
     *   2. Pairwise DIoU similarity is evaluated for all valid records.
     *      When the similarity exceeds the activation threshold, the
     *      bottom-Y comparison decides which tracklet is in front. When the
     *      foot points are near-equal (within a small fraction of the
     *      shared height), the kinematic depth rank is used as tiebreaker.
     *      The tracklet behind receives the front one as its occluder
     *      candidate; if a stronger overlap is observed later, the
     *      candidate is upgraded.
     *
     * @param tracklets Collection of tracklets to analyse.
     */
    void BottomYInteractionGraph::build(const std::vector<TrackletPtr>& tracklets)
    {
        records_.clear();

        std::vector<size_t> ids;
        ids.reserve(tracklets.size());

        for (const auto& t : tracklets)
        {
            if (!t) continue;
            const size_t tid = t->getTrackId();
            if (tid == 0) continue;

            const auto& r = t->getRect();
            Record rec;
            rec.x = r.x();
            rec.y = r.y();
            rec.w = r.width();
            rec.h = r.height();
            rec.kinematic_state = t->getLastSeenState();
            rec.occluder_id = kNoOccluder;
            records_[tid] = rec;
            ids.push_back(tid);
        }

        std::vector<float> best_sim(ids.size(), -2.0f);

        // The tiebreaker activates when the foot-point separation is below
        // a small fraction of the shared height. The fraction is intentionally
        // tight (10 percent) so that the FSM rank only intervenes on truly
        // ambiguous geometric pairs and never overrides a clear bottom-Y
        // ordering.
        constexpr float kFootEpsRatio = 0.10f;

        for (size_t i = 0; i + 1 < ids.size(); ++i)
        {
            Record& ri = records_[ids[i]];
            for (size_t j = i + 1; j < ids.size(); ++j)
            {
                Record& rj = records_[ids[j]];
                const float sim = diouTlwh(ri.x, ri.y, ri.w, ri.h,
                                           rj.x, rj.y, rj.w, rj.h);
                if (sim < iou_thresh_) continue;

                const float bottom_i = ri.y + ri.h;
                const float bottom_j = rj.y + rj.h;
                const float foot_eps = kFootEpsRatio * 0.5f * (ri.h + rj.h);

                bool i_is_front;
                if (std::fabs(bottom_i - bottom_j) <= foot_eps)
                {
                    // Geometric tie: fall back to the kinematic rank. Ties at
                    // the rank level are resolved by keeping the lexically
                    // smaller identifier in front, which is deterministic
                    // and prevents oscillation across consecutive frames.
                    const int ri_rank = kinematicDepthRank(ri.kinematic_state);
                    const int rj_rank = kinematicDepthRank(rj.kinematic_state);
                    if (ri_rank != rj_rank)
                        i_is_front = (ri_rank > rj_rank);
                    else
                        i_is_front = (ids[i] < ids[j]);
                }
                else
                {
                    i_is_front = (bottom_i > bottom_j);
                }

                if (i_is_front)
                {
                    if (sim > best_sim[j])
                    {
                        best_sim[j] = sim;
                        rj.occluder_id = static_cast<int>(ids[i]);
                    }
                }
                else
                {
                    if (sim > best_sim[i])
                    {
                        best_sim[i] = sim;
                        ri.occluder_id = static_cast<int>(ids[j]);
                    }
                }
            }
        }
    }

    /**
     * @brief Retrieves the identifier of the front-most occluder.
     *
     * @param tracklet_id Track identifier of the queried tracklet.
     * @return Identifier of the closest front-neighbour, or kNoOccluder if no
     *         qualifying neighbour was recorded.
     */
    int BottomYInteractionGraph::getFrontOccluder(size_t tracklet_id) const
    {
        const auto it = records_.find(tracklet_id);
        if (it == records_.end()) return kNoOccluder;
        return it->second.occluder_id;
    }

    /**
     * @brief Tests whether the identifier participates in the graph.
     *
     * @param tracklet_id Track identifier to test.
     * @return True when the identifier corresponds to a tracklet passed to
     *         the most recent build call.
     */
    bool BottomYInteractionGraph::contains(size_t tracklet_id) const
    {
        return records_.find(tracklet_id) != records_.end();
    }

    /**
     * @brief Computes the DIoU similarity between a stored occluder
     *        rectangle and a candidate rectangle.
     *
     * @param occluder_id Identifier of the occluder tracklet.
     * @param cand_x Top-left horizontal coordinate of the candidate.
     * @param cand_y Top-left vertical coordinate of the candidate.
     * @param cand_w Width of the candidate.
     * @param cand_h Height of the candidate.
     * @return DIoU similarity in the range [-1, 1], or zero when the
     *         occluder is unknown to the graph.
     */
    float BottomYInteractionGraph::occluderOverlap(size_t occluder_id,
                                                   float cand_x, float cand_y,
                                                   float cand_w, float cand_h) const
    {
        const auto it = records_.find(occluder_id);
        if (it == records_.end()) return 0.0f;
        const Record& r = it->second;
        return diouTlwh(r.x, r.y, r.w, r.h, cand_x, cand_y, cand_w, cand_h);
    }

    /**
     * @brief Retrieves the recorded kinematic state for a tracklet.
     *
     * @param tracklet_id Track identifier to query.
     * @return KinematicState captured at build time, or Cruising when the
     *         identifier is unknown to the graph.
     */
    KinematicState BottomYInteractionGraph::getKinematicState(size_t tracklet_id) const
    {
        const auto it = records_.find(tracklet_id);
        if (it == records_.end()) return KinematicState::Cruising;
        return it->second.kinematic_state;
    }

} // namespace SushiTrack
