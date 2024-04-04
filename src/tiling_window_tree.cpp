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

#include "window_metadata.h"
#define MIR_LOG_COMPONENT "window_tree"

#include "tiling_window_tree.h"
#include "parent_node.h"
#include "leaf_node.h"
#include "window_helpers.h"
#include "output_content.h"
#include "miracle_config.h"

#include <memory>
#include <mir/log.h>
#include <iostream>
#include <cmath>

using namespace miracle;

TilingWindowTree::TilingWindowTree(
    OutputContent* screen,
    TilingInterface& tiling_interface,
    std::shared_ptr<MiracleConfig> const& config)
    : screen{screen},
      root_lane{std::make_shared<ParentNode>(
        tiling_interface,
        std::move(geom::Rectangle{screen->get_area().top_left, screen->get_area().size}),
        config,
        this,
        nullptr)},
      config{config},
      tiling_interface{tiling_interface}
{
    recalculate_root_node_area();
    config_handle = config->register_listener([&](auto&)
    {
        recalculate_root_node_area();
    });
}

TilingWindowTree::~TilingWindowTree()
{
    config->unregister_listener(config_handle);
}

miral::WindowSpecification TilingWindowTree::allocate_position(const miral::WindowSpecification &requested_specification)
{
    miral::WindowSpecification new_spec = requested_specification;
    new_spec.server_side_decorated() = false;
    new_spec.min_width() = geom::Width{0};
    new_spec.max_width() = geom::Width{std::numeric_limits<int>::max()};
    new_spec.min_height() = geom::Height{0};
    new_spec.max_height() = geom::Height{std::numeric_limits<int>::max()};
    auto node = get_active_lane()->create_space_for_window();
    auto rect = node->get_logical_area();
    new_spec.size() = rect.size;
    new_spec.top_left() = rect.top_left;

    if (new_spec.state().is_set() && window_helpers::is_window_fullscreen(new_spec.state().value()))
    {
        // Don't start anyone in fullscreen mode
        new_spec.state() = mir::optional_value<MirWindowState>();
    }
    return new_spec;
}

std::shared_ptr<LeafNode> TilingWindowTree::advise_new_window(miral::WindowInfo const& window_info)
{
    auto node = get_active_lane()->confirm_window(window_info.window());
    if (window_helpers::is_window_fullscreen(window_info.state()))
    {
        tiling_interface.select_active_window(window_info.window());
        advise_fullscreen_window(window_info.window());
    }
    else
    {
        tiling_interface.send_to_back(window_info.window());
    }

    return node;
}

void TilingWindowTree::toggle_resize_mode()
{
    is_resizing = !is_resizing;
}

bool TilingWindowTree::try_resize_active_window(miracle::Direction direction)
{
    if (!is_resizing)
    {
        mir::log_warning("Unable to resize the active window: not resizing");
        return false;
    }

    if (is_active_window_fullscreen)
    {
        mir::log_warning("Unable to resize the next window: fullscreened");
        return false;
    }

    if (!active_window)
    {
        mir::log_warning("Unable to resize the active window: active window is not set");
        return false;
    }

    handle_resize(active_window, direction, config->get_resize_jump());
    return true;
}

bool TilingWindowTree::try_select_next(miracle::Direction direction)
{
    if (is_active_window_fullscreen)
    {
        mir::log_warning("Unable to select the next window: fullscreened");
        return false;
    }

    if (is_resizing)
    {
        mir::log_warning("Unable to select the next window: resizing");
        return false;
    }

    if (!active_window)
    {
        mir::log_warning("Unable to select the next window: active window not set");
        return false;
    }

    auto node = handle_select(active_window, direction);
    if (!node)
    {
        mir::log_warning("Unable to select the next window: handle_select failed");
        return false;
    }

    tiling_interface.select_active_window(node->get_window());
    return true;
}

bool TilingWindowTree::try_toggle_active_fullscreen()
{
    if (is_resizing)
    {
        mir::log_warning("Cannot toggle fullscreen while resizing");
        return false;
    }

    if (!active_window)
    {
        mir::log_warning("Active window is null while trying to toggle fullscreen");
        return false;
    }

    active_window->toggle_fullscreen();
    active_window->commit_changes();
    if (is_active_window_fullscreen)
        advise_restored_window(active_window->get_window());
    else
        advise_fullscreen_window(active_window->get_window());
    return true;
}

