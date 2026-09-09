/**
Copyright (C) 2025  Matthew Kosarek

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

#ifndef MIRACLE_GEOMETRY_HELPERS_H
#define MIRACLE_GEOMETRY_HELPERS_H

#include <glm/glm.hpp>
#include <mir/geometry/point.h>
#include <mir/geometry/rectangle.h>
#include <mir/geometry/size.h>

namespace miracle
{
namespace geometry_helpers
{

    /**
     * @brief Converts a mir::geometry::Point to a glm::vec2 with float precision
     *
     * @param point The mir geometry point to convert
     * @return glm::vec2 The converted point as a 2D float vector
     */
    inline glm::vec2 to_glm(mir::geometry::Point const& point)
    {
        return glm::vec2(
            static_cast<float>(point.x.as_int()),
            static_cast<float>(point.y.as_int()));
    }

    /**
     * @brief Converts a mir::geometry::Point to a glm::ivec2 with integer precision
     *
     * @param point The mir geometry point to convert
     * @return glm::ivec2 The converted point as a 2D integer vector
     */
    inline glm::ivec2 to_glm_int(mir::geometry::Point const& point)
    {
        return glm::ivec2(point.x.as_int(), point.y.as_int());
    }

    /**
     * @brief Converts a mir::geometry::Size to a glm::vec2 with float precision
     *
     * @param size The mir geometry size to convert
     * @return glm::vec2 The converted size as a 2D float vector (width, height)
     */
    inline glm::vec2 to_glm(mir::geometry::Size const& size)
    {
        return glm::vec2(
            static_cast<float>(size.width.as_int()),
            static_cast<float>(size.height.as_int()));
    }

    /**
     * @brief Converts a mir::geometry::Size to a glm::ivec2 with integer precision
     *
     * @param size The mir geometry size to convert
     * @return glm::ivec2 The converted size as a 2D integer vector (width, height)
     */
    inline glm::ivec2 to_glm_int(mir::geometry::Size const& size)
    {
        return glm::ivec2(size.width.as_int(), size.height.as_int());
    }

    /**
     * @brief Structure to hold a rectangle as separate position and size vectors
     */
    struct RectangleAsVec2
    {
        glm::vec2 position; ///< Top-left position of the rectangle
        glm::vec2 size; ///< Width and height of the rectangle
    };

    /**
     * @brief Converts a mir::geometry::Rectangle to position and size vectors
     *
     * @param rect The mir geometry rectangle to convert
     * @return RectangleAsVec2 The converted rectangle as separate position and size vectors
     */
    inline RectangleAsVec2 to_glm(mir::geometry::Rectangle const& rect)
    {
        return {
            .position = to_glm(rect.top_left),
            .size = to_glm(rect.size)
        };
    }

    /**
     * @brief Converts a glm::vec2 to a mir::geometry::Point
     *
     * @param vec The 2D float vector to convert (x, y components)
     * @return mir::geometry::Point The converted point with integer coordinates
     */
    inline mir::geometry::Point from_glm(glm::vec2 const& vec)
    {
        return mir::geometry::Point {
            mir::geometry::X { static_cast<int>(vec.x) },
            mir::geometry::Y { static_cast<int>(vec.y) }
        };
    }

    /**
     * @brief Converts a glm::ivec2 to a mir::geometry::Point
     *
     * @param vec The 2D integer vector to convert (x, y components)
     * @return mir::geometry::Point The converted point
     */
    inline mir::geometry::Point from_glm(glm::ivec2 const& vec)
    {
        return mir::geometry::Point {
            mir::geometry::X { vec.x },
            mir::geometry::Y { vec.y }
        };
    }

    /**
     * @brief Converts a glm::vec2 to a mir::geometry::Size
     *
     * @param vec The 2D float vector to convert (width, height components)
     * @return mir::geometry::Size The converted size with integer dimensions
     */
    inline mir::geometry::Size size_from_glm(glm::vec2 const& vec)
    {
        return mir::geometry::Size {
            mir::geometry::Width { static_cast<int>(vec.x) },
            mir::geometry::Height { static_cast<int>(vec.y) }
        };
    }

    /**
     * @brief Converts a glm::ivec2 to a mir::geometry::Size
     *
     * @param vec The 2D integer vector to convert (width, height components)
     * @return mir::geometry::Size The converted size
     */
    inline mir::geometry::Size size_from_glm(glm::ivec2 const& vec)
    {
        return mir::geometry::Size {
            mir::geometry::Width { vec.x },
            mir::geometry::Height { vec.y }
        };
    }

    /**
     * @brief Converts position and size vectors back to a mir::geometry::Rectangle
     *
     * @param position The top-left position as a 2D float vector
     * @param size The width and height as a 2D float vector
     * @return mir::geometry::Rectangle The constructed rectangle
     */
    inline mir::geometry::Rectangle from_glm(glm::vec2 const& position, glm::vec2 const& size)
    {
        return mir::geometry::Rectangle {
            from_glm(position),
            size_from_glm(size)
        };
    }

    /**
     * @brief Convenience functions for common OpenGL operations that need individual components
     */
    namespace gl
    {

        /**
         * @brief Extracts the x coordinate from a point as a float
         * @param point The point to extract from
         * @return float The x coordinate for OpenGL operations
         */
        inline float x(mir::geometry::Point const& point) { return static_cast<float>(point.x.as_int()); }

        /**
         * @brief Extracts the y coordinate from a point as a float
         * @param point The point to extract from
         * @return float The y coordinate for OpenGL operations
         */
        inline float y(mir::geometry::Point const& point) { return static_cast<float>(point.y.as_int()); }

        /**
         * @brief Extracts the width from a size as a float
         * @param size The size to extract from
         * @return float The width for OpenGL operations
         */
        inline float width(mir::geometry::Size const& size) { return static_cast<float>(size.width.as_int()); }

        /**
         * @brief Extracts the height from a size as a float
         * @param size The size to extract from
         * @return float The height for OpenGL operations
         */
        inline float height(mir::geometry::Size const& size) { return static_cast<float>(size.height.as_int()); }

        /**
         * @brief Extracts the x coordinate from a point as an integer
         * @param point The point to extract from
         * @return int The x coordinate for OpenGL operations that require integers
         */
        inline int x_int(mir::geometry::Point const& point) { return point.x.as_int(); }

        /**
         * @brief Extracts the y coordinate from a point as an integer
         * @param point The point to extract from
         * @return int The y coordinate for OpenGL operations that require integers
         */
        inline int y_int(mir::geometry::Point const& point) { return point.y.as_int(); }

        /**
         * @brief Extracts the width from a size as an integer
         * @param size The size to extract from
         * @return int The width for OpenGL operations that require integers
         */
        inline int width_int(mir::geometry::Size const& size) { return size.width.as_int(); }

        /**
         * @brief Extracts the height from a size as an integer
         * @param size The size to extract from
         * @return int The height for OpenGL operations that require integers
         */
        inline int height_int(mir::geometry::Size const& size) { return size.height.as_int(); }

        /**
         * @brief Extracts the width using as_value() for texture operations
         * @param size The size to extract from
         * @return int The width value for texture creation and similar operations
         */
        inline int width_value(mir::geometry::Size const& size) { return size.width.as_value(); }

        /**
         * @brief Extracts the height using as_value() for texture operations
         * @param size The size to extract from
         * @return int The height value for texture creation and similar operations
         */
        inline int height_value(mir::geometry::Size const& size) { return size.height.as_value(); }

    }

} // namespace geometry_helpers
} // namespace miracle

#endif // MIRACLE_GEOMETRY_HELPERS_H