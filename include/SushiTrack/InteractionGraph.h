/**
 * @file      InteractionGraph.h
 * @brief     Bottom-Y depth-ordering implementation of IInteractionGraph.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      11.05.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */

#pragma once

#include "SushiTrack/IInteractionGraph.h"
#include "SushiTrack/TrackFSM.h"

#include <cstddef>
#include <unordered_map>
#include <vector>

namespace SushiTrack
{
    /**
     * @brief Pairwise interaction graph that orders tracklets by bottom-Y.
     *
     * Concrete implementation of IInteractionGraph that determines which of
     * two overlapping tracklets is closer to the camera by comparing the
     * bottom edge of their predicted bounding boxes. The convention assumes
     * a ground-plane scene with a roughly horizontal camera, in which an
     * object whose foot point projects lower on the image is closer to the
     * camera. The heuristic is appropriate for street-level deployments
     * with vehicles, pallets and pedestrians; it is not suitable for
     * overhead views.
     *
     * The bottom-Y comparison is augmented with a kinematic tiebreaker:
     * when two overlapping tracklets have near-equal foot points, the
     * recorded kinematic state is consulted to decide which is in front.
     * Stationary and Maneuvering tracks are treated as closer to the
     * camera because their velocity profile reflects either a stable
     * foreground object or active local motion, whereas Cruising and
     * Coasting tracks are treated as farther away. This breaks the
     * indeterminacy that the pure bottom-Y heuristic exhibits when two
     * tracklets walk side-by-side or are vertically aligned.
     *
     * The overlap metric used to register and query interactions is
     * Distance-IoU (DIoU). DIoU subtracts the squared centroid distance
     * normalised by the diagonal of the smallest enclosing rectangle,
     * which preserves the activation contract of plain IoU on overlapping
     * boxes while remaining strictly negative for boxes that are far
     * apart but otherwise pass a bounding-area check. The activation
     * threshold is interpreted in the same direction as IoU: a value at
     * or above the configured threshold registers an interaction.
     *
     * Pairs whose DIoU similarity is below the configured threshold are
     * ignored. For each tracklet only the single highest-similarity
     * front-neighbour is retained, which keeps downstream policy
     * decisions simple and avoids ambiguity in dense scenes.
     */
    class BottomYInteractionGraph final : public IInteractionGraph
    {
        public:
            /**
             * @brief Initializes the graph with the IoU activation threshold.
             *
             * @param iou_thresh Minimum predicted-box IoU required to record
             *                   an interaction between two tracklets.
             */
            explicit BottomYInteractionGraph(float iou_thresh);

            /**
             * @brief Destroys the BottomYInteractionGraph instance.
             */
            ~BottomYInteractionGraph() override = default;

            /**
             * @brief Builds the per-frame interaction graph.
             *
             * Computes the pairwise IoU of the predicted bounding boxes,
             * filters out pairs below the activation threshold, and records
             * for each tracklet the identifier of its best front-neighbour
             * as determined by the bottom-Y depth heuristic.
             *
             * @param tracklets Collection of tracklets to analyse.
             */
            void build(const std::vector<TrackletPtr>& tracklets) override;

            /**
             * @brief Retrieves the identifier of the front-most occluder.
             *
             * @param tracklet_id Track identifier of the queried tracklet.
             * @return Identifier of the closest front-neighbour, or
             *         kNoOccluder if no qualifying neighbour was recorded.
             */
            int getFrontOccluder(size_t tracklet_id) const override;

            /**
             * @brief Tests whether the identifier participates in the graph.
             *
             * @param tracklet_id Track identifier to test.
             * @return True when the identifier corresponds to a tracklet
             *         passed to the most recent build call.
             */
            bool contains(size_t tracklet_id) const override;

            /**
             * @brief Computes the IoU between a stored occluder rectangle
             *        and a candidate rectangle.
             *
             * @param occluder_id Identifier of the occluder tracklet.
             * @param cand_x Top-left horizontal coordinate of the candidate.
             * @param cand_y Top-left vertical coordinate of the candidate.
             * @param cand_w Width of the candidate.
             * @param cand_h Height of the candidate.
             * @return Intersection over union in the closed range [0, 1], or
             *         zero when the occluder is unknown to the graph.
             */
            float occluderOverlap(size_t occluder_id,
                                  float cand_x, float cand_y,
                                  float cand_w, float cand_h) const override;

            /**
             * @brief Retrieves the recorded kinematic state for a tracklet.
             *
             * @param tracklet_id Track identifier to query.
             * @return KinematicState captured at build time, or Cruising
             *         when the identifier is unknown to the graph.
             */
            KinematicState getKinematicState(size_t tracklet_id) const override;

        private:
            /**
             * @brief Per-tracklet record stored after a build call.
             */
            struct Record
            {
                float x = 0.0f;       ///< Top-left X of the predicted bounding box.
                float y = 0.0f;       ///< Top-left Y of the predicted bounding box.
                float w = 0.0f;       ///< Predicted bounding-box width.
                float h = 0.0f;       ///< Predicted bounding-box height.
                /**
                 * @brief Kinematic regime captured at build time.
                 *
                 * Stored so that downstream consumers can consult the
                 * occluder's motion profile without having to resolve the
                 * underlying Tracklet pointer again.
                 */
                KinematicState kinematic_state = KinematicState::Cruising;
                int occluder_id = kNoOccluder; ///< Best front-neighbour identifier.
            };

            float iou_thresh_;                              ///< Minimum IoU to register a pair.
            std::unordered_map<size_t, Record> records_;    ///< Per-tracklet records indexed by track_id.
    };

} // namespace SushiTrack
