#ifndef WINDOW_TREE_H
#define WINDOW_TREE_H

#include "node.h"
#include <memory>
#include <vector>
#include <miral/window.h>
#include <miral/window_specification.h>
#include <mir/geometry/rectangle.h>
#include <mir/geometry/rectangle.h>
#include <miral/window_manager_tools.h>
#include <miral/zone.h>

namespace geom = mir::geometry;

namespace miracle
{

enum class Direction
{
    up,
    left,
    down,
    right
};

struct WindowTreeOptions
{
    int gap_x;
    int gap_y;
};

/// Represents a tiling tree for an output.
class WindowTree
{
public:
    WindowTree(geom::Rectangle const& area, miral::WindowManagerTools const& tools, WindowTreeOptions const& options);
    ~WindowTree() = default;

    /// Makes space for the new window and returns its specified spot in the grid. Note that the returned
    /// position is the position WITH GAPS.
    miral::WindowSpecification allocate_position(const miral::WindowSpecification &requested_specification);

    void advise_new_window(miral::WindowInfo const&);

    /// Places us into resize mode. Other operations are prohibited while we are in resize mode.
    void toggle_resize_mode();

    /// Try to resize the current active window in the provided direction
    bool try_resize_active_window(Direction direction);

    /// Move the active window in the provided direction
    bool try_move_active_window(Direction direction);

    /// Select the next window in the provided direction
    bool try_select_next(Direction direction);

    // Request a change to vertical window placement
    void request_vertical();

    // Request a change to horizontal window placement
    void request_horizontal();

    /// Advises us to focus the provided window.
    void advise_focus_gained(miral::Window&);

    /// Advises us to lose focus on the provided window.
    void advise_focus_lost(miral::Window&);

    /// Called when the window was deleted.
    void advise_delete_window(miral::Window&);

    /// Called when the physical display is resized.
    void set_output_area(geom::Rectangle const& new_area);

    bool point_is_in_output(int x, int y);

    bool select_window_from_point(int x, int y);

    void advise_application_zone_create(miral::Zone const& application_zone);
    void advise_application_zone_update(miral::Zone const& updated, miral::Zone const& original);
    void advise_application_zone_delete(miral::Zone const& application_zone);

    bool advise_fullscreen_window(miral::WindowInfo const&);
    bool advise_restored_window(miral::WindowInfo const &window_info);
    bool handle_window_ready(miral::WindowInfo& window_info);

    bool advise_state_change(miral::WindowInfo const& window_info, MirWindowState state);
    bool confirm_placement_on_display(
        const miral::WindowInfo &window_info,
        MirWindowState new_state,
        mir::geometry::Rectangle &new_placement);

    /// Constrains the window to its tile if it is in this tree.
    bool constrain(miral::WindowInfo& window_info);

    void add_tree(WindowTree&);

    void foreach_node(std::function<void(std::shared_ptr<Node>)>);

private:
    miral::WindowManagerTools tools;
    WindowTreeOptions options;
    std::shared_ptr<Node> root_lane;
    std::shared_ptr<Node> active_window;
    geom::Rectangle area;
    bool is_resizing = false;
    std::vector<miral::Zone> application_zone_list;
    bool is_active_window_fullscreen = false;

    std::shared_ptr<Node> _get_active_lane();
    void _handle_direction_request(NodeLayoutDirection direction);
    void _handle_resize_request(std::shared_ptr<Node> const& node, Direction direction, int amount);
    /// From the provided node, find the next node in the provided direction.
    /// This method is guaranteed to return a Window node, not a Lane.
    static std::shared_ptr<Node> _traverse(std::shared_ptr<Node> const& from, Direction direction);
    void _recalculate_root_node_area();
};

}


#endif //MIRCOMPOSITOR_WINDOW_TREE_H