void TilingWindowTree::set_output_area(geom::Rectangle const& new_area)
{
    auto area = screen->get_area();
    double x_scale = static_cast<double>(new_area.size.width.as_int()) / static_cast<double>(area.size.width.as_int());
    double y_scale = static_cast<double>(new_area.size.height.as_int()) / static_cast<double>(area.size.height.as_int());

    int position_diff_x = new_area.top_left.x.as_int() - area.top_left.x.as_int();
    int position_diff_y = new_area.top_left.y.as_int() - area.top_left.y.as_int();
    area.top_left = new_area.top_left;
    area.size = geom::Size{
        geom::Width{ceil(area.size.width.as_int() * x_scale)},
        geom::Height {ceil(area.size.height.as_int() * y_scale)}};

    root_lane->scale_area(x_scale, y_scale);
    root_lane->translate(position_diff_x, position_diff_y);
}

std::shared_ptr<LeafNode> TilingWindowTree::select_window_from_point(int x, int y)
{
    if (is_active_window_fullscreen)
    {
        tiling_interface.select_active_window(active_window->get_window());
        return active_window;
    }

    auto node = root_lane->find_where([&](std::shared_ptr<Node> const& node)
    {
        return node->is_leaf() && node->get_logical_area().contains(geom::Point(x, y));
    });

    return Node::as_leaf(node);
}

bool TilingWindowTree::try_move_active_window(miracle::Direction direction)
{
    if (is_active_window_fullscreen)
    {
        mir::log_warning("Unable to move active window: fullscreen");
        return false;
    }

    if (is_resizing)
    {
        mir::log_warning("Unable to move active window: resizing");
        return false;
    }

    if (!active_window)
    {
        mir::log_warning("Unable to move active window: active window not set");
        return false;
    }

    auto traversal_result = handle_move(active_window, direction);
    switch (traversal_result.traversal_type)
    {
        case MoveResult::traversal_type_insert:
        {
            auto target_node = traversal_result.node;
            if (!target_node)
            {
                mir::log_warning("Unable to move active window: target_window not found");
                return false;
            }

            auto target_parent = target_node->get_parent().lock();
            if (!target_parent)
            {
                mir::log_warning("Unable to move active window: second_window has no second_parent");
                return false;
            }

            auto active_parent = active_window->get_parent().lock();
            if (active_parent == target_parent)
            {
                active_parent->swap_nodes(active_window, target_node);
                active_parent->commit_changes();
                break;
            }

            auto [first, second] = transfer_node(active_window, target_node);
            first->commit_changes();
            second->commit_changes();
            break;
        }
        case MoveResult::traversal_type_append:
        {
            auto lane_node = Node::as_lane(traversal_result.node);
            auto moving_node = active_window;
            handle_remove(moving_node);
            lane_node->graft_existing(moving_node, lane_node->num_nodes());
            lane_node->commit_changes();
            break;
        }
        case MoveResult::traversal_type_prepend:
        {
            auto lane_node = Node::as_lane(traversal_result.node);
            auto moving_node = active_window;
            handle_remove(moving_node);
            lane_node->graft_existing(moving_node, 0);
            lane_node->commit_changes();
            break;
        }
        default:
        {
            mir::log_error("Unable to move window");
            return false;
        }
    }

    return true;
}

void TilingWindowTree::request_vertical()
{
    handle_direction_change(NodeLayoutDirection::vertical);
}

void TilingWindowTree::request_horizontal()
{
    handle_direction_change(NodeLayoutDirection::horizontal);
}

void TilingWindowTree::handle_direction_change(NodeLayoutDirection direction)
{
    if (is_active_window_fullscreen)
    {
        mir::log_warning("Unable to handle direction request: fullscreen");
        return;
    }

    if (is_resizing)
    {
        mir::log_warning("Unable to handle direction request: resizing");
        return;
    }

    if (!active_window)
    {
        mir::log_warning("Unable to handle direction request: active window not set");
        return;
    }

    if (active_window->get_parent().lock()->num_nodes() != 1)
        get_active_lane()->convert_to_lane(active_window);

    get_active_lane()->set_direction(direction);
}

void TilingWindowTree::advise_focus_gained(miral::Window& window)
{
    is_resizing = false;

    auto metadata = tiling_interface.get_metadata(window, this);
    if (!metadata)
    {
        active_window = nullptr;
        return;
    }

    active_window = metadata->get_tiling_node();
    if (active_window && is_active_window_fullscreen)
        tiling_interface.raise(window);
    else
        tiling_interface.send_to_back(window);
}

void TilingWindowTree::advise_focus_lost(miral::Window& window)
{
    is_resizing = false;

    if (active_window != nullptr && active_window->get_window() == window && !is_active_window_fullscreen)
        active_window = nullptr;
}

