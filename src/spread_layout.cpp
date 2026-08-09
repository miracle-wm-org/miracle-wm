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
geom::DisplacementF half_extent(geom::RectangleF const& r)
{
    return { r.size.width.as_value() / 2.f, r.size.height.as_value() / 2.f };
}

geom::PointF center_of(geom::RectangleF const& r)
{
    return r.top_left + half_extent(r);
}

void move_center_to(geom::RectangleF& r, geom::PointF const& center)
{
    r.top_left = center - half_extent(r);
}

void scale_about_center(geom::RectangleF& r, float scale)
{
    auto const center = center_of(r);
    r.size = { r.size.width.as_value() * scale, r.size.height.as_value() * scale };
    move_center_to(r, center);
}

bool overlaps(geom::RectangleF const& a, geom::RectangleF const& b, float gap)
{
    return a.left().as_value() < b.right().as_value() + gap
        && b.left().as_value() < a.right().as_value() + gap
        && a.top().as_value() < b.bottom().as_value() + gap
        && b.top().as_value() < a.bottom().as_value() + gap;
}

void clamp_into(geom::RectangleF& r, geom::RectangleF const& bounds)
{
    float const half_w = std::min(r.size.width.as_value(), bounds.size.width.as_value()) / 2.f;
    float const half_h = std::min(r.size.height.as_value(), bounds.size.height.as_value()) / 2.f;
    auto const center = center_of(r);
    move_center_to(r, geom::PointF {
                          std::clamp(center.x.as_value(), bounds.left().as_value() + half_w, bounds.right().as_value() - half_w),
                          std::clamp(center.y.as_value(), bounds.top().as_value() + half_h, bounds.bottom().as_value() - half_h) });
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
    geom::RectangleF fbounds {
        {},
        geom::SizeF { std::max(static_cast<float>(bounds.size.width.as_value()) - 2.f * fgap, 1.f),
                     std::max(static_cast<float>(bounds.size.height.as_value()) - 2.f * fgap, 1.f) }
    };
    move_center_to(fbounds, geom::PointF { static_cast<float>(bounds.top_left.x.as_value()) + static_cast<float>(bounds.size.width.as_value()) / 2.f,
                                static_cast<float>(bounds.top_left.y.as_value()) + static_cast<float>(bounds.size.height.as_value()) / 2.f });

    // Radial pre-spread: push every window center outward from the bounds
    // center so the spread visibly moves even for already non-overlapping
    // layouts.
    auto const bounds_center = center_of(fbounds);
    float const diagonal = std::hypot(fbounds.size.width.as_value(), fbounds.size.height.as_value());
    std::vector<geom::RectangleF> pre(n);
    for (size_t i = 0; i < n; ++i)
    {
        auto const& w = windows[i];
        geom::RectangleF r {
            geom::PointF { static_cast<float>(w.top_left.x.as_value()), static_cast<float>(w.top_left.y.as_value()) },
            geom::SizeF { static_cast<float>(w.size.width.as_value()), static_cast<float>(w.size.height.as_value()) }
        };

        auto const center = center_of(r);
        float dx = center.x.as_value() - bounds_center.x.as_value();
        float dy = center.y.as_value() - bounds_center.y.as_value();
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

        move_center_to(r, center + geom::DisplacementF { dx, dy });
        pre[i] = r;
    }

    float scale = 1.f;
    std::vector<geom::RectangleF> best;
    for (int attempt = 0; attempt < 8; ++attempt)
    {
        std::vector<geom::RectangleF> rects = pre;
        for (auto& r : rects)
        {
            scale_about_center(r, scale);
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

                    auto const center_i = center_of(rects[i]);
                    auto const center_j = center_of(rects[j]);
                    float dx = center_j.x.as_value() - center_i.x.as_value();
                    float dy = center_j.y.as_value() - center_i.y.as_value();
                    float length = std::hypot(dx, dy);
                    if (length < 1.f)
                    {
                        auto const [ux, uy] = direction_for_index(j, n);
                        dx = ux;
                        dy = uy;
                        length = 1.f;
                    }

                    float const overlap_x = (rects[i].size.width.as_value() + rects[j].size.width.as_value()) / 2.f + fgap
                        - std::abs(center_j.x.as_value() - center_i.x.as_value());
                    float const overlap_y = (rects[i].size.height.as_value() + rects[j].size.height.as_value()) / 2.f + fgap
                        - std::abs(center_j.y.as_value() - center_i.y.as_value());
                    float const push = std::min(overlap_x, overlap_y) / 2.f + 1.f;

                    geom::DisplacementF const offset { dx / length * push, dy / length * push };
                    move_center_to(rects[i], center_i - offset);
                    move_center_to(rects[j], center_j + offset);
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
            geom::Point { static_cast<int>(std::round(r.left().as_value())), static_cast<int>(std::round(r.top().as_value())) },
            geom::Size { std::max(static_cast<int>(std::round(r.size.width.as_value())), 1),
                std::max(static_cast<int>(std::round(r.size.height.as_value())), 1) });
    }

    return result;
}
