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
    pending_lane = active_lane;
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
    auto rectangle = pending_lane->get_rectangle();
    if (active_lane->get_direction() == NodeDirection::horizontal)
    {
        auto width_per_item = rectangle.size.width.as_int() / static_cast<float>(pending_lane->get_sub_nodes().size() + 1);
        auto new_size = geom::Size{geom::Width{width_per_item}, rectangle.size.height};
        for (auto index = 0; index < pending_lane->get_sub_nodes().size(); index++)
        {
            auto item = pending_lane->get_sub_nodes()[index];
            auto new_position = geom::Point{
                rectangle.top_left.x.as_int() + width_per_item * index,
                rectangle.top_left.y
            };
            item->set_rectangle(geom::Rectangle{new_position, new_size});
        }

        auto new_position = geom::Point{
            rectangle.top_left.x.as_int() + rectangle.size.width.as_int() - width_per_item,
            rectangle.top_left.y
        };
        new_spec.top_left() = new_position;
        new_spec.size() = new_size;
    }
    else if (active_lane->get_direction() == NodeDirection::vertical)
    {
        auto height_per_item = rectangle.size.height.as_int() / static_cast<float>(pending_lane->get_sub_nodes().size() + 1);
        auto new_size = geom::Size{rectangle.size.width, height_per_item};
        for (auto index = 0; index < pending_lane->get_sub_nodes().size(); index++)
        {
            auto item = pending_lane->get_sub_nodes()[index];
            auto new_position = geom::Point{
                rectangle.top_left.x,
                rectangle.top_left.y.as_int() + height_per_item * index
            };
            item->set_rectangle(geom::Rectangle{new_position, new_size});
        }

        auto new_position = geom::Point{
            rectangle.top_left.x,
            rectangle.top_left.y.as_int() + rectangle.size.height.as_int() - height_per_item
        };
        new_spec.top_left() = new_position;
        new_spec.size() = new_size;
    }

    return new_spec;
}

miral::Rectangle WindowTree::confirm(miral::Window &window)
{
    if (!pending_lane)
    {
        // TODO: Error
        return miral::Rectangle{};
    }

    pending_lane->get_sub_nodes().push_back(std::make_shared<Node>(pending_lane, window));
    pending_lane = nullptr;
    advise_focus_gained(window);
    return active_lane->get_rectangle();
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
    active_lane = active_lane->find_node_for_window(active_window);
    active_lane->to_lane();
    active_lane->set_direction(NodeDirection::vertical);
}

void WindowTree::request_horizontal()
{
    active_lane = active_lane->find_node_for_window(active_window);
    active_lane->to_lane();
    active_lane->set_direction(NodeDirection::horizontal);
}

void WindowTree::advise_focus_gained(miral::Window& window)
{
    // The node that we find will be the window, so its parent must be the lane
    auto found_node = root_lane->find_node_for_window(window);
    active_lane = found_node->parent;
    if (!active_lane->is_lane())
    {
        // TODO: proper error
        printf("Active lane is NOT a lane");
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

    // Resize the active lane to account for its new size.
    // TODO: Account for window with unequal size
    // TODO: This is very similar to allocate_position
    if (active_lane->get_direction() == NodeDirection::horizontal)
    {
        auto width_per_item = rectangle.size.width.as_int() / static_cast<float>(active_lane->get_sub_nodes().size());
        auto new_size = geom::Size{geom::Width{width_per_item}, rectangle.size.height};
        for (auto index = 0; index < active_lane->get_sub_nodes().size(); index++)
        {
            auto item = active_lane->get_sub_nodes()[index];
            auto new_position = geom::Point{
                rectangle.top_left.x.as_int() + width_per_item * index,
                rectangle.top_left.y
            };
            item->set_rectangle(geom::Rectangle{new_position, new_size});
        }
    }
    else if (active_lane->get_direction() == NodeDirection::vertical)
    {
        auto height_per_item = rectangle.size.height.as_int() / static_cast<float>(active_lane->get_sub_nodes().size());
        auto new_size = geom::Size{rectangle.size.width, height_per_item};
        for (auto index = 0; index < active_lane->get_sub_nodes().size(); index++)
        {
            auto item = active_lane->get_sub_nodes()[index];
            auto new_position = geom::Point{
                rectangle.top_left.x,
                rectangle.top_left.y.as_int() + height_per_item * index
            };
            item->set_rectangle(geom::Rectangle{new_position, new_size});
        }
    }
}