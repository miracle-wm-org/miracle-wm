/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "window_tree.h"
#include <memory>
#include <mir/log.h>
#include <iostream>

using namespace miracle;

WindowTree::WindowTree(geom::Size default_size)
    : root_lane{std::make_shared<Node>()},
      active_lane{root_lane},
      size{default_size}
{
}

void WindowTree::resize_node_to(std::shared_ptr<Node> node)
{

}

miral::WindowSpecification WindowTree::allocate_position(const miral::WindowSpecification &requested_specification)
{
    miral::WindowSpecification new_spec = requested_specification;
    if (active_lane->is_root() && active_lane->get_sub_nodes().size() == 0)
    {
        // Special case: take up the full size of the root node.
        new_spec.top_left() = geom::Point{0, 0};
        new_spec.size() = size;
        return new_spec;
    }

    // Everyone get out the damn way. Slide the other items to the left for now.
    // TODO: Handle inserting in the middle of the group
    // TODO: Handle non-equal sizing
    auto rectangle = active_lane->get_rectangle();
    if (active_lane->get_direction() == NodeDirection::horizontal)
    {
        auto width_per_item = rectangle.size.width.as_int() / static_cast<float>(active_lane->get_sub_nodes().size() + 1);
        auto new_size = geom::Size{geom::Width{width_per_item}, rectangle.size.height};
        auto new_position = geom::Point{
            rectangle.top_left.x.as_int() + rectangle.size.width.as_int() - width_per_item,
            rectangle.top_left.y
        };
        new_spec.top_left() = new_position;
        new_spec.size() = new_size;
    }
    else if (active_lane->get_direction() == NodeDirection::vertical)
    {
        auto height_per_item = rectangle.size.height.as_int() / static_cast<float>(active_lane->get_sub_nodes().size() + 1);
        auto new_size = geom::Size{rectangle.size.width, height_per_item};
        auto new_position = geom::Point{
            rectangle.top_left.x,
            rectangle.top_left.y.as_int() + rectangle.size.height.as_int() - height_per_item
        };
        new_spec.top_left() = new_position;
        new_spec.size() = new_size;
    }

    return new_spec;
}

void WindowTree::confirm(miral::Window &window)
{
    geom::Rectangle rectangle;
    if (active_lane->is_root() && active_lane->get_sub_nodes().size() == 0)
    {
        // Special case: take up the full size of the root node.
        rectangle.top_left = geom::Point{0, 0};
        rectangle.size = size;
    }
    else
    {
        rectangle  = active_lane->get_rectangle();
    }

    active_lane->get_sub_nodes().push_back(std::make_shared<Node>(active_lane, window));
    active_lane->set_rectangle(rectangle);
    advise_focus_gained(window);
}

void WindowTree::remove(miral::Window &)
{

}

void WindowTree::resize(geom::Size new_size)
{
    size = new_size;
    // TODO: Resize all windows
}

void WindowTree::request_vertical()
{
    handle_direction_request(NodeDirection::vertical);
}

void WindowTree::request_horizontal()
{
    handle_direction_request(NodeDirection::horizontal);
}

void WindowTree::handle_direction_request(NodeDirection direction)
{
    auto window_lane = active_lane->find_node_for_window(active_window);
    if (!window_lane->is_window())
    {
        std::cerr << "WindowTree::handle_direction_request: must change direction of a window" << std::endl;
        return;
    }

    // Edge case: if we're an only child, we can just change the direction of the parent to achieve the same effect
    if (window_lane->parent->get_sub_nodes().size() == 1)
    {
        active_lane = window_lane->parent;
    }
    else
    {
        active_lane = window_lane;
        active_lane->to_lane();
    }

    active_lane->set_direction(direction);
}

void WindowTree::advise_focus_gained(miral::Window& window)
{
    // The node that we find will be the window, so its parent must be the lane
    auto found_node = root_lane->find_node_for_window(window);
    active_lane = found_node->parent;
    if (!active_lane->is_lane())
    {
        std::cerr << "Active lane is NOT a lane" << std::endl;
    }
    active_window = window;
}

void WindowTree::advise_focus_lost(miral::Window& window)
{
}

void WindowTree::advise_delete_window(miral::Window& window)
{
    // Capture the previous size before anything starts
    auto rectangle = active_lane->get_rectangle();

    // Resize the other nodes in the lane accordingly
    auto lane = root_lane->find_node_for_window(window);
    if (!lane)
    {
        std::cerr << "Unable to find lane for window" << std::endl;
        return;
    }

    if (!lane->is_window())
    {
        std::cerr << "Lane should have been a window" << std::endl;
        return;
    }

    active_lane = lane->parent;

    if (active_lane->get_sub_nodes().size() == 1)
    {
        // Remove the entire lane if this lane is now empty
        if (active_lane->parent)
        {
            auto prev_active = active_lane;
            active_lane = active_lane->parent;
            rectangle = active_lane->get_rectangle(); // Note: The rectangle needs to point to the new active to be correct.

            active_lane->get_sub_nodes().erase(
                std::remove_if(active_lane->get_sub_nodes().begin(), active_lane->get_sub_nodes().end(), [&](std::shared_ptr<Node> content) {
                    return content->is_lane() && content == prev_active;
                }),
                active_lane->get_sub_nodes().end()
            );
        }
    }
    else
    {
        // Remove the window from the active lane
        active_lane->get_sub_nodes().erase(
            std::remove_if(active_lane->get_sub_nodes().begin(), active_lane->get_sub_nodes().end(), [&](std::shared_ptr<Node> content) {
                return content->is_window() && content->get_window() == window;
            }),
            active_lane->get_sub_nodes().end()
        );
    }

    // Edge case: If the newly active node only owns one other lane, it can absorb the node
    if (active_lane->get_sub_nodes().size() == 1 && active_lane->get_sub_nodes()[0]->is_lane())
    {
        auto dying_lane = active_lane->get_sub_nodes()[0];
        active_lane->get_sub_nodes().clear();
        for (auto sub_node : dying_lane->get_sub_nodes())
        {
            active_lane->get_sub_nodes().push_back(sub_node);
        }
        active_lane->set_direction(dying_lane->get_direction());
    }

    active_lane->set_rectangle(rectangle);
}