#ifndef NODE_H
#define NODE_H

#include <mir/geometry/rectangle.h>
#include <vector>
#include <memory>
#include <miral/window.h>
#include <miral/window_manager_tools.h>
#include <functional>

namespace geom = mir::geometry;

namespace miracle
{

enum class NodeState
{
    window ,
    lane
};

enum class NodeLayoutDirection
{
    horizontal,
    vertical
};

/// A node in the tree is either a single window or a lane.
class Node : public std::enable_shared_from_this<Node>
{
public:
    Node(miral::WindowManagerTools const& tools, geom::Rectangle, int gap_x, int gap_y);
    Node(miral::WindowManagerTools const& tools,geom::Rectangle, std::shared_ptr<Node> parent, miral::Window& window, int gap_x, int gap_y);

    /// Area taken up by the node including gaps.
    geom::Rectangle get_logical_area();

    /// Area taken up by the node minus the gaps
    geom::Rectangle get_visible_area();

    /// Makes room for a new node on the lane.
    geom::Rectangle new_node_position(int index = -1);

    /// Append the node to the lane
    void add_window(miral::Window&);

    /// Recalculates the size of the nodes in the lane.
    void redistribute_size();

    /// Updates the node's logical area (including gaps)
    void set_rectangle(geom::Rectangle target_rect);

    /// Walk the tree to find the lane that contains this window.
    std::shared_ptr<Node> find_node_for_window(miral::Window& window);

    /// Transform the window  in the list to a Node. Returns the
    /// new Node if the Window was found, otherwise null.
    std::shared_ptr<Node> window_to_node(miral::Window& window);

    /// Insert a node at a particular index
    void insert_node(std::shared_ptr<Node> node, int index);

    void set_direction(NodeLayoutDirection in_direction) { direction = in_direction; }

    /// Removes the node from the lane but does NOT recalcualte the size
    void remove_node(std::shared_ptr<Node> const& node);

    int get_index_of_node(std::shared_ptr<Node>);
    int num_nodes();
    std::shared_ptr<Node> node_at(int i);

    std::shared_ptr<Node> to_lane();
    std::shared_ptr<Node> find_nth_window_child(int i);

    void scale_area(double x_scale, double y_scale);
    void translate_by(int x, int y);

    std::shared_ptr<Node> find_where(std::function<bool(std::shared_ptr<Node>)> func);
    bool restore(std::shared_ptr<Node>& node);
    bool minimize(std::shared_ptr<Node>& node);

    int get_min_width();
    int get_min_height();
    bool is_window() { return state == NodeState::window; }
    bool is_lane() { return state == NodeState::lane; }
    NodeLayoutDirection get_direction() { return direction; }
    miral::Window& get_window() { return window; }
    std::shared_ptr<Node> get_parent() { return parent; }
    int get_gap_x() { return gap_x; }
    int get_gap_y() { return gap_y; }
    std::vector<std::shared_ptr<Node>> const& get_sub_nodes() { return sub_nodes; }
    void constrain();

private:
    std::shared_ptr<Node> parent;
    miral::WindowManagerTools tools;
    miral::Window window;
    std::vector<std::shared_ptr<Node>> sub_nodes;
    std::vector<std::shared_ptr<Node>> hidden_nodes;
    NodeState state;
    NodeLayoutDirection direction = NodeLayoutDirection::horizontal;
    geom::Rectangle logical_area;
    int gap_x;
    int gap_y;
    int pending_index = -1;

    void _set_window_rectangle(geom::Rectangle area);
    static geom::Rectangle _get_visible_from_logical(geom::Rectangle const& logical_area, int gap_x, int gap_y);
    static geom::Rectangle _get_logical_from_visible(const geom::Rectangle &visible_area, int gap_x, int gap_y);
};
}


#endif //MIRCOMPOSITOR_NODE_H
