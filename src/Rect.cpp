/**
 * @file      Rect.cpp
 * @brief     Implementation of the Rect class and geometric utility functions.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      28.04.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */

#include "SushiTrack/Rect.h"
#include <algorithm>

namespace SushiTrack
{
    /**
     * @brief Constructs a Rect instance from top-left coordinates and dimensions.
     * 
     * @param x Horizontal coordinate of the top-left corner.
     * @param y Vertical coordinate of the top-left corner.
     * @param width Horizontal dimension.
     * @param height Vertical dimension.
     */
    template <typename T>
    Rect<T>::Rect(const T &x, const T &y, const T &width, const T &height) :
        tlwh({x, y, width, height})
    { }

    /**
     * @brief Destroys the Rect instance.
     */
    template <typename T>
    Rect<T>::~Rect()
    { }

    /**
     * @brief Retrieves the top-left X coordinate.
     * @return Constant reference to the X coordinate.
     */
    template <typename T>
    const T& Rect<T>::x() const { return tlwh[0]; }

    /**
     * @brief Retrieves the top-left Y coordinate.
     * @return Constant reference to the Y coordinate.
     */
    template <typename T>
    const T& Rect<T>::y() const { return tlwh[1]; }

    /**
     * @brief Retrieves the width dimension.
     * @return Constant reference to the width.
     */
    template <typename T>
    const T& Rect<T>::width() const { return tlwh[2]; }

    /**
     * @brief Retrieves the height dimension.
     * @return Constant reference to the height.
     */
    template <typename T>
    const T& Rect<T>::height() const { return tlwh[3]; }

    /**
     * @brief Retrieves a mutable reference to the top-left X coordinate.
     * @return Reference to the X coordinate.
     */
    template <typename T>
    T& Rect<T>::x() { return tlwh[0]; }

    /**
     * @brief Retrieves a mutable reference to the top-left Y coordinate.
     * @return Reference to the Y coordinate.
     */
    template <typename T>
    T& Rect<T>::y() { return tlwh[1]; }

    /**
     * @brief Retrieves a mutable reference to the width dimension.
     * @return Reference to the width.
     */
    template <typename T>
    T& Rect<T>::width() { return tlwh[2]; }

    /**
     * @brief Retrieves a mutable reference to the height dimension.
     * @return Reference to the height.
     */
    template <typename T>
    T& Rect<T>::height() { return tlwh[3]; }

    /**
     * @brief Formats the rectangle as [top, left, bottom, right].
     * 
     * @return Vector containing TLBR coordinates.
     */
    template <typename T>
    Tlbr<T> Rect<T>::getTlbr() const
    {
        return {
            tlwh[0],
            tlwh[1],
            tlwh[0] + tlwh[2],
            tlwh[1] + tlwh[3],
        };
    }

    /**
     * @brief Formats the rectangle as [center_x, center_y, width, height].
     * 
     * @return Vector containing XYWH coordinates.
     */
    template <typename T>
    Xywh<T> Rect<T>::getXywh() const
    {
        return {
            tlwh[0] + tlwh[2] / 2,
            tlwh[1] + tlwh[3] / 2,
            tlwh[2],
            tlwh[3],
        };
    }

    /**
     * @brief Retrieves the rectangle dimensions in [top-left x, top-left y, width, height] format.
     * 
     * @return Vector containing TLWH coordinates.
     */
    template <typename T>
    Tlwh<T> Rect<T>::getTlwh() const
    {
        return tlwh;
    }

    /**
     * @brief Creates a Rect instance from TLBR coordinates.
     * 
     * @param tlbr Vector containing [top, left, bottom, right].
     * @return Initialized Rect instance.
     */
    template<typename T>
    Rect<T> generate_rect_by_tlbr(const Tlbr<T>& tlbr)
    {
        return Rect<T>(tlbr[0], tlbr[1], tlbr[2] - tlbr[0], tlbr[3] - tlbr[1]);
    }

    /**
     * @brief Creates a Rect instance from center-based coordinates and dimensions.
     * 
     * @param xywh Vector containing [center_x, center_y, width, height].
     * @return Initialized Rect instance.
     */
    template<typename T>
    Rect<T> generate_rect_by_xywh(const Xywh<T>& xywh)
    {
        const auto width = xywh[2];
        return Rect<T>(xywh[0] - width / 2, xywh[1] - xywh[3] / 2, width, xywh[3]);
    }

    template class Rect<int>;
    template class Rect<float>;

    template Rect<int> generate_rect_by_tlbr<int>(const Tlbr<int>&);
    template Rect<float> generate_rect_by_tlbr<float>(const Tlbr<float>&);

    template Rect<int> generate_rect_by_xywh<int>(const Xywh<int>&);
    template Rect<float> generate_rect_by_xywh<float>(const Xywh<float>&);

} // namespace SushiTrack
