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

class NodeContent;

/// This describes an horizontal or vertical lane
/// along which windows and other lanes may be laid out.
struct Node : public std::enable_shared_from_this<Node>
{
    std::vector<std::shared_ptr<NodeContent>> items;

    enum {
        horizontal,
        vertical,
        length
    } direction  = horizontal;

    /// The rectangle defined by the lane can be retrieved dynamically
    /// by calculating the dimensions of all the windows involved in
    /// this lane and its sub-lanes;
    geom::Rectangle get_rectangle();

    /// Walk the tree to find the lane that contains this window.
    std::shared_ptr<Node> find_lane(miral::Window& window);

    /// Transform the window found in the list to a lane. Returns the
    /// new window tree lane item if found, otherwise null.
    std::shared_ptr<Node> to_lane(miral::Window& window);
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

    // Request a change to vertical window placement
    void request_vertical();

    void advise_focus_gained(miral::Window&);
    void advise_focus_lost(miral::Window&);

private:
    std::shared_ptr<Node> root_lane;
    std::shared_ptr<Node> active_lane;
    std::shared_ptr<Node> pending_lane;
    miral::Window active_window;
    geom::Size size;
};

}


#endif //MIRCOMPOSITOR_WINDOW_TREE_H
