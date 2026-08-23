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
/// The signed number of slots that the window at \p index sits away from
/// \p position, taking the shorter way around a carousel of \p count windows.
///
/// The result lies in (-count/2, count/2], so a window that has fallen off one
/// end comes back around the other. Ties break towards the right, which keeps
/// the layout stable as [position] sweeps past the half-way mark.
float wrapped_distance(size_t index, float position, size_t count)
{
    auto const n = static_cast<float>(count);
    auto const half = n / 2.f;

    float d = std::fmod(static_cast<float>(index) - position, n);
    if (d < 0.f)
        d += n;

    return d > half ? d - n : d;
}

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

    // Neighbouring slots are always exactly one step apart, so spacing them by
    // the half-box of the center slot plus the half-box of its neighbour plus
    // the gap is what makes overlap impossible. That pair is the tightest one:
    // every slot further out is smaller still, and a window fitted into its box
    // is never wider than the box.
    auto const step = box_width * (1.f + options.side_scale) / 2.f + options.gap;

    std::vector<Placement> result;
    result.reserve(windows.size());
    for (size_t i = 0; i < windows.size(); ++i)
    {
        auto const distance = wrapped_distance(i, position, windows.size());
        auto const falloff = std::pow(options.side_scale, std::abs(distance));

        auto const window_width = static_cast<float>(std::max(windows[i].size.width.as_value(), 1));
        auto const window_height = static_cast<float>(std::max(windows[i].size.height.as_value(), 1));

        // A single uniform scale keeps the aspect ratio intact.
        auto const scale = std::min({ box_width * falloff / window_width,
            box_height * falloff / window_height,
            options.max_scale });
        auto const width = window_width * scale;
        auto const height = window_height * scale;

        result.push_back(Placement {
            .x = center_x + distance * step - width / 2.f,
            .y = center_y - height / 2.f,
            .width = width,
            .height = height,
            .opacity = options.dim + (1.f - options.dim) * clamp01(1.f - std::abs(distance)) });
    }

    return result;
}
