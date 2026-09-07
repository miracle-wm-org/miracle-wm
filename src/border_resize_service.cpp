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

#include "border_resize_service.h"

#include "compositor_state.h"
#include "config.h"
#include "cursor_override.h"
#include "output_manager.h"
#include "resize_service.h"
#include "window_container.h"
#include "window_controller.h"

#include <mir_toolkit/cursors.h>

namespace miracle
{

MirResizeEdge border_edge_at(
    geom::Rectangle const& logical,
    geom::Rectangle const& visible,
    int border_size,
    bool allow_diagonals,
    float x,
    float y)
{
    geom::Point const point { x, y };
    if (!logical.contains(point))
        return mir_resize_edge_none;

    // The client's content rectangle: what is left of the visible area once the
    // border that miracle draws in the window margins is taken off.
    int const inset = std::max(0, border_size);
    int const inner_width = visible.size.width.as_int() - 2 * inset;
    int const inner_height = visible.size.height.as_int() - 2 * inset;
    if (inner_width > 0 && inner_height > 0)
    {
        geom::Rectangle const inner {
            geom::Point { visible.left().as_int() + inset, visible.top().as_int() + inset },
            geom::Size { inner_width,                     inner_height                   }
        };
        if (inner.contains(point))
            return mir_resize_edge_none;

        int edge = mir_resize_edge_none;
        if (point.x < inner.left())
            edge |= mir_resize_edge_west;
        else if (point.x >= inner.right())
            edge |= mir_resize_edge_east;

        if (point.y < inner.top())
            edge |= mir_resize_edge_north;
        else if (point.y >= inner.bottom())
            edge |= mir_resize_edge_south;

        if (!allow_diagonals)
        {
            // Prefer whichever axis the point is furthest into the band on, so that a
            // point in a corner still resizes the edge the user is most likely aiming at.
            if ((edge & (mir_resize_edge_west | mir_resize_edge_east))
                && (edge & (mir_resize_edge_north | mir_resize_edge_south)))
            {
                auto const horizontal_depth = point.x < inner.left()
                    ? (inner.left() - point.x).as_int()
                    : (point.x - inner.right()).as_int();
                auto const vertical_depth = point.y < inner.top()
                    ? (inner.top() - point.y).as_int()
                    : (point.y - inner.bottom()).as_int();

                if (horizontal_depth >= vertical_depth)
                    edge &= ~(mir_resize_edge_north | mir_resize_edge_south);
                else
                    edge &= ~(mir_resize_edge_west | mir_resize_edge_east);
            }

            return static_cast<MirResizeEdge>(edge);
        }

        // Promote a lone edge to a diagonal near the corners, which are otherwise only
        // as large as the band is thick and consequently near impossible to hit.
        if (!(edge & (mir_resize_edge_north | mir_resize_edge_south)))
        {
            if ((point.y - logical.top()).as_int() < border_resize_corner_size)
                edge |= mir_resize_edge_north;
            else if ((logical.bottom() - point.y).as_int() <= border_resize_corner_size)
                edge |= mir_resize_edge_south;
        }

        if (!(edge & (mir_resize_edge_west | mir_resize_edge_east)))
        {
            if ((point.x - logical.left()).as_int() < border_resize_corner_size)
                edge |= mir_resize_edge_west;
            else if ((logical.right() - point.x).as_int() <= border_resize_corner_size)
                edge |= mir_resize_edge_east;
        }

        return static_cast<MirResizeEdge>(edge);
    }

    // The window is smaller than its own border. Treat the whole thing as a corner
    // rather than reporting nothing at all.
    return allow_diagonals ? mir_resize_edge_northwest : mir_resize_edge_west;
}

std::string const& cursor_name_for_edge(MirResizeEdge edge)
{
    static std::string const vertical { mir_vertical_resize_cursor_name };
    static std::string const horizontal { mir_horizontal_resize_cursor_name };
    static std::string const north_east { mir_diagonal_resize_bottom_to_top_cursor_name };
    static std::string const south_west { mir_diagonal_resize_bottom_to_left_cursor_name };
    static std::string const north_west { mir_diagonal_resize_top_to_left_cursor_name };
    static std::string const south_east { mir_diagonal_resize_top_to_bottom_cursor_name };
    static std::string const fallback { mir_default_cursor_name };

    switch (edge)
    {
    case mir_resize_edge_north:
    case mir_resize_edge_south:
        return vertical;
    case mir_resize_edge_east:
    case mir_resize_edge_west:
        return horizontal;
    case mir_resize_edge_northeast:
        return north_east;
    case mir_resize_edge_southwest:
        return south_west;
    case mir_resize_edge_northwest:
        return north_west;
    case mir_resize_edge_southeast:
        return south_east;
    default:
        return fallback;
    }
}

namespace
{
    /// Whether a tiled container can actually give or take space on \p edge. Without a
    /// neighbour in that direction [Container::execute_resize] is a no-op, so we must not
    /// advertise a resize handle there - the outer edges of a workspace, for instance.
    bool has_neighbor(WindowContainer const& container, MirResizeEdge edge)
    {
        switch (edge)
        {
        case mir_resize_edge_north:
            return container.neighbor_north() != nullptr;
        case mir_resize_edge_south:
            return container.neighbor_south() != nullptr;
        case mir_resize_edge_east:
            return container.neighbor_east() != nullptr;
        case mir_resize_edge_west:
            return container.neighbor_west() != nullptr;
        default:
            return false;
        }
    }
}

BorderResizeService::BorderResizeService(
    std::shared_ptr<Config> const& config,
    std::shared_ptr<CompositorState> const& state,
    std::shared_ptr<OutputManager> const& output_manager,
    std::shared_ptr<WindowController> const& window_controller,
    std::shared_ptr<CursorOverrideController> const& cursor_override,
    ResizeService& resize_service) :
    config { config },
    state { state },
    output_manager { output_manager },
    window_controller { window_controller },
    cursor_override { cursor_override },
    resize_service { resize_service }
{
}

std::pair<std::shared_ptr<WindowContainer>, MirResizeEdge> BorderResizeService::resolve(float x, float y)
{
    auto const output = output_manager->focused();
    if (!output)
        return { nullptr, mir_resize_edge_none };

    // The border band lives inside the visible area, so the regular intersection -
    // which respects stacking order - finds it. A point in the gap between two windows
    // belongs to no visible area, hence the fallback over logical areas.
    auto container = output->intersect(x, y);
    if (!container)
        container = output->intersect_border(x, y);

    if (!container || !container->needs_outline() || container->is_fullscreen())
        return { nullptr, mir_resize_edge_none };

    bool const anchored = container->anchored();
    auto const edge = border_edge_at(
        container->get_logical_area(),
        container->get_visible_area(),
        config->get_border_config().size,
        !anchored,
        x,
        y);

    if (edge == mir_resize_edge_none)
        return { nullptr, mir_resize_edge_none };

    if (anchored && !has_neighbor(*container, edge))
        return { nullptr, mir_resize_edge_none };

    return { container, edge };
}

bool BorderResizeService::handle_pointer_event(float x, float y, MirPointerAction action, MirPointerButtons buttons)
{
    if (action == mir_pointer_action_leave)
    {
        clear();
        return false;
    }

    if (state->mode() != WindowManagerMode::normal)
    {
        clear();
        return false;
    }

    auto const [container, edge] = resolve(x, y);
    if (!container)
    {
        clear();
        return false;
    }

    cursor_override->set_override(cursor_name_for_edge(edge));

    if (action != mir_pointer_action_button_down || !(buttons & mir_pointer_button_primary))
        return false;

    if (auto const window = container->window())
        window_controller->select_active_window(window.value());

    resize_service.handle_request_resize(container, action, edge, x, y);
    return true;
}

void BorderResizeService::clear()
{
    cursor_override->set_override(std::nullopt);
}

} // namespace miracle
