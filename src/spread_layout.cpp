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
#include <optional>

namespace geom = mir::geometry;

namespace
{
/// The cell that a single window is fitted into for a candidate row count.
struct Cell
{
    float width;
    float height;
};

/// Largest uniform scale that fits \p window into \p cell without enlarging it.
float scale_into(Cell const& cell, geom::SizeF const& window)
{
    return std::min({ 1.f,
        cell.width / std::max(window.width.as_value(), 1.f),
        cell.height / std::max(window.height.as_value(), 1.f) });
}

/// The cell every window gets when \p n windows are laid out over \p rows rows.
/// Empty if the rows cannot fit inside \p usable at all.
std::optional<Cell> cell_for(geom::SizeF const& usable, size_t n, size_t rows, float gap)
{
    size_t const columns = (n + rows - 1) / rows;
    Cell const cell {
        (usable.width.as_value() - static_cast<float>(columns - 1) * gap) / static_cast<float>(columns),
        (usable.height.as_value() - static_cast<float>(rows - 1) * gap) / static_cast<float>(rows)
    };
    if (cell.width < 1.f || cell.height < 1.f)
        return std::nullopt;

    return cell;
}

/// Picks the row count that leaves the windows covering the most screen area.
/// Ties break toward fewer rows, which is what makes five 16:9 windows land as
/// three-then-two rather than as three rows of two.
size_t best_row_count(geom::SizeF const& usable, std::vector<geom::SizeF> const& windows, float gap)
{
    size_t best_rows = 1;
    float best_score = -1.f;
    for (size_t rows = 1; rows <= windows.size(); ++rows)
    {
        auto const cell = cell_for(usable, windows.size(), rows, gap);
        if (!cell)
            continue;

        float score = 0.f;
        for (auto const& window : windows)
        {
            float const scale = scale_into(*cell, window);
            score += scale * scale * window.width.as_value() * window.height.as_value();
        }

        if (score > best_score)
        {
            best_score = score;
            best_rows = rows;
        }
    }

    return best_rows;
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

    size_t const rows = best_row_count(usable, sizes, fgap);
    auto const counts = distribute(n, rows);

    // Every candidate row count that reached this point had a cell, and the
    // winner is one of them.
    auto const cell = *cell_for(usable, n, rows, fgap);

    // Scale each window into the cell, preserving its aspect ratio.
    std::vector<geom::SizeF> scaled;
    scaled.reserve(n);
    for (auto const& size : sizes)
    {
        float const scale = scale_into(cell, size);
        scaled.emplace_back(size.width.as_value() * scale, size.height.as_value() * scale);
    }

    // Rows are as tall as their tallest window rather than a fixed cell height,
    // and the whole stack is centered vertically, so wide windows do not leave
    // dead space above and below.
    std::vector<float> row_heights(rows, 0.f);
    float total_height = static_cast<float>(rows - 1) * fgap;
    for (size_t row = 0, i = 0; row < rows; i += counts[row], ++row)
    {
        for (size_t k = 0; k < counts[row]; ++k)
            row_heights[row] = std::max(row_heights[row], scaled[i + k].height.as_value());
        total_height += row_heights[row];
    }

    std::vector<geom::Rectangle> result;
    result.reserve(n);
    float y = usable_top_left.y.as_value() + (usable.height.as_value() - total_height) / 2.f;
    for (size_t row = 0, i = 0; row < rows; i += counts[row], ++row)
    {
        float row_width = static_cast<float>(counts[row] - 1) * fgap;
        for (size_t k = 0; k < counts[row]; ++k)
            row_width += scaled[i + k].width.as_value();

        float x = usable_top_left.x.as_value() + (usable.width.as_value() - row_width) / 2.f;
        for (size_t k = 0; k < counts[row]; ++k)
        {
            auto const& size = scaled[i + k];
            geom::PointF const top_left { x, y + (row_heights[row] - size.height.as_value()) / 2.f };
            result.push_back(to_int_rect(top_left, size, bounds));
            x += size.width.as_value() + fgap;
        }

        y += row_heights[row] + fgap;
    }

    return result;
}
