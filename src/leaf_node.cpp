#include "leaf_node.h"
#include "miracle_config.h"
#include "parent_node.h"
#include <cmath>

using namespace miracle;

LeafNode::LeafNode(
    std::shared_ptr<TilingInterface> const& node_interface,
    geom::Rectangle area,
    std::shared_ptr<MiracleConfig> const& config,
    TilingWindowTree* tree,
    std::shared_ptr<ParentNode> const& parent)
    : Node(parent),
      node_interface{node_interface},
      logical_area{std::move(area)},
      config{config},
      tree{tree}
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

void LeafNode::set_parent(std::shared_ptr<ParentNode> const& in_parent)
{
    parent = in_parent;
}

void LeafNode::scale_area(double x, double y)
{
    logical_area.size.width = geom::Width{ceil(x * logical_area.size.width.as_int())};
    logical_area.size.height = geom::Height {ceil(y * logical_area.size.height.as_int())};
}

void LeafNode::translate(int x, int y)
{
    logical_area.top_left.x = geom::X{logical_area.top_left.x.as_int() + x};
    logical_area.top_left.y = geom::Y{logical_area.top_left.y.as_int() + y};
}

geom::Rectangle LeafNode::get_visible_area() const
{
    int half_gap_x = _has_right_neighbor() ? (int)(ceil((double) config->get_inner_gaps_x() / 2.0)) : 0;
    int half_gap_y = _has_bottom_neighbor() ? (int)(ceil((double) config->get_inner_gaps_y() / 2.0)) : 0;
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

void LeafNode::constrain()
{
    if (node_interface->is_fullscreen(window))
        node_interface->noclip(window);
    else
        node_interface->clip(window, get_visible_area());
}

size_t LeafNode::get_min_width() const
{
    return 50;
}

size_t LeafNode::get_min_height() const
{
    return 50;
}

void LeafNode::show()
{
    next_state = before_shown_state;
    before_shown_state.reset();
}

void LeafNode::hide()
{
    before_shown_state = node_interface->get_state(window);
    next_state = mir_window_state_hidden;
}

void LeafNode::toggle_fullscreen()
{
    auto state = node_interface->get_state(window);
    if (state == mir_window_state_fullscreen)
        next_state = mir_window_state_restored;
    else
        next_state = mir_window_state_fullscreen;
}

void LeafNode::commit_changes()
{
    if (next_state)
    {
        node_interface->change_state(window, next_state.value());
        next_state.reset();
    }
    node_interface->set_rectangle(window, logical_area);
    constrain();
}