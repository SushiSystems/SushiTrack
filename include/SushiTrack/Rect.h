/**
 * @file      Rect.h
 * @brief     Geometric bounding box representation and utility functions.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      28.04.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */

#pragma once

#include "Eigen/Dense"

namespace SushiTrack
{
    /** @brief Row-major matrix for [Top-Left X, Top-Left Y, Width, Height] representation. */
    template<typename T>
    using Tlwh = Eigen::Matrix<T, 1, 4, Eigen::RowMajor>;

    /** @brief Row-major matrix for [X1, Y1, X2, Y2] representation. */
    template<typename T>
    using Tlbr = Eigen::Matrix<T, 1, 4, Eigen::RowMajor>;

    /** @brief Row-major matrix for [Center X, Center Y, Width, Height] representation. */
    template<typename T>
    using Xywh = Eigen::Matrix<T, 1, 4, Eigen::RowMajor>;

    /**
     * @brief Template class for bounding box representation and geometric operations.
     * 
     * Stores dimensions in TLWH format and provides conversion utilities 
     * for alternative coordinate systems and intersection-over-union calculations.
     */
    template<typename T>
    class alignas(16) Rect
    {
        public:
        EIGEN_MAKE_ALIGNED_OPERATOR_NEW
        Tlwh<T> tlwh; ///< Contiguous storage for [Top-Left X, Top-Left Y, Width, Height] coordinates.

        Rect() = default;
        
        /**
         * @brief Initializes a Rect instance from coordinates.
         * 
         * @param x Top-left horizontal coordinate.
         * @param y Top-left vertical coordinate.
         * @param width Horizontal dimension.
         * @param height Vertical dimension.
         */
        Rect(const T &x, const T &y, const T &width, const T &height);

        /**
         * @brief Destroys the Rect instance.
         */
        ~Rect();

        const T &x() const;      ///< Returns Top-Left X coordinate.
        const T &y() const;      ///< Returns Top-Left Y coordinate.
        const T &width() const;  ///< Returns width dimension.
        const T &height() const; ///< Returns height dimension.

        T &x();                  ///< Returns reference to Top-Left X coordinate for modification.
        T &y();                  ///< Returns reference to Top-Left Y coordinate for modification.
        T &width();              ///< Returns reference to width dimension for modification.
        T &height();             ///< Returns reference to height dimension for modification.

        /**
         * @brief Converts dimensions to TLBR [x1, y1, x2, y2] format.
         * 
         * @return Vector containing top-left and bottom-right coordinates.
         */
        Tlbr<T> getTlbr() const;

        /**
         * @brief Converts dimensions to XYWH [cx, cy, w, h] format.
         * 
         * @return Vector containing centroid coordinates and dimensions.
         */
        Xywh<T> getXywh() const;

    };

    /**
     * @brief Instantiates a Rect object from TLBR coordinates.
     * 
     * @param tlbr Vector containing [top, left, bottom, right] coordinates.
     * @return Initialized Rect instance.
     */
    template<typename T>
    Rect<T> generate_rect_by_tlbr(const Tlbr<T>& tlbr);

    /**
     * @brief Instantiates a Rect object from XYWH coordinates.
     * 
     * @param xywh Vector containing [center_x, center_y, width, height] coordinates.
     * @return Initialized Rect instance.
     */
    template<typename T>
    Rect<T> generate_rect_by_xywh(const Xywh<T>& xywh);

    /** @brief Aligned vector for Rect instances. */
    template<typename T>
    using RectVector = std::vector<Rect<T>, Eigen::aligned_allocator<Rect<T>>>;

} // namespace SushiTrack
