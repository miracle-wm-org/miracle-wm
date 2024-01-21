#define MIR_LOG_COMPONENT "window_tree"

#include "window_tree.h"
#include "window_helpers.h"
#include <memory>
#include <mir/log.h>
#include <iostream>
#include <cmath>

using namespace miracle;

namespace
{
bool point_in_rect(geom::Rectangle const& area, int x, int y)
{
    return x >= area.top_left.x.as_int() && x < area.top_left.x.as_int() + area.size.width.as_int()
           && y >= area.top_left.y.as_int() && y < area.top_left.y.as_int() + area.size.height.as_int();
}
}

WindowTree::WindowTree(
    geom::Rectangle const& default_area,
    miral::WindowManagerTools const& tools,
    WindowTreeOptions const& options)
    : root_lane{std::make_shared<Node>(
        tools,
        std::move(geom::Rectangle{default_area.top_left, default_area.size}),
        options.gap_x, options.gap_y)},
      tools{tools},
      area{default_area},
      options{options}
{
}

miral::WindowSpecification WindowTree::allocate_position(const miral::WindowSpecification &requested_specification)
{
    miral::WindowSpecification new_spec = requested_specification;
    new_spec.min_width() = geom::Width{0};
    new_spec.max_width() = geom::Width{std::numeric_limits<int>::max()};
    new_spec.min_height() = geom::Height{0};
    new_spec.max_height() = geom::Height{std::numeric_limits<int>::max()};
    auto rect = _get_active_lane()->create_new_node_position();
    new_spec.size() = rect.size;
    new_spec.top_left() = rect.top_left;

    if (new_spec.state().is_set() && window_helpers::is_window_fullscreen(new_spec.state().value()))
    {
        // Don't start anyone in fullscreen mode
        new_spec.state() = mir::optional_value<MirWindowState>();
    }
    return new_spec;
}

void WindowTree::advise_new_window(miral::WindowInfo const& window_info)
{
    _get_active_lane()->add_window(window_info.window());
    if (window_helpers::is_window_fullscreen(window_info.state()))
    {
        tools.select_active_window(window_info.window());
        advise_fullscreen_window(window_info);
    }
}

void WindowTree::toggle_resize_mode()
{
    is_resizing = !is_resizing;
}

bool WindowTree::try_resize_active_window(miracle::Direction direction)
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

    // TODO: We have a hardcoded resize amount
    _handle_resize_request(active_window, direction, 50);
    return true;
}

bool WindowTree::try_select_next(miracle::Direction direction)
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

    auto node = _traverse(active_window, direction);
    if (!node)
    {
        mir::log_warning("Unable to select the next window: _traverse failed");
        return false;
    }

    tools.select_active_window(node->get_window());
    return true;
}

void WindowTree::set_output_area(geom::Rectangle const& new_area)
{
    double x_scale = static_cast<double>(new_area.size.width.as_int()) / static_cast<double>(area.size.width.as_int());
    double y_scale = static_cast<double>(new_area.size.height.as_int()) / static_cast<double>(area.size.height.as_int());

    int position_diff_x = new_area.top_left.x.as_int() - area.top_left.x.as_int();
    int position_diff_y = new_area.top_left.y.as_int() - area.top_left.y.as_int();
    area.top_left = new_area.top_left;
    area.size = geom::Size{
        geom::Width{ceil(area.size.width.as_int() * x_scale)},
        geom::Height {ceil(area.size.height.as_int() * y_scale)}};

    root_lane->scale_area(x_scale, y_scale);
    root_lane->translate_by(position_diff_x, position_diff_y);
}

bool WindowTree::point_is_in_output(int x, int y)
{
    return point_in_rect(area, x, y);
}

bool WindowTree::select_window_from_point(int x, int y)
{
    if (is_active_window_fullscreen)
    {
        tools.select_active_window(active_window->get_window());
        return true;
    }

    auto node = root_lane->find_where([&](std::shared_ptr<Node> const& node)
    {
        return node->is_window() && point_in_rect(node->get_logical_area(), x, y);
    });
    if (!node)
        return false;

    if (active_window != node)
        tools.select_active_window(node->get_window());
    return true;
}

bool WindowTree::try_move_active_window(miracle::Direction direction)
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

    auto second_window = _traverse(active_window, direction);
    if (!second_window)
    {
        mir::log_warning("Unable to move active window: second_window not found");
        return false;
    }

    auto second_parent = second_window->get_parent();
    if (!second_parent)
    {
        mir::log_warning("Unable to move active window: second_window has no second_parent");
        return false;
    }

    auto first_parent = active_window->get_parent();
    if (first_parent == second_parent)
    {
        first_parent->swap_nodes(active_window, second_window);
    }
    else
    {
        auto index = second_parent->get_index_of_node(second_window);
        auto moving_node = active_window;
        first_parent->remove_node(moving_node);
        second_parent->insert_node(moving_node, index + 1);
    }
    return true;
}

