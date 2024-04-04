miracle-wm
Copyright (C) 2024  Matthew Kosarek

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

#include "leaf_node.h"
#include "miracle_config.h"
#include "parent_node.h"
#include <cmath>

using namespace miracle;

LeafNode::LeafNode(
    TilingInterface& node_interface,
    geom::Rectangle area,
    std::shared_ptr<MiracleConfig> const& config,
    TilingWindowTree* tree,
    std::shared_ptr<ParentNode> const& parent)
    : Node(parent),
      node_interface{node_interface},
      logical_area{std::move(area)},
      config{config},
      tree{tree}
{
}

void LeafNode::associate_to_window(miral::Window const& in_window)
{
    window = in_window;
}

geom::Rectangle LeafNode::get_logical_area() const
{
    return next_logical_area ? next_logical_area.value() : logical_area;
}

void LeafNode::set_logical_area(geom::Rectangle const& target_rect)
{
    next_logical_area = target_rect;
}

void LeafNode::set_parent(std::shared_ptr<ParentNode> const& in_parent)
{
    parent = in_parent;
}

void LeafNode::scale_area(double x, double y)
{
    logical_area.size.width = geom::Width{ceil(x * logical_area.size.width.as_int())};
    logical_area.size.height = geom::Height {ceil(y * logical_area.size.height.as_int())};
}

void LeafNode::translate(int x, int y)
{
    logical_area.top_left.x = geom::X{logical_area.top_left.x.as_int() + x};
    logical_area.top_left.y = geom::Y{logical_area.top_left.y.as_int() + y};
}

geom::Rectangle LeafNode::get_visible_area() const
{
    int half_gap_x = _has_right_neighbor() ? (int)(ceil((double) config->get_inner_gaps_x() / 2.0)) : 0;
    int half_gap_y = _has_bottom_neighbor() ? (int)(ceil((double) config->get_inner_gaps_y() / 2.0)) : 0;
    return {
        geom::Point{
            logical_area.top_left.x.as_int(),
            logical_area.top_left.y.as_int()
        },
        geom::Size{
            logical_area.size.width.as_int() - 2 * half_gap_x,
            logical_area.size.height.as_int() - 2 * half_gap_y
        }
    };
}

void LeafNode::constrain()
{
    if (node_interface.is_fullscreen(window))
        node_interface.noclip(window);
    else
        node_interface.clip(window, get_visible_area());
}

size_t LeafNode::get_min_width() const
{
    return 50;
}

size_t LeafNode::get_min_height() const
{
    return 50;
}

void LeafNode::show()
{
    next_state = before_shown_state;
    before_shown_state.reset();
}

void LeafNode::hide()
{
    before_shown_state = node_interface.get_state(window);
    next_state = mir_window_state_hidden;
}

void LeafNode::toggle_fullscreen()
{
    auto state = node_interface.get_state(window);
    if (state == mir_window_state_fullscreen)
        next_state = mir_window_state_restored;
    else
        next_state = mir_window_state_fullscreen;
}

bool LeafNode::is_fullscreen() const
{
    return node_interface.get_state(window) == mir_window_state_fullscreen;
}

void LeafNode::commit_changes()
{
    if (next_state)
    {
        node_interface.change_state(window, next_state.value());
        constrain();
        next_state.reset();
    }

    if (next_logical_area)
    {
        logical_area = next_logical_area.value();
        next_logical_area.reset();
        node_interface.set_rectangle(window, get_visible_area());
        constrain();
    }
}