void TilingWindowTree::advise_delete_window(miral::Window& window)
{
    auto metadata = tiling_interface.get_metadata(window, this);
    if (!metadata)
    {
        mir::log_warning("Unable to delete window: cannot find node");
        return;
    }

    auto window_node = metadata->get_tiling_node();
    if (window_node == active_window)
    {
        active_window = nullptr;
        if (is_active_window_fullscreen)
            is_active_window_fullscreen = false;
    }

    auto parent = handle_remove(window_node);
    parent->commit_changes();
}

namespace
{
std::shared_ptr<LeafNode> get_closest_window_to_select_from_node(
    std::shared_ptr<Node> node,
    miracle::Direction direction)
{
    // This function attempts to get the first window within a node provided the direction that we are coming
    // from as a hint. If the node that we want to move to has the same direction as that which we are coming
    // from, a seamless experience would mean that - at times - we select the _LAST_ node in that list, instead
    // of the first one. This makes it feel as though we are moving "across" the screen.
    if (node->is_leaf())
        return Node::as_leaf(node);

    bool is_vertical = direction == Direction::up || direction == Direction::down;
    bool is_negative = direction == Direction::up || direction == Direction::left;
    auto lane_node = Node::as_lane(node);
    if (is_vertical && lane_node->get_direction() == NodeLayoutDirection::vertical
        || !is_vertical && lane_node->get_direction() == NodeLayoutDirection::horizontal)
    {
        if (is_negative)
        {
            auto sub_nodes = lane_node->get_sub_nodes();
            for (auto i = sub_nodes.size()  - 1; i != 0; i--)
            {
                if (auto retval = get_closest_window_to_select_from_node(sub_nodes[i], direction))
                    return retval;
            }
        }
    }

    for (auto const& sub_node : lane_node->get_sub_nodes())
    {
        if (auto retval = get_closest_window_to_select_from_node(sub_node, direction))
            return retval;
    }

    return nullptr;
}
}

std::shared_ptr<LeafNode> TilingWindowTree::handle_select(
    std::shared_ptr<Node> const &from,
    Direction direction)
{
    // Algorithm:
    //  1. Retrieve the parent
    //  2. If the parent matches the target direction, then
    //     we select the next node in the direction
    //  3. If the current_node does NOT match the target direction,
    //     then we climb the tree until we find a current_node who matches
    //  4. If none match, we return nullptr
    bool is_vertical = direction == Direction::up || direction == Direction::down;
    bool is_negative = direction == Direction::up || direction == Direction::left;
    auto current_node = from;
    auto parent = current_node->get_parent().lock();
    if (!parent)
    {
        mir::log_warning("Cannot handle_select the root node");
        return nullptr;
    }

    do {
        auto grandparent_direction = parent->get_direction();
        int index = parent->get_index_of_node(current_node);
        if (is_vertical && grandparent_direction == NodeLayoutDirection::vertical
            || !is_vertical && grandparent_direction == NodeLayoutDirection::horizontal)
        {
            if (is_negative)
            {
                if (index > 0)
                    return get_closest_window_to_select_from_node(parent->at(index - 1), direction);
            }
            else
            {
                if (index < parent->num_nodes() - 1)
                    return get_closest_window_to_select_from_node(parent->at(index + 1), direction);
            }
        }

        current_node = parent;
        parent = parent->get_parent().lock();
    } while (parent != nullptr);

    return nullptr;
}

TilingWindowTree::MoveResult TilingWindowTree::handle_move(std::shared_ptr<Node>const& from, Direction direction)
{
    // Algorithm:
    //  1. Perform the _select algorithm. If that passes, then we want to be where the selected node
    //     currently is
    //  2. If our parent layout direction does not equal the root layout direction, we can append
    //     or prepend to the root
    if (auto insert_node = handle_select(from, direction))
    {
        return {
            MoveResult::traversal_type_insert,
            insert_node
        };
    }

    auto parent = from->get_parent().lock();
    if (root_lane->get_direction() != parent->get_direction())
    {
        bool is_negative = direction == Direction::left || direction == Direction::up;
        if (is_negative)
            return {
                MoveResult::traversal_type_prepend,
                root_lane
            };
        else
            return {
                MoveResult::traversal_type_append,
                root_lane
            };
    }

    return {};
}

std::shared_ptr<ParentNode> TilingWindowTree::get_active_lane()
{
    if (!active_window)
        return root_lane;

    return active_window->get_parent().lock();
}