void WindowTree::request_vertical()
{
    _handle_direction_request(NodeLayoutDirection::vertical);
}

void WindowTree::request_horizontal()
{
    _handle_direction_request(NodeLayoutDirection::horizontal);
}

void WindowTree::_handle_direction_request(NodeLayoutDirection direction)
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

    if (active_window->get_parent()->num_nodes() != 1)
        active_window = active_window->to_lane();

    _get_active_lane()->set_direction(direction);
}

void WindowTree::advise_focus_gained(miral::Window& window)
{
    is_resizing = false;

    auto found_node = root_lane->find_node_for_window(window);
    if (!found_node)
    {
        active_window = nullptr;
        return;
    }

    active_window = found_node;
}

void WindowTree::advise_focus_lost(miral::Window& window)
{
    is_resizing = false;

    if (active_window != nullptr && active_window->get_window() == window)
        active_window = nullptr;
}

void WindowTree::advise_delete_window(miral::Window& window)
{
    auto window_node = root_lane->find_node_for_window(window);
    if (!window_node)
    {
        mir::log_warning("Unable to delete window: cannot find node");
        return;
    }

    if (window_node == active_window)
    {
        active_window = nullptr;
        if (is_active_window_fullscreen)
            is_active_window_fullscreen = false;
    }

    auto parent = window_node->get_parent();
    if (!parent)
    {
        mir::log_warning("Unable to delete window: node does not have a parent");
        return;
    }

    if (parent->num_nodes() == 1 && parent->get_parent())
    {
        // Remove the entire lane if this lane is now empty
        auto prev_active = parent;
        parent = parent->get_parent();
        parent->remove_node(prev_active);
    }
    else
    {
        // Remove the window from the active lane
        parent->remove_node(window_node);
    }
}

std::shared_ptr<Node> WindowTree::_traverse(std::shared_ptr<Node>const& from, Direction direction)
{
    if (!from->get_parent())
    {
        mir::log_warning("Cannot _traverse the root node");
        return nullptr;
    }

    auto parent = from->get_parent();
    int index = parent->get_index_of_node(from);
    auto parent_direction = parent->get_direction();

    bool is_vertical = direction == Direction::up || direction == Direction::down;
    bool is_negative = direction == Direction::up || direction == Direction::left;

    if (is_vertical && parent_direction == NodeLayoutDirection::vertical
        || !is_vertical && parent_direction == NodeLayoutDirection::horizontal)
    {
        // Simplest case: we're within a lane
        if (is_negative)
        {
            if (index == 0)
                goto grandparent_route;  // TODO: lazy lazy for readability
            else
                return parent->node_at(index - 1);
        }
        else
        {
            if (index == parent->num_nodes() - 1)
                goto grandparent_route;  // TODO: lazy lazy for readability
            else
                return parent->node_at(index + 1);
        }
    }
    else
    {
grandparent_route:
        // Harder case: we need to jump to another lane. The best thing to do here is to
        // find the first ancestor that matches the direction that we want to travel in.
        // If  that ancestor cannot be found, then we throw up our hands.
        auto grandparent = parent->get_parent();
        if (!grandparent)
        {
            mir::log_warning("Parent lane lacks a grandparent. It should AT LEAST be root");
            return nullptr;
        }

        do {
            auto index_of_parent = grandparent->get_index_of_node(parent);
            if (is_negative)
                index_of_parent--;
            else
                index_of_parent++;

            if (grandparent->get_direction() == NodeLayoutDirection::horizontal && !is_vertical
                || grandparent->get_direction() == NodeLayoutDirection::vertical && is_vertical)
            {
                return grandparent->find_nth_window_child(index_of_parent);
            }

            parent = grandparent;
            grandparent = grandparent->get_parent();
        } while (grandparent != nullptr);
    }


    return nullptr;
}

std::shared_ptr<Node> WindowTree::_get_active_lane()
{
    if (!active_window)
        return root_lane;

    return active_window->get_parent();
}

