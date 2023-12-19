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

WindowTree::WindowTree(geom::Size default_size, const miral::WindowManagerTools & tools)
    : root_lane{std::make_shared<Node>(geom::Rectangle{geom::Point{}, default_size})},
      tools{tools},
      size{default_size}
{
}

miral::WindowSpecification WindowTree::allocate_position(const miral::WindowSpecification &requested_specification)
{
    miral::WindowSpecification new_spec = requested_specification;
    auto rect = active_lane()->new_node_position();
    new_spec.size() = rect.size;
    new_spec.top_left() = rect.top_left;
    return new_spec;
}

void WindowTree::confirm(miral::Window &window)
{
    active_lane()->add_node(std::make_shared<Node>(
        geom::Rectangle{window.top_left(), window.size()},
        active_lane(),
        window));
    tools.select_active_window(window);
}

void WindowTree::toggle_resize_mode()
{
    if (is_resizing)
    {
        is_resizing = false;
        return;
    }

    is_resizing = true;
}

bool WindowTree::try_resize_active_window(miracle::Direction direction)
{
    if (!is_resizing)
        return false;

    if (!active_window)
        return false;

    // TODO: We have a hardcoded resize amount
    resize_node_internal(active_window, direction, 50);
    return true;
}

void WindowTree::resize_node_internal(
    std::shared_ptr<Node> node,
    Direction direction,
    int amount)
{
    auto parent = node->parent;
    if (parent == nullptr)
    {
        // Can't resize, most likely the root
        return;
    }

    bool is_vertical = direction == Direction::up || direction == Direction::down;
    bool is_main_axis_movement = (is_vertical  && parent->get_direction() == NodeLayoutDirection::vertical)
        || (!is_vertical && parent->get_direction() == NodeLayoutDirection::horizontal);
    if (!is_main_axis_movement)
    {
        resize_node_internal(parent, direction, amount);
        return;
    }

    bool is_negative = direction == Direction::left || direction == Direction::up;
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

bool WindowTree::try_select_next(miracle::Direction direction)
{
    if (!active_window)
        return false;

    auto node = traverse(active_window, direction);
    if (!node)
        return false;
    tools.select_active_window(node->get_window());
    return true;
}

void WindowTree::resize(geom::Size new_size)
{
    size = new_size;
    // TODO: Resize all windows
}

bool WindowTree::try_move_active_window(miracle::Direction direction)
{
    if (is_resizing)
        return false;

    if (!active_window)
        return false;

    auto second_window = traverse(active_window, direction);
    if (!second_window)
        return false;

    auto parent = second_window->parent;
    if (!parent)
        return false;

    auto insertion_index = parent->get_index_of_node(second_window);
    advise_delete_window(active_window->get_window());
    parent->insert_node(active_window, insertion_index);
    return true;
}

void WindowTree::request_vertical()
{
    handle_direction_request(NodeLayoutDirection::vertical);
}

void WindowTree::request_horizontal()
{
    handle_direction_request(NodeLayoutDirection::horizontal);
}

void WindowTree::handle_direction_request(NodeLayoutDirection direction)
{
    if (is_resizing)
        return;

    if (!active_window)
        return;

    if (active_window->parent->get_sub_nodes().size() != 1)
        active_window = active_window->to_lane();

    active_lane()->set_direction(direction);
}

void WindowTree::advise_focus_gained(miral::Window& window)
{
    if (is_resizing)
        is_resizing = false;

    // The node that we find will be the window, so its parent must be the lane
    auto found_node = root_lane->find_node_for_window(window);
    active_window = found_node;
}

void WindowTree::advise_focus_lost(miral::Window&)
{
    if (is_resizing)
        is_resizing = false;
}

void WindowTree::advise_delete_window(miral::Window& window)
{
    // Resize the other nodes in the lane accordingly
    auto parent = active_lane();

    if (parent->get_sub_nodes().size() == 1)
    {
        // Remove the entire lane if this lane is now empty
        if (parent->parent)
        {
            auto prev_active = parent;
            parent = parent->parent;

            parent->get_sub_nodes().erase(
                std::remove_if(parent->get_sub_nodes().begin(), parent->get_sub_nodes().end(), [&](std::shared_ptr<Node> content) {
                    return content->is_lane() && content == prev_active;
                }),
                parent->get_sub_nodes().end()
            );
        }
    }
    else
    {
        // Remove the window from the active lane
        parent->get_sub_nodes().erase(
            std::remove_if(parent->get_sub_nodes().begin(), parent->get_sub_nodes().end(), [&](std::shared_ptr<Node> content) {
                return content->is_window() && content->get_window() == window;
            }),
            parent->get_sub_nodes().end()
        );
    }

    // Edge case: If the newly active node only owns one other lane, it can absorb the node
    if (parent->get_sub_nodes().size() == 1 && parent->get_sub_nodes()[0]->is_lane())
    {
        auto dying_lane = parent->get_sub_nodes()[0];
        parent->get_sub_nodes().clear();
        for (auto sub_node : dying_lane->get_sub_nodes())
        {
            parent->add_node(sub_node);
        }
        parent->set_direction(dying_lane->get_direction());
    }

    parent->redistribute_size();
}

std::shared_ptr<Node> WindowTree::traverse(std::shared_ptr<Node> from, Direction direction)
{
    if (!from->parent)
    {
        std::cerr << "Cannot traverse the root node\n";
        return nullptr;
    }

    auto parent = from->parent;
    int index = parent->get_index_of_node(from);
    auto parent_direction = parent->get_direction();

    bool is_vertical = direction == Direction::up || direction == Direction::down;
    bool is_negative = direction == Direction::up || direction == Direction::left;

    if (is_vertical && parent_direction == NodeLayoutDirection::vertical
        || !is_vertical && parent_direction == NodeLayoutDirection::horizontal)
    {
        // Simplest case: we're within a lane
        if (is_negative)
        {
            if (index == 0)
                goto grandparent_route;  // TODO: lazy lazy for readability
            else
                return parent->node_at(index - 1);
        }
        else
        {
            if (index == parent->num_nodes() - 1)
                goto grandparent_route;  // TODO: lazy lazy for readability
            else
                return parent->node_at(index + 1);
        }
    }
    else
    {
grandparent_route:
        // Harder case: we need to jump to another lane. The best thing to do here is to
        // find the first ancestor that matches the direction that we want to travel in.
        // If  that ancestor cannot be found, then we throw up our hands.
        auto grandparent = parent->parent;
        if (!grandparent)
        {
            std::cerr << "Parent lane lacks a grandparent. It should AT LEAST be root\n";
            return nullptr;
        }

        do {
            auto index_of_parent = grandparent->get_index_of_node(parent);
            if (is_negative)
                index_of_parent--;
            else
                index_of_parent++;

            if (grandparent->get_direction() == NodeLayoutDirection::horizontal && !is_vertical
                || grandparent->get_direction() == NodeLayoutDirection::vertical && is_vertical)
            {
                return grandparent->find_nth_window_child(index_of_parent);
            }

            parent = grandparent;
            grandparent = grandparent->parent;
        } while (grandparent != nullptr);
    }


    return nullptr;
}

std::shared_ptr<Node> WindowTree::active_lane()
{
    if (!active_window)
        return root_lane;

    return active_window->parent;
}