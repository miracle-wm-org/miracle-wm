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

#include "spread_layout.h"

#include <algorithm>
#include <cmath>

namespace geom = mir::geometry;

namespace
{
/// The largest a window is ever drawn in the spread, as a fraction of its real
/// size. The spread is a world overview rather than a rearrangement, so even a
/// window with a cell to spare is shrunk: it makes the view read as a step back
/// from the desktop, and it leaves room between the tiles for picking one.
constexpr float MAX_SCALE = 0.75f;

/// The cell that a single window is fitted into.
struct Cell
{
    float width;
    float height;
};

/// Largest uniform scale that fits \p window into \p cell, never exceeding
/// [MAX_SCALE].
float scale_into(Cell const& cell, geom::SizeF const& window)
{
    return std::min({ MAX_SCALE,
        cell.width / std::max(window.width.as_value(), 1.f),
        cell.height / std::max(window.height.as_value(), 1.f) });
}

/// The cell every window gets in a \p columns by \p rows grid. Degenerate cells
/// are clamped to a pixel so that a grid too dense for its bounds still produces
/// placements instead of inverted rectangles.
Cell cell_for(geom::SizeF const& usable, size_t columns, size_t rows, float gap)
{
    return Cell {
        std::max((usable.width.as_value() - static_cast<float>(columns - 1) * gap) / static_cast<float>(columns), 1.f),
        std::max((usable.height.as_value() - static_cast<float>(rows - 1) * gap) / static_cast<float>(rows), 1.f)
    };
}

/// The number of columns in the balanced grid that holds \p n windows. Squaring
/// the grid rather than choosing the row count that covers the most area is what
/// spreads the windows over the whole output: three side-by-side windows become
/// two on top and one beneath instead of staying in one row at nearly full size.
size_t column_count(size_t n)
{
    return static_cast<size_t>(std::ceil(std::sqrt(static_cast<double>(n))));
}

/// Distributes \p n windows over \p rows rows, handing the remainder to the
/// earlier rows so that five windows over two rows is three then two.
std::vector<size_t> distribute(size_t n, size_t rows)
{
    std::vector<size_t> counts(rows, n / rows);
    for (size_t i = 0; i < n % rows; ++i)
        counts[i]++;
    return counts;
}

geom::Rectangle to_int_rect(geom::PointF const& top_left, geom::SizeF const& size, geom::Rectangle const& bounds)
{
    geom::Size const rounded {
        std::max(static_cast<int>(std::round(size.width.as_value())), 1),
        std::max(static_cast<int>(std::round(size.height.as_value())), 1)
    };

    // Rounding can nudge a tile a pixel past the edge: keep it inside.
    return {
        geom::Point {
                     std::clamp(static_cast<int>(std::round(top_left.x.as_value())),
                     bounds.top_left.x.as_value(),
                     std::max(bounds.right().as_value() - rounded.width.as_value(), bounds.top_left.x.as_value())),
                     std::clamp(static_cast<int>(std::round(top_left.y.as_value())),
                     bounds.top_left.y.as_value(),
                     std::max(bounds.bottom().as_value() - rounded.height.as_value(), bounds.top_left.y.as_value())) },
        rounded
    };
}
}

std::vector<geom::Rectangle> miracle::spread_layout::compute(
    geom::Rectangle const& bounds,
    std::vector<geom::Rectangle> const& windows,
    int gap)
{
    size_t const n = windows.size();
    if (n == 0)
        return {};

    float const fgap = static_cast<float>(gap);
    geom::PointF const usable_top_left {
        static_cast<float>(bounds.top_left.x.as_value()) + fgap,
        static_cast<float>(bounds.top_left.y.as_value()) + fgap
    };
    geom::SizeF const usable {
        std::max(static_cast<float>(bounds.size.width.as_value()) - 2.f * fgap, 1.f),
        std::max(static_cast<float>(bounds.size.height.as_value()) - 2.f * fgap, 1.f)
    };

    std::vector<geom::SizeF> sizes;
    sizes.reserve(n);
    for (auto const& w : windows)
        sizes.emplace_back(static_cast<float>(w.size.width.as_value()), static_cast<float>(w.size.height.as_value()));

    size_t const columns = column_count(n);
    size_t const rows = (n + columns - 1) / columns;
    auto const counts = distribute(n, rows);
    auto const cell = cell_for(usable, columns, rows, fgap);

    // Scale each window into the cell, preserving its aspect ratio.
    std::vector<geom::SizeF> scaled;
    scaled.reserve(n);
    for (auto const& size : sizes)
    {
        float const scale = scale_into(cell, size);
        scaled.emplace_back(size.width.as_value() * scale, size.height.as_value() * scale);
    }

    // Every window sits at the center of its own cell, and the grid of cells is
    // centered in the bounds. Spacing the tiles by the cell rather than packing
    // them against one another is what spreads them across the output; a short
    // final row is centered, so three windows read as two above one.
    float const grid_height = static_cast<float>(rows) * cell.height + static_cast<float>(rows - 1) * fgap;
    float const grid_top = usable_top_left.y.as_value() + (usable.height.as_value() - grid_height) / 2.f;

    std::vector<geom::Rectangle> result;
    result.reserve(n);
    for (size_t row = 0, i = 0; row < rows; i += counts[row], ++row)
    {
        float const row_width = static_cast<float>(counts[row]) * cell.width
            + static_cast<float>(counts[row] - 1) * fgap;
        float const row_left = usable_top_left.x.as_value() + (usable.width.as_value() - row_width) / 2.f;
        float const row_top = grid_top + static_cast<float>(row) * (cell.height + fgap);

        for (size_t k = 0; k < counts[row]; ++k)
        {
            auto const& size = scaled[i + k];
            geom::PointF const top_left {
                row_left + static_cast<float>(k) * (cell.width + fgap) + (cell.width - size.width.as_value()) / 2.f,
                row_top + (cell.height - size.height.as_value()) / 2.f
            };
            result.push_back(to_int_rect(top_left, size, bounds));
        }
    }

    return result;
}
