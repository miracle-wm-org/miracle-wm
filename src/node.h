#ifndef NODE_H
#define NODE_H

#include "window_metadata.h"
#include <mir/geometry/rectangle.h>
#include <vector>
#include <memory>
#include <miral/window.h>
#include <miral/window_manager_tools.h>
#include <functional>

namespace geom = mir::geometry;

namespace miracle
{
class MiracleConfig;

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
    Node(miral::WindowManagerTools const& tools, geom::Rectangle const&, std::shared_ptr<MiracleConfig> const& config);
    Node(miral::WindowManagerTools const& tools,
         geom::Rectangle const&,
         std::shared_ptr<Node> parent,
         std::shared_ptr<WindowMetadata> const& metadata,
         std::shared_ptr<MiracleConfig> const& config);

    /// Area taken up by the node including gaps.
    geom::Rectangle get_logical_area();

    /// Area taken up by the node minus the gaps
    geom::Rectangle get_visible_area();

    /// Makes room for a new node on the lane.
    geom::Rectangle create_new_node_position(int index = -1);

    /// Append the node to the lane
    void add_window(miral::Window&);

    /// Updates the node's logical area (including gaps)
    void set_logical_area(geom::Rectangle const& target_rect);

    // Scales the logical area by the provided amount
    void scale_area(double x_scale, double y_scale);

    // Translates the logical area by the provided amount
    void translate_by(int x, int y);

    /// Walk the tree to find the lane that contains this window.
    /// @returns The node if it is found, otherwise false
    std::shared_ptr<Node> find_node_for_window(miral::Window& window);

    /// Insert a node at a particular index
    void insert_node(std::shared_ptr<Node> const& node, int index);

    /// Swap the position of two nodes in the lane
    void swap_nodes(std::shared_ptr<Node> const& first, std::shared_ptr<Node> const& second);

    void set_direction(NodeLayoutDirection in_direction) { direction = in_direction; }

    /// Removes the node from the lane but does NOT recalcualte the size
    void remove_node(std::shared_ptr<Node> const& node);

    /// Transform the window to a lane if it isn't already
    std::shared_ptr<Node> to_lane();

    bool restore(std::shared_ptr<Node>& node);
    bool minimize(std::shared_ptr<Node>& node);

    /// Constrains all nodes in this subtree to the size of their logical area
    void constrain();

    /* Getters below here */

    std::shared_ptr<Node> find_where(std::function<bool(std::shared_ptr<Node> const&)> func) const;
    int get_index_of_node(std::shared_ptr<Node> const&) const;
    int num_nodes() const;
    std::shared_ptr<Node> node_at(int i) const;
    std::shared_ptr<Node> find_nth_window_child(int i) const;
    int get_min_width() const;
    int get_min_height() const;
    bool is_window() const { return state == NodeState::window; }
    bool is_lane() const { return state == NodeState::lane; }
    NodeLayoutDirection get_direction() const { return direction; }
    miral::Window& get_window() { return metadata->get_window(); }
    std::shared_ptr<Node> get_parent() const { return parent; }
    std::vector<std::shared_ptr<Node>> const& get_sub_nodes() const { return sub_nodes; }

private:
    std::shared_ptr<Node> parent;
    miral::WindowManagerTools tools;
    std::shared_ptr<WindowMetadata> metadata;
    std::vector<std::shared_ptr<Node>> sub_nodes;
    std::vector<std::shared_ptr<Node>> hidden_nodes;
    NodeState state;
    NodeLayoutDirection direction = NodeLayoutDirection::horizontal;
    geom::Rectangle logical_area;
    std::shared_ptr<MiracleConfig> config;
    int pending_index = -1;
    geom::Rectangle pending_logical_rect;

    void _set_window_rectangle(geom::Rectangle area);
    geom::Rectangle get_logical_area_internal(geom::Rectangle const& rectangle);
    static geom::Rectangle _get_visible_from_logical(
        geom::Rectangle const& logical_area,
        std::shared_ptr<MiracleConfig> const& config);
    /// Recalculates the size of the nodes in the lane.
    void _refit_node_to_area();
};
}


#endif //MIRCOMPOSITOR_NODE_H
