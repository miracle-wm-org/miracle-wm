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
#include <cmath>

using namespace miracle;

WindowTree::WindowTree(geom::Size default_size)
    : root_lane{std::make_shared<Node>(geom::Rectangle{geom::Point{}, default_size})},
      active_lane{root_lane},
      size{default_size}
{
}

miral::WindowSpecification WindowTree::allocate_position(const miral::WindowSpecification &requested_specification)
{
    miral::WindowSpecification new_spec = requested_specification;
    auto rect = active_lane->new_node_position();
    new_spec.size() = rect.size;
    new_spec.top_left() = rect.top_left;
    return new_spec;
}

void WindowTree::confirm(miral::Window &window)
{
    active_lane->add_node(std::make_shared<Node>(
        geom::Rectangle{window.top_left(), window.size()},
        active_lane,
        window));
}

void WindowTree::toggle_resize_mode()
{
    if (is_resizing)
    {
        is_resizing = false;
        return;
    }

    auto window_lane = root_lane->find_node_for_window(active_window);
    if (!window_lane)
        return;

    is_resizing = true;
}

bool WindowTree::try_resize_active_window(miracle::WindowResizeDirection direction)
{
    if (!is_resizing)
        return false;

    auto window_lane = root_lane->find_node_for_window(active_window);
    if (!window_lane->is_window())
    {
        std::cerr << "WindowTree::try_resize_active_window: unable to resize a non-window" << std::endl;
        return false;
    }

    // TODO: We have a hardcoded resize amount
    resize_node_internal(window_lane, direction, 50);
    return true;
}

void WindowTree::resize_node_internal(
    std::shared_ptr<Node> node,
    WindowResizeDirection direction,
    int amount)
{
    auto parent = node->parent;
    if (parent == nullptr)
    {
        // Can't resize, most likely the root
        return;
    }

    bool is_vertical = direction == WindowResizeDirection::up || direction == WindowResizeDirection::down;
    bool is_main_axis_movement = (is_vertical  && parent->get_direction() == NodeDirection::vertical)
        || (!is_vertical && parent->get_direction() == NodeDirection::horizontal);
    if (!is_main_axis_movement)
    {
        resize_node_internal(parent, direction, amount);
        return;
    }

    bool is_negative = direction == WindowResizeDirection::left || direction == WindowResizeDirection::up;
    auto resize_amount = is_negative ? -amount : amount;
    auto nodes = parent->get_sub_nodes();
    if (is_vertical)
    {
        int height_for_others = floor(-resize_amount / static_cast<float>(nodes.size() - 1));
        for (size_t i = 0; i < nodes.size(); i++)
        {
            auto other_node = nodes[i];
            auto other_rect = other_node->get_rectangle();
            if (node == other_node)
                other_rect.size.height = geom::Height{other_rect.size.height.as_int() + resize_amount};
            else
                other_rect.size.height = geom::Height{other_rect.size.height.as_int() + height_for_others};

            if (i != 0)
            {
                auto prev_rect = nodes[i - 1]->get_rectangle();
                other_rect.top_left.y = geom::Y{prev_rect.top_left.y.as_int() + prev_rect.size.height.as_int()};
            }
            other_node->set_rectangle(other_rect);
        }
    }
    else
    {
        int width_for_others = floor(-resize_amount / static_cast<float>(nodes.size() - 1));
        for (size_t i = 0; i < nodes.size(); i++)
        {
            auto other_node = nodes[i];
            auto other_rect = other_node->get_rectangle();
            if (node == other_node)
                other_rect.size.width = geom::Width {other_rect.size.width.as_int() + resize_amount};
            else
                other_rect.size.width = geom::Width {other_rect.size.width.as_int() + width_for_others};

            if (i != 0)
            {
                auto prev_rect = nodes[i - 1]->get_rectangle();
                other_rect.top_left.x = geom::X{prev_rect.top_left.x.as_int() + prev_rect.size.width.as_int()};
            }
            other_node->set_rectangle(other_rect);
        }
    }
}

void WindowTree::resize(geom::Size new_size)
{
    size = new_size;
    // TODO: Resize all windows
}

bool WindowTree::try_move_active_window(miracle::WindowMoveDirection direction)
{
    if (is_resizing)
        return false;

    bool is_vertical = direction == WindowMoveDirection::up || direction == WindowMoveDirection::down;
    bool is_negative = direction == WindowMoveDirection::up || direction == WindowMoveDirection::left;

    int node_index = 0;
    for (; node_index < active_lane->get_sub_nodes().size(); node_index++)
        if (active_lane->get_sub_nodes()[node_index]->get_window() == active_window)
            break;

    // In both of the first two cases, we first delete the node from the parent
    // and then move it to a new parent, if one exists. If none exists, we do nothing.
    // TODO: Also update the active_lane!
    if (node_index == 0 && is_negative)
    {
        auto parent = active_lane->parent;
        if (!parent)
        {
            // TODO: Error  message?
            return false;
        }

        // Move "up" the tree to the parent node. The node should
        // get inserted into the parent node at the index before
        // the currently active lane.
        auto node_to_move = active_lane->get_sub_nodes()[node_index];

        int active_lane_index = 0;
        for (; active_lane_index < parent->get_sub_nodes().size(); active_lane_index++)
            if (parent->get_sub_nodes()[active_lane_index] == active_lane)
                break;

        advise_delete_window(node_to_move->get_window());
        parent->insert_node(node_to_move, active_lane_index);
        active_lane = parent;
    }
    else if (node_index == active_lane->get_sub_nodes().size() - 1 && !is_negative)
    {
        // Move "down" the tree into the parent node
        auto parent = active_lane->parent;
        if (!parent)
        {
            // TODO: Error  message?
            return false;
        }

        // Move "up" the tree to the parent node. The node should
        // get inserted into the parent node at the index before
        // the currently active lane.
        auto node_to_move = active_lane->get_sub_nodes()[node_index];

        int active_lane_index = 0;
        for (; active_lane_index < parent->get_sub_nodes().size(); active_lane_index++)
            if (parent->get_sub_nodes()[active_lane_index] == active_lane)
                break;

        advise_delete_window(node_to_move->get_window());
        parent->insert_node(node_to_move, active_lane_index + 1);
        active_lane = parent;
    }
    else if (is_negative)
    {
        active_lane->move_node(node_index, node_index - 1);
    }
    else
    {
        active_lane->move_node(node_index, node_index + 1);
    }

    return true;
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
    if (is_resizing)
        return;

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
    if (is_resizing)
        is_resizing = false;

    // The node that we find will be the window, so its parent must be the lane
    auto found_node = root_lane->find_node_for_window(window);
    active_lane = found_node->parent;
    if (!active_lane->is_lane())
    {
        std::cerr << "Active lane is NOT a lane" << std::endl;
    }
    active_window = window;
}

void WindowTree::advise_focus_lost(miral::Window&)
{
    if (is_resizing)
        is_resizing = false;
}

void WindowTree::advise_delete_window(miral::Window& window)
{
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
            active_lane->add_node(sub_node);
        }
        active_lane->set_direction(dying_lane->get_direction());
    }

    active_lane->redistribute_size();
}