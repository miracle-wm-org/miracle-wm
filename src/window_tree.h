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

#ifndef MIRCOMPOSITOR_WINDOW_TREE_H
#define MIRCOMPOSITOR_WINDOW_TREE_H

#include <memory>
#include <vector>
#include <miral/window.h>
#include <miral/window_specification.h>
#include <mir/geometry/size.h>
#include <mir/geometry/rectangle.h>

namespace geom = mir::geometry;

namespace miracle
{

class WindowTreeLaneItem;

/// Terrible name. This describes an horizontal or vertical lane
/// along which windows and other lanes may be laid out.
struct WindowTreeLane
{
    std::vector<std::shared_ptr<WindowTreeLaneItem>> items;

    enum {
        horizontal,
        vertical,
        length
    } direction;

    /// The rectangle defined by the lane can be retrieved dynamically
    /// by calculating the dimensions of all the windows involved in
    /// this lane and its sub-lanes;
    geom::Rectangle get_rectangle();

    void add(miral::Window& window);
    void remove(miral::Window& window);
};

/// Represents a tiling tree for an output.
class WindowTree
{
public:
    WindowTree(geom::Size default_size);
    ~WindowTree() = default;

    /// Makes space for the new window and returns its specified spot in the world.
    miral::WindowSpecification allocate_position(const miral::WindowSpecification &requested_specification);

    /// Confirms the position of this window in the previously allocated position.
    miral::Rectangle confirm(miral::Window&);

    void remove(miral::Window&);
    void resize(geom::Size new_size);

private:
    std::shared_ptr<WindowTreeLane> root;
    std::shared_ptr<WindowTreeLane> active;
    std::shared_ptr<WindowTreeLane> pending_lane;
    geom::Size size;
};

}


#endif //MIRCOMPOSITOR_WINDOW_TREE_H
