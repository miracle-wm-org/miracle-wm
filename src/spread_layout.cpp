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
#include <numbers>

namespace geom = mir::geometry;

namespace
{
struct FloatRect
{
    float cx;
    float cy;
    float w;
    float h;

    float left() const { return cx - w / 2.f; }
    float right() const { return cx + w / 2.f; }
    float top() const { return cy - h / 2.f; }
    float bottom() const { return cy + h / 2.f; }
};

bool overlaps(FloatRect const& a, FloatRect const& b, float gap)
{
    return a.left() < b.right() + gap && b.left() < a.right() + gap
        && a.top() < b.bottom() + gap && b.top() < a.bottom() + gap;
}

void clamp_into(FloatRect& r, FloatRect const& bounds)
{
    float const half_w = std::min(r.w, bounds.w) / 2.f;
    float const half_h = std::min(r.h, bounds.h) / 2.f;
    r.cx = std::clamp(r.cx, bounds.left() + half_w, bounds.right() - half_w);
    r.cy = std::clamp(r.cy, bounds.top() + half_h, bounds.bottom() - half_h);
}

/// Direction used when two rects share a center (or a window sits exactly on
/// the spread origin): derived from the index so it is deterministic.
std::pair<float, float> direction_for_index(size_t i, size_t n)
{
    float const angle = 2.f * std::numbers::pi_v<float> * static_cast<float>(i) / static_cast<float>(std::max<size_t>(n, 1));
    return { std::cos(angle), std::sin(angle) };
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
    FloatRect const fbounds {
        static_cast<float>(bounds.top_left.x.as_value()) + static_cast<float>(bounds.size.width.as_value()) / 2.f,
        static_cast<float>(bounds.top_left.y.as_value()) + static_cast<float>(bounds.size.height.as_value()) / 2.f,
        std::max(static_cast<float>(bounds.size.width.as_value()) - 2.f * fgap, 1.f),
        std::max(static_cast<float>(bounds.size.height.as_value()) - 2.f * fgap, 1.f)
    };

    // Radial pre-spread: push every window center outward from the bounds
    // center so the spread visibly moves even for already non-overlapping
    // layouts.
    float const diagonal = std::hypot(fbounds.w, fbounds.h);
    std::vector<FloatRect> pre(n);
    for (size_t i = 0; i < n; ++i)
    {
        auto const& w = windows[i];
        float const cx = static_cast<float>(w.top_left.x.as_value()) + static_cast<float>(w.size.width.as_value()) / 2.f;
        float const cy = static_cast<float>(w.top_left.y.as_value()) + static_cast<float>(w.size.height.as_value()) / 2.f;
        float dx = cx - fbounds.cx;
        float dy = cy - fbounds.cy;
        if (std::abs(dx) < 1.f && std::abs(dy) < 1.f)
        {
            auto const [ux, uy] = direction_for_index(i, n);
            dx = ux * diagonal / 8.f;
            dy = uy * diagonal / 8.f;
        }
        else
        {
            dx *= 0.15f;
            dy *= 0.15f;
        }

        pre[i] = FloatRect {
            cx + dx,
            cy + dy,
            static_cast<float>(w.size.width.as_value()),
            static_cast<float>(w.size.height.as_value())
        };
    }

    float scale = 1.f;
    std::vector<FloatRect> best;
    for (int attempt = 0; attempt < 8; ++attempt)
    {
        std::vector<FloatRect> rects = pre;
        for (auto& r : rects)
        {
            r.w *= scale;
            r.h *= scale;
            clamp_into(r, fbounds);
        }

        bool settled = false;
        for (int iteration = 0; iteration < 300 && !settled; ++iteration)
        {
            settled = true;
            for (size_t i = 0; i < n; ++i)
            {
                for (size_t j = i + 1; j < n; ++j)
                {
                    if (!overlaps(rects[i], rects[j], fgap))
                        continue;

                    float dx = rects[j].cx - rects[i].cx;
                    float dy = rects[j].cy - rects[i].cy;
                    float length = std::hypot(dx, dy);
                    if (length < 1.f)
                    {
                        auto const [ux, uy] = direction_for_index(j, n);
                        dx = ux;
                        dy = uy;
                        length = 1.f;
                    }

                    float const overlap_x = (rects[i].w + rects[j].w) / 2.f + fgap - std::abs(rects[j].cx - rects[i].cx);
                    float const overlap_y = (rects[i].h + rects[j].h) / 2.f + fgap - std::abs(rects[j].cy - rects[i].cy);
                    float const push = std::min(overlap_x, overlap_y) / 2.f + 1.f;

                    rects[i].cx -= dx / length * push;
                    rects[i].cy -= dy / length * push;
                    rects[j].cx += dx / length * push;
                    rects[j].cy += dy / length * push;
                    clamp_into(rects[i], fbounds);
                    clamp_into(rects[j], fbounds);
                    settled = false;
                }
            }
        }

        best = rects;
        if (settled)
            break;

        scale *= 0.85f;
    }

    std::vector<geom::Rectangle> result;
    result.reserve(n);
    for (auto const& r : best)
    {
        result.emplace_back(
            geom::Point { static_cast<int>(std::round(r.left())), static_cast<int>(std::round(r.top())) },
            geom::Size { std::max(static_cast<int>(std::round(r.w)), 1), std::max(static_cast<int>(std::round(r.h)), 1) });
    }

    return result;
}
