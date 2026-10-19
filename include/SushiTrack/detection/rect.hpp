/**************************************************************************/
/* rect.hpp                                                               */
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
/*                                                                        */
/* Holds code ported from ByteTrack-cpp (MIT), see NOTICE.md              */
/* Copyright (c) 2022 Vertical Beach (lp6m, medalotte)                    */
/* Copyright (c) 2021 Yifu Zhang                                          */
/**************************************************************************/

/**
 * @file rect.hpp
 * @brief Geometric bounding box representation and intersection utilities.
 * @author Mustafa Garip
 */

#pragma once

#include <algorithm>
#include <vector>

#include <Eigen/Dense>

namespace SushiTrack
{
    /** @brief Row-major matrix for [Top-Left X, Top-Left Y, Width, Height] representation. */
    template <typename T>
    using Tlwh = Eigen::Matrix<T, 1, 4, Eigen::RowMajor>;

    /** @brief Row-major matrix for [X1, Y1, X2, Y2] representation. */
    template <typename T>
    using Tlbr = Eigen::Matrix<T, 1, 4, Eigen::RowMajor>;

    /** @brief Row-major matrix for [Center X, Center Y, Width, Height] representation. */
    template <typename T>
    using Xywh = Eigen::Matrix<T, 1, 4, Eigen::RowMajor>;

    /**
     * @brief Bounding box representation and geometric operations.
     */
    template <typename T>
    class alignas(16) Rect final
    {
        public:
            EIGEN_MAKE_ALIGNED_OPERATOR_NEW
            Tlwh<T>
                tlwh; ///< Contiguous storage for [Top-Left X, Top-Left Y, Width, Height] coordinates.

            Rect() = default;

            /**
             * @brief Initializes a Rect instance from coordinates.
             * @param x Top-left horizontal coordinate.
             * @param y Top-left vertical coordinate.
             * @param width Horizontal dimension.
             * @param height Vertical dimension.
             */
            Rect(const T& x, const T& y, const T& width, const T& height);

            /**
             * @brief Destroys the Rect instance.
             */
            ~Rect();

            [[nodiscard]] const T& x() const;      ///< Returns Top-Left X coordinate.
            [[nodiscard]] const T& y() const;      ///< Returns Top-Left Y coordinate.
            [[nodiscard]] const T& width() const;  ///< Returns width dimension.
            [[nodiscard]] const T& height() const; ///< Returns height dimension.

            [[nodiscard]] T& x(); ///< Returns reference to Top-Left X coordinate for modification.
            [[nodiscard]] T& y(); ///< Returns reference to Top-Left Y coordinate for modification.
            [[nodiscard]] T& width();  ///< Returns reference to width dimension for modification.
            [[nodiscard]] T& height(); ///< Returns reference to height dimension for modification.

            /**
             * @brief Converts dimensions to TLBR [x1, y1, x2, y2] format.
             * @return Vector containing top-left and bottom-right coordinates.
             */
            [[nodiscard]] Tlbr<T> get_tlbr() const;

            /**
             * @brief Converts dimensions to XYWH [cx, cy, w, h] format.
             * @return Vector containing centroid coordinates and dimensions.
             */
            [[nodiscard]] Xywh<T> get_xywh() const;

            /**
             * @brief Retrieves dimensions in TLWH [x, y, w, h] format.
             * @return Vector containing top-left coordinates and dimensions.
             */
            [[nodiscard]] Tlwh<T> get_tlwh() const;
    };

    /**
     * @brief Instantiates a Rect object from TLBR coordinates.
     * @param tlbr Vector containing [top, left, bottom, right] coordinates.
     * @return Initialized Rect instance.
     */
    template <typename T>
    Rect<T> generate_rect_by_tlbr(const Tlbr<T>& tlbr);

    /**
     * @brief Instantiates a Rect object from XYWH coordinates.
     * @param xywh Vector containing [center_x, center_y, width, height] coordinates.
     * @return Initialized Rect instance.
     */
    template <typename T>
    Rect<T> generate_rect_by_xywh(const Xywh<T>& xywh);

    /**
     * @brief Plain Intersection-over-Union of two boxes in [0, 1]; zero when disjoint.
     */
    template <typename T>
    inline T iou(const Rect<T>& a, const Rect<T>& b)
    {
        const T iw = std::min(a.x() + a.width(), b.x() + b.width()) - std::max(a.x(), b.x());
        if (iw <= T(0))
            return T(0);
        const T ih = std::min(a.y() + a.height(), b.y() + b.height()) - std::max(a.y(), b.y());
        if (ih <= T(0))
            return T(0);

        const T inter = iw * ih;
        const T ua = a.width() * a.height() + b.width() * b.height() - inter;
        return inter / std::max(static_cast<T>(1e-6), ua);
    }

    /** @brief Aligned vector for Rect instances. */
    template <typename T>
    using RectVector = std::vector<Rect<T>, Eigen::aligned_allocator<Rect<T>>>;

} // namespace SushiTrack