void WindowTree::_handle_resize_request(
    std::shared_ptr<Node> const& node,
    Direction direction,
    int amount)
{
    auto parent = node->get_parent();
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
        _handle_resize_request(parent, direction, amount);
        return;
    }

    bool is_negative = direction == Direction::left || direction == Direction::up;
    auto resize_amount = is_negative ? -amount : amount;
    auto nodes = parent->get_sub_nodes();
    std::vector<geom::Rectangle> pending_node_resizes;
    if (is_vertical)
    {
        int height_for_others = (int)floorf32(-(float)resize_amount / static_cast<float>(nodes.size() - 1));
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

            pending_node_resizes.push_back(other_rect);
        }
    }
    else
    {
        int width_for_others = (int)floorf((float)-resize_amount / static_cast<float>(nodes.size() - 1));
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

            pending_node_resizes.push_back(other_rect);
        }
    }

    for (size_t i = 0; i < nodes.size(); i++)
    {
        nodes[i]->set_logical_area(pending_node_resizes[i]);
    }
}

void WindowTree::advise_application_zone_create(miral::Zone const& application_zone)
{
    if (application_zone.extents().contains(area))
    {
        application_zone_list.push_back(application_zone);
        _recalculate_root_node_area();
    }
}

void WindowTree::advise_application_zone_update(miral::Zone const& updated, miral::Zone const& original)
{
    for (auto& zone : application_zone_list)
        if (zone == original)
        {
            zone = updated;
            _recalculate_root_node_area();
            break;
        }
}

void WindowTree::advise_application_zone_delete(miral::Zone const& application_zone)
{
    if (std::remove(application_zone_list.begin(), application_zone_list.end(), application_zone) != application_zone_list.end())
    {
        _recalculate_root_node_area();
    }
}

void WindowTree::_recalculate_root_node_area()
{
    // TODO: We don't take care of multiple application zones, so maybe that has to do with multiple outputs?
    for (auto const& zone : application_zone_list)
    {
        root_lane->set_logical_area(zone.extents());
        break;
    }
}

bool WindowTree::advise_fullscreen_window(miral::WindowInfo const& window_info)
{
    auto node = root_lane->find_node_for_window(window_info.window());
    if (!node)
        return false;

    tools.select_active_window(node->get_window());
    is_active_window_fullscreen = true;
    is_resizing = false;
    return true;
}

bool WindowTree::advise_restored_window(miral::WindowInfo const& window_info)
{
    auto node = root_lane->find_node_for_window(window_info.window());
    if (!node)
        return false;

    if (node == active_window && is_active_window_fullscreen)
        is_active_window_fullscreen = false;

    return true;
}

bool WindowTree::handle_window_ready(miral::WindowInfo &window_info)
{
    auto node = root_lane->find_node_for_window(window_info.window());
    if (!node)
        return false;

    if (is_active_window_fullscreen)
        return true;

    if (window_info.can_be_active())
        tools.select_active_window(window_info.window());

    constrain(window_info);
    return true;
}

bool WindowTree::advise_state_change(const miral::WindowInfo &window_info, MirWindowState state)
{
    auto node = root_lane->find_node_for_window(window_info.window());
    if (!node)
        return false;

    switch (state)
    {
        case mir_window_state_restored:
            if (auto parent = node->get_parent())
            {
                parent->restore(node);
            }
            break;
        case mir_window_state_hidden:
        case mir_window_state_minimized:
            if (auto parent = node->get_parent())
            {
                parent->minimize(node);
            }
            break;
        default:
            break;
    }

    return true;
}

bool WindowTree::confirm_placement_on_display(
    const miral::WindowInfo &window_info,
    MirWindowState new_state,
    mir::geometry::Rectangle &new_placement)
{
    auto node = root_lane->find_node_for_window(window_info.window());
    if (!node)
        return false;

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

bool WindowTree::constrain(miral::WindowInfo &window_info)
{
    auto node = root_lane->find_node_for_window(window_info.window());
    if (!node)
        return false;

    if (!node->get_parent())
    {
        std::cerr << "Unable to constrain node without parent\n";
        return true;
    }

    node->get_parent()->constrain();
    return true;
}

void WindowTree::add_tree(WindowTree& other_tree)
{
    other_tree.foreach_node([&](auto node)
    {
        if (node->is_window())
        {
            auto new_node_position = root_lane->create_new_node_position();
            node->set_logical_area(new_node_position);
            root_lane->add_window(node->get_window());
        }
    });
}

namespace
{
void foreach_node_internal(std::function<void(std::shared_ptr<Node>)> f, std::shared_ptr<Node> parent)
{
    f(parent);
    if (parent->is_window())
        return;

    for (auto node : parent->get_sub_nodes())
        foreach_node_internal(f, node);
}
}

void WindowTree::foreach_node(std::function<void(std::shared_ptr<Node>)> f)
{
    foreach_node_internal(f, root_lane);
}