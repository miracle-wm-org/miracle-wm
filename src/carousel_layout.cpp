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

#include "carousel_layout.h"

#include <algorithm>
#include <cmath>

namespace geom = mir::geometry;

namespace
{
float clamp01(float value)
{
    return std::clamp(value, 0.f, 1.f);
}
}

miracle::carousel_layout::Placement miracle::carousel_layout::lerp(
    Placement const& from, Placement const& to, float p)
{
    auto const mix = [p](float a, float b)
    {
        return a + (b - a) * p;
    };
    return Placement {
        .x = mix(from.x, to.x),
        .y = mix(from.y, to.y),
        .width = mix(from.width, to.width),
        .height = mix(from.height, to.height),
        .opacity = mix(from.opacity, to.opacity)
    };
}

bool miracle::carousel_layout::contains(Placement const& placement, float x, float y)
{
    return x >= placement.x
        && y >= placement.y
        && x < placement.x + placement.width
        && y < placement.y + placement.height;
}

miracle::carousel_layout::Placement miracle::carousel_layout::fit(
    geom::Rectangle const& source,
    Placement const& tile,
    geom::Rectangle const& rect)
{
    auto const source_width = static_cast<float>(std::max(source.size.width.as_value(), 1));
    auto const source_height = static_cast<float>(std::max(source.size.height.as_value(), 1));

    // The tile was fitted to the source with a single uniform scale, so one
    // ratio describes both axes; averaging the two guards against a tile whose
    // aspect drifted by a pixel of rounding.
    auto const scale = (tile.width / source_width + tile.height / source_height) / 2.f;

    auto const offset_x = static_cast<float>(rect.top_left.x.as_value() - source.top_left.x.as_value());
    auto const offset_y = static_cast<float>(rect.top_left.y.as_value() - source.top_left.y.as_value());

    return Placement {
        .x = tile.x + offset_x * scale,
        .y = tile.y + offset_y * scale,
        .width = static_cast<float>(std::max(rect.size.width.as_value(), 1)) * scale,
        .height = static_cast<float>(std::max(rect.size.height.as_value(), 1)) * scale,
        .opacity = tile.opacity
    };
}

std::vector<miracle::carousel_layout::Placement> miracle::carousel_layout::compute(
    geom::Rectangle const& bounds,
    std::vector<geom::Rectangle> const& windows,
    float position,
    Options const& options)
{
    if (windows.empty())
        return {};

    auto const bounds_width = static_cast<float>(std::max(bounds.size.width.as_value(), 1));
    auto const bounds_height = static_cast<float>(std::max(bounds.size.height.as_value(), 1));
    auto const center_x = static_cast<float>(bounds.top_left.x.as_value()) + bounds_width / 2.f;
    auto const center_y = static_cast<float>(bounds.top_left.y.as_value()) + bounds_height / 2.f;

    auto const box_width = options.center_width_fraction * bounds_width;
    auto const box_height = options.center_height_fraction * bounds_height;

    // Pass 1: the size every window is actually drawn at. Spacing is derived
    // from these rather than from the slot boxes they were fitted into, because
    // a window rarely fills its box: max_scale or the box's height usually
    // binds first, and spacing by the box would leave that slack as dead air.
    std::vector<float> widths(windows.size());
    std::vector<float> heights(windows.size());
    for (size_t i = 0; i < windows.size(); ++i)
    {
        auto const distance = static_cast<float>(i) - position;
        auto const falloff = std::pow(options.side_scale, std::abs(distance));

        auto const window_width = static_cast<float>(std::max(windows[i].size.width.as_value(), 1));
        auto const window_height = static_cast<float>(std::max(windows[i].size.height.as_value(), 1));

        // A single uniform scale keeps the aspect ratio intact.
        auto const scale = std::min({ box_width * falloff / window_width,
            box_height * falloff / window_height,
            options.max_scale });
        widths[i] = window_width * scale;
        heights[i] = window_height * scale;
    }

    // Pass 2: chain the slots up edge to edge, so neighbours sit exactly the
    // gap apart. Neighbours are the tightest pair in the strip, so at a spacing
    // of one that also makes overlap impossible anywhere else.
    std::vector<float> offsets(windows.size(), 0.f);
    for (size_t i = 1; i < windows.size(); ++i)
        offsets[i] = offsets[i - 1] + (widths[i - 1] + widths[i]) / 2.f * options.spacing + options.gap;

    // The centered slot is at a fractional index, so the point that gets pinned
    // to the middle of the bounds is interpolated between the two offsets it
    // lies between. Widths vary continuously with position and so, therefore,
    // do the offsets: the strip slides rather than jumping between slots.
    auto const clamped = std::clamp(position, 0.f, static_cast<float>(windows.size() - 1));
    auto const lower = static_cast<size_t>(std::floor(clamped));
    auto const upper = std::min(lower + 1, windows.size() - 1);
    auto const anchor = offsets[lower]
        + (offsets[upper] - offsets[lower]) * (clamped - static_cast<float>(lower));

    std::vector<Placement> result;
    result.reserve(windows.size());
    for (size_t i = 0; i < windows.size(); ++i)
    {
        auto const distance = static_cast<float>(i) - position;
        result.push_back(Placement {
            .x = center_x + offsets[i] - anchor - widths[i] / 2.f,
            .y = center_y - heights[i] / 2.f,
            .width = widths[i],
            .height = heights[i],
            .opacity = options.dim + (1.f - options.dim) * clamp01(1.f - std::abs(distance)) });
    }

    return result;
}
