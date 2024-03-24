#include "leaf_node.h"
#include "miracle_config.h"
#include <cmath>

using namespace miracle;

LeafNode::LeafNode(
    std::shared_ptr<NodeInterface> const& node_interface,
    geom::Rectangle area,
    std::shared_ptr<MiracleConfig> const& config,
    Tree* tree,
    Node* parent)
    : node_interface{node_interface},
      logical_area{std::move(area)},
      config{config},
      tree{tree},
      parent{parent}
{
}

void LeafNode::associate_to_window(miral::Window const& in_window)
{
    window = in_window;
}

geom::Rectangle LeafNode::get_logical_area() const
{
    return logical_area;
}

void LeafNode::set_logical_area(geom::Rectangle const& target_rect)
{
    logical_area = target_rect;
}

geom::Rectangle LeafNode::get_visible_area() const
{
    // TODO!
    bool has_right_neighbor = false;
    bool has_bottom_neighbor = false;
    int half_gap_x = has_right_neighbor ? (int)(ceil((double) config->get_inner_gaps_x() / 2.0)) : 0;
    int half_gap_y = has_bottom_neighbor ? (int)(ceil((double) config->get_inner_gaps_y() / 2.0)) : 0;
    return {
        geom::Point{
            logical_area.top_left.x.as_int(),
            logical_area.top_left.y.as_int()
        },
        geom::Size{
            logical_area.size.width.as_int() - 2 * half_gap_x,
            logical_area.size.height.as_int() - 2 * half_gap_y
        }
    };
}

void LeafNode::show()
{
    is_shown = true;
}

void LeafNode::hide()
{
    is_shown = false;
}

void LeafNode::commit_changes()
{
    node_interface->set_rectangle(window, logical_area);
    if (is_shown)
        node_interface->show(window);
    else
        node_interface->hide(window);
}