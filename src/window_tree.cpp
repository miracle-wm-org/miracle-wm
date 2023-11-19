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
    explicit WindowTreeLaneItem(std::shared_ptr<mir::scene::Session> application)
        : application{application}
    {
    }

    geom::Rectangle get_rectangle()
    {
        return geom::Rectangle{};
    }

private:
    std::weak_ptr<mir::scene::Session> application;
    std::unique_ptr<WindowTreeLane> maybe_lane;
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
    return requested_specification;
}

miral::Rectangle WindowTree::confirm(miral::Window &)
{
    if (!pending_lane)
    {
        // TODO: Error
        return miral::Rectangle{};
    }
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
    auto item = std::make_shared<WindowTreeLaneItem>(window.application());
    items.push_back(item);
}

void WindowTreeLane::remove(miral::Window &window)
{

}