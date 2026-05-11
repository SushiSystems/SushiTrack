/**
 * @file      IInteractionGraph.h
 * @brief     Strategy interface for scene-level tracklet interaction modelling.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      11.05.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */

#pragma once

#include "SushiTrack/TrackFSM.h"

#include <cstddef>
#include <memory>
#include <vector>

namespace SushiTrack
{
    class Tracklet;

    /**
     * @brief Strategy interface for scene-level interaction analysis.
     *
     * Implementations consume the predicted geometric state of a collection
     * of tracklets and produce, for each tracklet, the identifier of the
     * neighbouring tracklet that most likely occludes it from the camera, if
     * any. The decision criterion (depth ordering heuristic, overlap
     * threshold, additional cues) is encapsulated by the concrete strategy
     * so that the tracker pipeline depends only on this abstraction.
     *
     * Implementations are expected to be stateless across frames; per-frame
     * caches may be retained internally for performance but must be rebuilt
     * on each call to build().
     */
    class IInteractionGraph
    {
        public:
            using TrackletPtr = std::shared_ptr<Tracklet>;

            /**
             * @brief Sentinel returned by getFrontOccluder when no occluder is
             *        identified for the queried tracklet.
             */
            static constexpr int kNoOccluder = -1;

            /**
             * @brief Virtual destructor for the interface.
             */
            virtual ~IInteractionGraph() = default;

            /**
             * @brief Builds the interaction graph for a frame.
             *
             * Implementations evaluate pairwise interactions between all input
             * tracklets and persist the results internally for subsequent
             * queries. The set of tracklets passed to this method defines the
             * universe considered by subsequent getFrontOccluder calls; any
             * tracklet not present in the input collection is treated as
             * having no occluder.
             *
             * @param tracklets Collection of tracklets to analyse. Their
             *                  rectangles are assumed to reflect the predicted
             *                  state for the current frame.
             */
            virtual void build(const std::vector<TrackletPtr>& tracklets) = 0;

            /**
             * @brief Retrieves the identifier of the tracklet that is most
             *        likely occluding the queried tracklet.
             *
             * @param tracklet_id Track identifier of the queried tracklet.
             * @return Identifier of the front-most overlapping neighbour, or
             *         kNoOccluder if no qualifying neighbour exists.
             */
            virtual int getFrontOccluder(size_t tracklet_id) const = 0;

            /**
             * @brief Checks whether a specific tracklet identifier is present
             *        in the interaction graph and currently tracked.
             *
             * Used by lost-recovery policies to determine whether a stored
             * occluder snapshot still refers to a tracklet that is active in
             * the scene.
             *
             * @param tracklet_id Track identifier to test.
             * @return True if the identifier corresponds to a tracklet that
             *         participated in the latest build call.
             */
            virtual bool contains(size_t tracklet_id) const = 0;

            /**
             * @brief Retrieves the bounding box overlap between the occluder
             *        snapshot referenced by @p occluder_id and an arbitrary
             *        candidate rectangle.
             *
             * Implementations return a value in the closed interval [0, 1].
             * The value zero is returned when the occluder is not part of the
             * graph. Used during lost-recovery to decide whether a candidate
             * detection coincides with the predicted occluder location.
             *
             * @param occluder_id Identifier of the occluder tracklet.
             * @param cand_x Top-left horizontal coordinate of the candidate.
             * @param cand_y Top-left vertical coordinate of the candidate.
             * @param cand_w Width of the candidate.
             * @param cand_h Height of the candidate.
             * @return Intersection over union in the closed range [0, 1].
             */
            virtual float occluderOverlap(size_t occluder_id,
                                          float cand_x, float cand_y,
                                          float cand_w, float cand_h) const = 0;

            /**
             * @brief Retrieves the kinematic state recorded at build time
             *        for a given tracklet identifier.
             *
             * Exposes the motion regime captured during the most recent
             * build pass so that downstream consumers (notably the
             * occluder-aware penalty helper) can adapt their decisions to
             * the occluder's motion profile without having to resolve the
             * underlying Tracklet pointer again. Returns Cruising as a
             * neutral fallback when the identifier is unknown to the
             * graph, which leaves identity-valued multipliers in place
             * for any policy that consumes this value.
             *
             * @param tracklet_id Track identifier to query.
             * @return KinematicState recorded for the tracklet, or
             *         Cruising when the identifier is not present.
             */
            virtual KinematicState getKinematicState(size_t tracklet_id) const = 0;
    };

} // namespace SushiTrack