void TilingWindowTree::handle_resize(
    std::shared_ptr<Node> const& node,
    Direction direction,
    int amount)
{
    auto parent = node->get_parent().lock();
    if (parent == nullptr)
    {
        // Can't resize, most likely the root
        return;
    }

    bool is_vertical = direction == Direction::up || direction == Direction::down;
    bool is_main_axis_movement = (is_vertical  && parent->get_direction() == NodeLayoutDirection::vertical)
                                 || (!is_vertical && parent->get_direction() == NodeLayoutDirection::horizontal);

    if (is_main_axis_movement && parent->num_nodes() == 1)
    {
        // Can't resize if we only have ourselves!
        return;
    }

    if (!is_main_axis_movement)
    {
        handle_resize(parent, direction, amount);
        return;
    }

    bool is_negative = direction == Direction::left || direction == Direction::up;
    auto resize_amount = is_negative ? -amount : amount;
    auto nodes = parent->get_sub_nodes();
    std::vector<geom::Rectangle> pending_node_resizes;
    if (is_vertical)
    {
        int height_for_others = (int)floor(-(double)resize_amount / static_cast<double>(nodes.size() - 1));
        int total_height = 0;
        for (size_t i = 0; i < nodes.size(); i++)
        {
            auto other_node = nodes[i];
            auto other_rect = other_node->get_logical_area();
            if (node == other_node)
                other_rect.size.height = geom::Height{other_rect.size.height.as_int() + resize_amount};
            else
                other_rect.size.height = geom::Height{other_rect.size.height.as_int() + height_for_others};

            if (i != 0)
            {
                auto prev_rect = pending_node_resizes[i - 1];
                other_rect.top_left.y = geom::Y{prev_rect.top_left.y.as_int() + prev_rect.size.height.as_int()};
            }

            if (other_rect.size.height.as_int() <= other_node->get_min_height())
            {
                mir::log_warning("Unable to resize a rectangle that would cause another to be negative");
                return;
            }

            total_height += other_rect.size.height.as_int();
            pending_node_resizes.push_back(other_rect);
        }

        // Due to some rounding errors, we may have to extend the final node
        int leftover_height = parent->get_logical_area().size.height.as_int() - total_height;
        pending_node_resizes.back().size.height = geom::Height{pending_node_resizes.back().size.height.as_int() + leftover_height};
    }
    else
    {
        int width_for_others = (int)floor((double)-resize_amount / static_cast<double>(nodes.size() - 1));
        int total_width = 0;
        for (size_t i = 0; i < nodes.size(); i++)
        {
            auto other_node = nodes[i];
            auto other_rect = other_node->get_logical_area();
            if (node == other_node)
                other_rect.size.width = geom::Width {other_rect.size.width.as_int() + resize_amount};
            else
                other_rect.size.width = geom::Width {other_rect.size.width.as_int() + width_for_others};

            if (i != 0)
            {
                auto prev_rect = pending_node_resizes[i - 1];
                other_rect.top_left.x = geom::X{prev_rect.top_left.x.as_int() + prev_rect.size.width.as_int()};
            }

            if (other_rect.size.width.as_int() <= other_node->get_min_width())
            {
                mir::log_warning("Unable to resize a rectangle that would cause another to be negative");
                return;
            }

            total_width += other_rect.size.width.as_int();
            pending_node_resizes.push_back(other_rect);
        }

        // Due to some rounding errors, we may have to extend the final node
        int leftover_width = parent->get_logical_area().size.width.as_int() - total_width;
        pending_node_resizes.back().size.width = geom::Width {pending_node_resizes.back().size.width.as_int() + leftover_width};
    }

    for (size_t i = 0; i < nodes.size(); i++)
    {
        nodes[i]->set_logical_area(pending_node_resizes[i]);
        nodes[i]->commit_changes();
    }
}

std::shared_ptr<ParentNode> TilingWindowTree::handle_remove(std::shared_ptr<Node> const& node)
{
    auto parent = node->get_parent().lock();
    if (parent == nullptr)
        return nullptr;

    if (parent->num_nodes() == 1 && parent->get_parent().lock())
    {
        // Remove the entire lane if this lane is now empty
        auto prev_active = parent;
        parent = parent->get_parent().lock();
        parent->remove(prev_active);
    }
    else
    {
        parent->remove(node);
    }

    return parent;
}

