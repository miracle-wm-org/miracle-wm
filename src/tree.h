#ifndef WINDOW_TREE_H
#define WINDOW_TREE_H

#include "node.h"
#include "window_metadata.h"
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

class Screen;
class MiracleConfig;
    
enum class Direction
{
    up,
    left,
    down,
    right
};

class Tree
{
public:
    Tree(Screen* parent, miral::WindowManagerTools const& tools, std::shared_ptr<MiracleConfig> const& options);
    ~Tree();

    /// Makes space for the new window and returns its specified spot in the grid. Note that the returned
    /// position is the position WITH GAPS.
    miral::WindowSpecification allocate_position(const miral::WindowSpecification &requested_specification);

    std::shared_ptr<WindowMetadata> advise_new_window(miral::WindowInfo const&);

    /// Places us into resize mode. Other operations are prohibited while we are in resize mode.
    void toggle_resize_mode();

    /// Try to resize the current active window in the provided direction
    bool try_resize_active_window(Direction direction);

    /// Move the active window in the provided direction
    bool try_move_active_window(Direction direction);

    /// Select the next window in the provided direction
    bool try_select_next(Direction direction);

    /// Toggle the active window between fullscreen and not fullscreen
    bool try_toggle_active_fullscreen();

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

    bool select_window_from_point(int x, int y);

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

    void add_tree(std::shared_ptr<Tree> const&);

    void foreach_node(std::function<void(std::shared_ptr<Node>)> const&);
    void close_active_window();

    /// Hides the entire tree
    void hide();

    /// Shows the entire tree
    void show();

    std::shared_ptr<Node> get_root_node();
    void recalculate_root_node_area();
    bool is_empty();

private:
    struct MoveResult
    {
        enum {
            traversal_type_invalid,
            traversal_type_insert,
            traversal_type_prepend,
            traversal_type_append
        } traversal_type = traversal_type_invalid;
        std::shared_ptr<Node> node = nullptr;
    };

    struct NodeResurrection
    {
        std::shared_ptr<Node> node;
        MirWindowState state;
    };

    Screen* screen;
    miral::WindowManagerTools tools;
    std::shared_ptr<MiracleConfig> config;
    std::shared_ptr<Node> root_lane;
    std::shared_ptr<Node> active_window;
    bool is_resizing = false;
    bool is_active_window_fullscreen = false;
    bool is_hidden = false;
    std::vector<NodeResurrection> nodes_to_resurrect;
    int config_handle = 0;

    std::shared_ptr<Node> _get_active_lane();
    void _handle_direction_request(NodeLayoutDirection direction);
    void _handle_resize_request(std::shared_ptr<Node> const& node, Direction direction, int amount);
    void _handle_node_remove(std::shared_ptr<Node> const& node);
    /// From the provided node, find the next node in the provided direction.
    /// This method is guaranteed to return a Window node, not a Lane.
    MoveResult _move(std::shared_ptr<Node> const& from, Direction direction);
    static std::shared_ptr<Node> _select(std::shared_ptr<Node> const& from, Direction direction);
};

}


#endif //MIRCOMPOSITOR_WINDOW_TREE_H
