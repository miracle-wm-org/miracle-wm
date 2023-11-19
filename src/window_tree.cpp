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

using namespace miracle;

namespace miracle
{

/// A window tree lane item _always_ begins its life as a single window.
/// When another window is added to it, it becomes a WindowTreeLane.
class WindowTreeLaneItem
{
public:
    explicit WindowTreeLaneItem(miral::Window& window)
        : window{window}
    {
    }

    geom::Rectangle get_rectangle()
    {
        return geom::Rectangle{window.top_left(), window.size()};
    }

    miral::Window window;
};

}

WindowTree::WindowTree(geom::Size default_size)
    : root{std::make_shared<WindowTreeLane>()},
      active{root},
      size{default_size}
{
}

miral::WindowSpecification WindowTree::allocate_position(const miral::WindowSpecification &requested_specification)
{
    miral::WindowSpecification new_spec = requested_specification;
    pending_lane = active;
    if (root == active && root->items.empty())
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
    auto width_per_item = rectangle.size.width.as_int() / static_cast<float>(pending_lane->items.size() + 1);
    auto new_size = geom::Size{geom::Width{width_per_item}, rectangle.size.height};
    for (auto index = 0; index < pending_lane->items.size(); index++)
    {
        auto item = pending_lane->items[index];
        auto new_position = geom::Point{
            rectangle.top_left.x.as_int() + width_per_item * index,
            rectangle.top_left.y
        };
        item->window.move_to(new_position);
        item->window.resize(new_size);
    }

    auto new_position = geom::Point{
        rectangle.top_left.x.as_int() + rectangle.size.width.as_int() - width_per_item,
        rectangle.top_left.y
    };
    new_spec.top_left() = new_position;
    new_spec.size() = new_size;
    return new_spec;
}

miral::Rectangle WindowTree::confirm(miral::Window &window)
{
    if (!pending_lane)
    {
        // TODO: Error
        return miral::Rectangle{};
    }

    auto item = std::make_shared<WindowTreeLaneItem>(window);
    pending_lane->items.push_back(item);
    pending_lane = nullptr;
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

void WindowTreeLane::add(miral::Window &window)
{

}

void WindowTreeLane::remove(miral::Window &window)
{

}

geom::Rectangle WindowTreeLane::get_rectangle()
{
    geom::Rectangle rectangle{geom::Point{INT_MAX, INT_MAX}, geom::Size{0, 0}};
    for (auto item : items)
    {
        auto item_rectangle = item->get_rectangle();
        rectangle.top_left.x = std::min(rectangle.top_left.x, item_rectangle.top_left.x);
        rectangle.top_left.y = std::min(rectangle.top_left.y, item_rectangle.top_left.y);
        rectangle.size.width = std::max(
            rectangle.size.width,
            geom::Width{item_rectangle.top_left.x.as_int() + item_rectangle.size.width.as_int()});
        rectangle.size.height = std::max(
            rectangle.size.height,
            geom::Height{item_rectangle.top_left.y.as_int() + item_rectangle.size.height.as_int()});
    }
    return rectangle;
}