std::tuple<std::shared_ptr<ParentNode>, std::shared_ptr<ParentNode>> TilingWindowTree::transfer_node(std::shared_ptr<LeafNode> const& node, std::shared_ptr<Node> const& to)
{
    // We are moving the active window to a new lane
    auto to_update = handle_remove(node);

    // Note: When we remove moving_node from its initial position, there's a chance
    // that the target_lane was melted into another lane. Hence, we need to update it
    auto target_parent = to->get_parent().lock();
    auto index = target_parent->get_index_of_node(to);
    target_parent->graft_existing(node, index + 1);

    return {target_parent, to_update};
}

void TilingWindowTree::recalculate_root_node_area()
{
    for (auto const& zone : screen->get_app_zones())
    {
        root_lane->set_logical_area(zone.extents());
        break;
    }
}

bool TilingWindowTree::advise_fullscreen_window(miral::Window& window)
{
    auto node = tiling_interface.get_metadata(window, this);
    if (!node)
        return false;

    tiling_interface.select_active_window(node->get_window());
    tiling_interface.raise(node->get_window());
    is_active_window_fullscreen = true;
    is_resizing = false;
    return true;
}

bool TilingWindowTree::advise_restored_window(miral::Window& window)
{
    auto metadata = tiling_interface.get_metadata(window, this);
    if (!metadata)
        return false;

    if (metadata->get_tiling_node() == active_window && is_active_window_fullscreen)
    {
        is_active_window_fullscreen = false;
        active_window->set_logical_area(active_window->get_logical_area());
        active_window->commit_changes();
    }

    return true;
}

bool TilingWindowTree::handle_window_ready(miral::WindowInfo &window_info)
{
    auto metadata = tiling_interface.get_metadata(window_info.window(), this);
    if (!metadata)
        return false;

    if (is_active_window_fullscreen)
        return true;

    if (window_info.can_be_active())
        tiling_interface.select_active_window(window_info.window());

    constrain(window_info.window());
    return true;
}

bool TilingWindowTree::advise_state_change(miral::Window const& window, MirWindowState state)
{
    auto metadata = tiling_interface.get_metadata(window, this);
    if (!metadata)
        return false;

    if (is_hidden)
        return true;

    return true;
}

bool TilingWindowTree::confirm_placement_on_display(
    miral::Window const& window,
    MirWindowState new_state,
    mir::geometry::Rectangle &new_placement)
{
    auto metadata = tiling_interface.get_metadata(window, this);
    if (!metadata)
        return false;

    auto node = metadata->get_tiling_node();
    auto node_rectangle = node->get_visible_area();
    switch (new_state)
    {
    case mir_window_state_restored:
        new_placement = node_rectangle;
        break;
    default:
        break;
    }

    return true;
}

bool TilingWindowTree::constrain(miral::Window& window)
{
    auto metadata = tiling_interface.get_metadata(window, this);
    if (!metadata)
        return false;

    if (is_hidden)
        return false;

    auto node = metadata->get_tiling_node();
    if (node->get_parent().expired())
    {
        mir::log_error("Unable to constrain node without parent");
        return true;
    }

    node->get_parent().lock()->constrain();
    return true;
}

namespace
{
void foreach_node_internal(std::function<void(std::shared_ptr<Node>)> const& f, std::shared_ptr<Node> const& parent)
{
    f(parent);
    if (parent->is_leaf())
        return;

    for (auto& node : Node::as_lane(parent)->get_sub_nodes())
        foreach_node_internal(f, node);
}
}

void TilingWindowTree::foreach_node(std::function<void(std::shared_ptr<Node>)> const& f)
{
    foreach_node_internal(f, root_lane);
}

void TilingWindowTree::hide()
{
    if (is_hidden)
    {
        mir::log_warning("Tree is already hidden");
        return;
    }

    is_hidden = true;
    foreach_node([&](auto node)
    {
        auto leaf_node = Node::as_leaf(node);
        if (leaf_node)
        {
            leaf_node->hide();
            leaf_node->commit_changes();
        }
    });
}

void TilingWindowTree::show()
{
    if (!is_hidden)
    {
        mir::log_warning("Tree is already shown");
        return;
    }

    is_hidden = false;
    foreach_node([&](auto node)
    {
        auto leaf_node = Node::as_leaf(node);
        if (leaf_node)
        {
            leaf_node->show();
            leaf_node->commit_changes();

            if (leaf_node->is_fullscreen())
            {
                tiling_interface.select_active_window(leaf_node->get_window());
                tiling_interface.raise(leaf_node->get_window());
            }
        }
    });
}

bool TilingWindowTree::is_empty()
{
    return root_lane->num_nodes() == 0;
}
