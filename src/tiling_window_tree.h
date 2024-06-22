/**
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
**/

#ifndef MIRACLE_TREE_H
#define MIRACLE_TREE_H

#include "direction.h"
#include "node.h"
#include "node_common.h"
#include <memory>
#include <mir/geometry/rectangle.h>
#include <miral/window.h>
#include <miral/window_manager_tools.h>
#include <miral/window_specification.h>
#include <miral/zone.h>
#include <vector>

namespace geom = mir::geometry;

namespace miracle
{

class OutputContent;
class MiracleConfig;
class TilingInterface;
class LeafNode;

class TilingWindowTree
{
public:
    TilingWindowTree(
        OutputContent* parent,
        TilingInterface&,
        std::shared_ptr<MiracleConfig> const& options);
    ~TilingWindowTree();

    /// Makes space for the new window and returns its specified spot in the grid. Note that the returned
    /// position is the position WITH gaps.
    miral::WindowSpecification allocate_position(const miral::WindowSpecification& requested_specification);

    std::shared_ptr<LeafNode> advise_new_window(miral::WindowInfo const&);

    /// Try to resize the current active window in the provided direction
    bool try_resize_active_window(Direction direction);

    /// Move the active window in the provided direction
    bool try_move_active_window(Direction direction);

    /// Select the next window in the provided direction
    bool try_select_next(Direction direction);

    /// Toggle the active window between fullscreen and not fullscreen
    bool try_toggle_active_fullscreen();

    bool has_fullscreen_window() const { return is_active_window_fullscreen; }

    // Request a change to vertical window placement
    void request_vertical();

    // Request a change to horizontal window placement
    void request_horizontal();

    // Request a change from the current layout scheme to another layout scheme
    void toggle_layout();

    /// Advises us to focus the provided window.
    void advise_focus_gained(miral::Window&);

    /// Advises us to lose focus on the provided window.
    void advise_focus_lost(miral::Window&);

    /// Called when the window was deleted.
    void advise_delete_window(miral::Window&);

    /// Called when the physical display is resized.
    void set_output_area(geom::Rectangle const& new_area);

    std::shared_ptr<LeafNode> select_window_from_point(int x, int y);

    bool advise_fullscreen_window(miral::Window&);
    bool advise_restored_window(miral::Window&);
    bool handle_window_ready(miral::WindowInfo& window_info);

    bool confirm_placement_on_display(
        miral::Window const& window,
        MirWindowState new_state,
        mir::geometry::Rectangle& new_placement);

    /// Constrains the window to its tile if it is in this tree.
    bool constrain(miral::Window& window);

    void foreach_node(std::function<void(std::shared_ptr<Node>)> const&);

    std::shared_ptr<Node> find_node(std::function<bool(std::shared_ptr<Node> const&)> const&);

    /// Hides the entire tree
    void hide();

    /// Shows the tree and returns a fullscreen node
    std::shared_ptr<LeafNode> show();

    void recalculate_root_node_area();
    bool is_empty();

private:
    struct MoveResult
    {
        enum
        {
            traversal_type_invalid,
            traversal_type_insert,
            traversal_type_prepend,
            traversal_type_append
        } traversal_type
            = traversal_type_invalid;
        std::shared_ptr<Node> node = nullptr;
    };

    OutputContent* screen;
    TilingInterface& tiling_interface;
    std::shared_ptr<MiracleConfig> config;
    std::shared_ptr<ParentNode> root_lane;

    // TODO: We can probably remove active_window and just resolve it efficiently now?
    std::shared_ptr<LeafNode> active_window;
    bool is_active_window_fullscreen = false;
    bool is_hidden = false;
    int config_handle = 0;

    std::shared_ptr<ParentNode> get_active_lane();
    void handle_direction_change(NodeLayoutDirection direction);
    void handle_resize(std::shared_ptr<Node> const& node, Direction direction, int amount);

    /// Removes the node from the tree
    /// @returns The parent that will need to have its changes committed
    std::shared_ptr<ParentNode> handle_remove(std::shared_ptr<Node> const& node);

    /// Transfer a node from its current parent to the parent of 'to'
    /// in a position right after 'to'.
    /// @returns The two parents who will need to have their changes committed
    std::tuple<std::shared_ptr<ParentNode>, std::shared_ptr<ParentNode>> transfer_node(
        std::shared_ptr<LeafNode> const& node,
        std::shared_ptr<Node> const& to);

    /// From the provided node, find the next node in the provided direction.
    /// This method is guaranteed to return a Window node, not a Lane.
    MoveResult handle_move(std::shared_ptr<Node> const& from, Direction direction);

    /// Selects the next node in the provided direction
    /// @returns The next selectable window or nullptr if none is found
    static std::shared_ptr<LeafNode> handle_select(std::shared_ptr<Node> const& from, Direction direction);
};

}

#endif // MIRACLE_TREE_H
