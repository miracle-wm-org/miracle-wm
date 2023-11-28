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

using namespace miracle;

namespace miracle
{

/// Node Content _always_ begins its life as a single window.
/// When requested by the user, it turns itself into a new
/// node in the tree.
class NodeContent
{
public:
    explicit NodeContent(miral::Window& window)
        : window{window}
    {
    }

    geom::Rectangle get_rectangle()
    {
        if (is_window())
        {
            return geom::Rectangle{window.top_left(), window.size()};
        }
        else
        {
            geom::Rectangle rectangle;
            for (auto item : node->node_content_list)
            {
                auto item_rectangle = item->get_rectangle();
                rectangle.top_left.x = std::min(rectangle.top_left.x, item_rectangle.top_left.x);
                rectangle.top_left.y = std::min(rectangle.top_left.y, item_rectangle.top_left.y);

                if (node->direction == Node::horizontal)
                {
                    rectangle.size.width = geom::Width{rectangle.size.width.as_int() + item_rectangle.size.width.as_int()};
                    rectangle.size.height = std::max(
                        rectangle.size.height,
                        geom::Height{item_rectangle.top_left.y.as_int() + item_rectangle.size.height.as_int()});
                }
                else if (node->direction == Node::vertical)
                {
                    rectangle.size.width = std::max(
                        rectangle.size.width,
                        geom::Width{item_rectangle.top_left.x.as_int() + item_rectangle.size.width.as_int()});
                    rectangle.size.height = geom::Height{rectangle.size.height.as_int() + item_rectangle.size.height.as_int()};
                }
            }
            return rectangle;
        }
    }

    void set_rectangle(geom::Rectangle rect)
    {
        if (is_window())
        {
            window.move_to(rect.top_left);
            window.resize(rect.size);
        }
        else
        {
            // TODO: This needs to divide the space equally among windows
            for (auto item : node->node_content_list)
            {
                item->set_rectangle(rect);
            }
        }
    }

    bool is_window()
    {
        return node == nullptr;
    }

    bool is_node()
    {
        return node != nullptr;
    }

    miral::Window& get_window() {
        return window;
    }

    std::shared_ptr<Node> get_node() {
        return node;
    }

    /// Transforms this lane item into a node.
    std::shared_ptr<Node> to_node()
    {
        if (node != nullptr)
            return node;

        node = std::make_shared<Node>();
        node->node_content_list.push_back(std::make_shared<NodeContent>(window));
        return node;
    }

private:
    miral::Window window;
    std::shared_ptr<Node> node = nullptr;
};

}

WindowTree::WindowTree(geom::Size default_size)
    : root_lane{std::make_shared<Node>()},
      active_lane{root_lane},
      size{default_size}
{
}

miral::WindowSpecification WindowTree::allocate_position(const miral::WindowSpecification &requested_specification)
{
    miral::WindowSpecification new_spec = requested_specification;
    pending_lane = active_lane;
    if (root_lane == active_lane && root_lane->node_content_list.empty())
    {
        // Special case: take up the full size of the root node.
        new_spec.top_left() = geom::Point{0, 0};
        new_spec.size() = size;
        return new_spec;
    }

    // Everyone get out the damn way. Slide the other items to the left for now.
    // TODO: Handle inserting in the middle of the group
    // TODO: Handle vertical placement
    // TODO: Handle non-equal sizing
    auto rectangle = pending_lane->get_rectangle();
    if (active_lane->direction == Node::horizontal)
    {
        auto width_per_item = rectangle.size.width.as_int() / static_cast<float>(pending_lane->node_content_list.size() + 1);
        auto new_size = geom::Size{geom::Width{width_per_item}, rectangle.size.height};
        for (auto index = 0; index < pending_lane->node_content_list.size(); index++)
        {
            auto item = pending_lane->node_content_list[index];
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
    else if (active_lane->direction == Node::vertical)
    {
        auto height_per_item = rectangle.size.height.as_int() / static_cast<float>(pending_lane->node_content_list.size() + 1);
        auto new_size = geom::Size{rectangle.size.width, height_per_item};
        for (auto index = 0; index < pending_lane->node_content_list.size(); index++)
        {
            auto item = pending_lane->node_content_list[index];
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

    auto item = std::make_shared<NodeContent>(window);
    pending_lane->node_content_list.push_back(item);
    pending_lane = nullptr;
    advise_focus_gained(window);
    return item->get_rectangle();
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
    active_lane = active_lane->window_to_node(active_window);
    active_lane->direction = Node::vertical;

    // This is when we add a new lane, which means that we need to know who is currently
    // selected.
}

void WindowTree::advise_focus_gained(miral::Window& window)
{
    active_lane = root_lane->find_node_for_window(window);
    active_window = window;
}

void WindowTree::advise_focus_lost(miral::Window& window)
{
}

geom::Rectangle Node::get_rectangle()
{
    geom::Rectangle rectangle{geom::Point{INT_MAX, INT_MAX}, geom::Size{0, 0}};

    // This method iterates over the content of the node and tries to figure out the area
    // that this node takes up.
    for (auto item : node_content_list)
    {
        auto item_rectangle = item->get_rectangle();
        rectangle.top_left.x = std::min(rectangle.top_left.x, item_rectangle.top_left.x);
        rectangle.top_left.y = std::min(rectangle.top_left.y, item_rectangle.top_left.y);

        if (direction == Node::horizontal)
        {
            rectangle.size.width = geom::Width{rectangle.size.width.as_int() + item_rectangle.size.width.as_int()};
            rectangle.size.height = std::max(
                rectangle.size.height,
                geom::Height{item_rectangle.size.height.as_int()});
        }
        else if (direction == Node::vertical)
        {
            rectangle.size.width = std::max(
                rectangle.size.width,
                geom::Width{item_rectangle.size.width.as_int()});
            rectangle.size.height = geom::Height{rectangle.size.height.as_int() + item_rectangle.size.height.as_int()};
        }

    }

    return rectangle;
}

std::shared_ptr<Node> Node::find_node_for_window(miral::Window &window)
{
    for (auto item : node_content_list)
    {
        if (item->is_window())
        {
            if (item->get_window() == window)
                return shared_from_this();
        }
        else
        {
            return item->get_node()->find_node_for_window(window);
        }
    }

    // TODO: Error
    return nullptr;
}

std::shared_ptr<miracle::Node> Node::window_to_node(miral::Window &window)
{
    for (auto item : node_content_list)
    {
        if (item->is_window())
        {
            if (item->get_window() == window)
                return item->to_node();
        }
        else
        {
            return item->get_node()->window_to_node(window);
        }
    }

    // TODO: Error
    return nullptr;
}