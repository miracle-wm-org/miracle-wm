/**
Copyright (C) 2025  Matthew Kosarek

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

#define MIR_LOG_COMPONENT "leaf_container"

#include "leaf_container.h"
#include "abstract_output.h"
#include "abstract_workspace.h"
#include "compositor_state.h"
#include "config.h"
#include "container_listener.h"
#include "container_scope.h"
#include "parent_container.h"
#include "window_helpers.h"

#include <cmath>
#include <jpcre2.h>
#include <mir/log.h>
#include <mir/scene/session.h>
#include <mir/scene/surface.h>

using namespace miracle;

namespace
{
std::shared_ptr<LeafContainer> get_closest_window_to_select_from_node(
    std::shared_ptr<Container> const& node,
    Direction direction)
{
    // This function attempts to get the first window within a node provided the direction that we are coming
    // from as a hint. If the node that we want to move to has the same direction as that which we are coming
    // from, a seamless experience would mean that - at times - we select the _LAST_ node in that list, instead
    // of the first one. This makes it feel as though we are moving "across" the screen.
    if (auto const leaf = Container::as_leaf(node))
        return leaf;

    bool const is_vertical = is_vertical_direction(direction);
    bool const is_negative = is_negative_direction(direction);
    auto const lane_node = Container::as_parent(node);
    if ((is_vertical && lane_node->get_scheme() == LayoutScheme::vertical)
        || (!is_vertical && lane_node->get_scheme() == LayoutScheme::horizontal))
    {
        if (is_negative)
        {
            auto sub_nodes = lane_node->children();
            for (auto i = sub_nodes.size() - 1; i != 0; i--)
            {
                if (auto retval = get_closest_window_to_select_from_node(sub_nodes[i], direction))
                    return retval;
            }
        }
    }

    for (auto const& sub_node : lane_node->children())
    {
        if (auto retval = get_closest_window_to_select_from_node(sub_node, direction))
            return retval;
    }

    return nullptr;
}

const char* scratchpad_state_to_string(ScratchpadState state)
{
    switch (state)
    {
    case ScratchpadState::none:
        return "none";
    case ScratchpadState::fresh:
        return "fresh";
    case ScratchpadState::changed:
        return "changed";
    default:
        return "unknown";
    }
}

std::shared_ptr<ParentContainer> handle_remove_container(std::shared_ptr<Container> const& container)
{
    auto parent = Container::as_parent(container->get_parent().lock());
    if (parent == nullptr)
        return nullptr;

    if (parent->num_children() == 1 && parent->get_parent().lock())
    {
        // Remove the entire parent if this parent is now empty
        auto prev_active = parent;
        parent = Container::as_parent(parent->get_parent().lock());
        parent->remove_child(prev_active);
    }
    else
    {
        parent->remove_child(container);
    }

    return parent;
}

std::tuple<std::shared_ptr<ParentContainer>, std::shared_ptr<ParentContainer>> transfer_node(
    std::shared_ptr<Container> const& node, std::shared_ptr<Container> const& to)
{
    // When we remove [node] from its initial position, there's a chance
    // that the target_lane was melted into another lane. Hence, we need to return it
    auto to_update = handle_remove_container(node);
    auto target_parent = Container::as_parent(to->get_parent().lock());
    auto const index = target_parent->get_index_of_node(to).value();
    target_parent->add_child(node, index + 1);
    node->set_workspace(target_parent->get_workspace());

    return { target_parent, to_update };
}
}

LeafContainer::LeafContainer(
    std::shared_ptr<AbstractWorkspace> const& workspace,
    std::shared_ptr<WindowController> const& window_controller,
    geom::Rectangle area,
    std::shared_ptr<Config> const& config,
    std::shared_ptr<ParentContainer> const& parent,
    std::shared_ptr<CompositorState> const& state) :
    WindowContainer(state->next_container_id(), state->render_data_manager(), window_controller),
    window_controller {
        window_controller
    },
    config { config },
    state { state },
    workspace_ { workspace },
    logical_area_ { std::move(area) },
    parent_ { parent }
{
}

geom::Rectangle LeafContainer::get_logical_area() const
{
    return next_logical_area_ ? next_logical_area_.value() : logical_area_;
}

void LeafContainer::set_logical_area(geom::Rectangle const& target_rect, bool with_animations)
{
    next_logical_area_ = target_rect;
    next_with_animations_ = with_animations;
}

std::weak_ptr<ParentContainer> LeafContainer::get_parent() const
{
    return parent_;
}

void LeafContainer::set_parent(std::shared_ptr<ParentContainer> const& in_parent)
{
    parent_ = in_parent;

    miral::WindowSpecification spec;
    spec.depth_layer() = get_depth_layer(
        is_fullscreen());
    window_controller->modify(window_, spec);
}

void LeafContainer::set_state(MirWindowState in_state)
{
    next_state_ = in_state;
}

geom::Rectangle LeafContainer::get_visible_area() const
{
    if (!visible_area_dirty && cached_visible_area.has_value())
    {
        return cached_visible_area.value();
    }

    // TODO: Could cache these half values in the config
    // TODO: Inner gaps only support X and Y for now, but the data model has support
    //  for different gaps on all sides. That is a bit too much trouble to implement
    //  for now though.
    auto gaps = config->get_inner_gaps();
    if (auto const sh_workspace = workspace_.lock())
    {
        if (auto const workspace_gaps = sh_workspace->inner_gaps())
            gaps = *workspace_gaps;
    }
    int const half_gap_x = static_cast<int>(ceil(static_cast<double>(gaps.left) / 2.0));
    int const half_gap_y = static_cast<int>(ceil(static_cast<double>(gaps.top) / 2.0));
    auto const neighbors = get_neighbors();
    auto const logical_area = logical_area_;
    int x = logical_area.top_left.x.as_int();
    int y = logical_area.top_left.y.as_int();
    int width = logical_area.size.width.as_int();
    int height = logical_area.size.height.as_int();
    if (neighbors[std::to_underlying(Direction::left)])
    {
        x += half_gap_x;
        width -= half_gap_x;
    }
    if (neighbors[std::to_underlying(Direction::right)])
    {
        width -= half_gap_x;
    }
    if (neighbors[std::to_underlying(Direction::up)])
    {
        y += half_gap_y;
        height -= half_gap_y;
    }
    if (neighbors[std::to_underlying(Direction::down)])
    {
        height -= half_gap_y;
    }

    cached_visible_area = geom::Rectangle {
        geom::Point { x,     y      },
        geom::Size { width, height }
    };
    visible_area_dirty = false;

    return cached_visible_area.value();
}

void LeafContainer::constrain()
{
    auto const w = window_;
    if (is_fullscreen() || is_dragging_)
        window_controller->noclip(w);
    else
        window_controller->clip(w, get_visible_area());
}

size_t LeafContainer::get_min_width() const
{
    return 50;
}

size_t LeafContainer::get_min_height() const
{
    return 50;
}

void LeafContainer::handle_ready()
{
    auto const focused = state->focused_container();
    auto const window_focused = std::dynamic_pointer_cast<WindowContainer>(focused);
    auto const w = window_;

    int const border_size = config->get_border_config().size;
    auto surface = w.operator std::shared_ptr<mir::scene::Surface>();
    surface->set_window_margins(
        mir::geometry::DeltaY { border_size },
        mir::geometry::DeltaX { border_size },
        mir::geometry::DeltaY { border_size },
        mir::geometry::DeltaX { border_size });

    if (!focused || !window_focused || !window_focused->is_fullscreen())
    {
        auto& info = window_controller->info_for(w);
        if (info.can_be_active())
            window_controller->select_active_window(w);
    }
}

void LeafContainer::handle_modify(miral::WindowSpecification const& modifications, bool hidden)
{
    /// Note: This request comes from the client, so we may accept or ignore whatever
    /// it is that we find here.
    auto const w = window_;
    auto const& info = window_controller->info_for(w);
    auto mods = modifications;
    auto visible_area = get_visible_area();
    auto cur_state = window_controller->get_state(w);
    if (mods.state())
    {
        // We will not respect any request for a maximized window. Only fullscreen is valid.
        switch (mods.state().value())
        {
        case mir_window_state_maximized:
        case mir_window_state_horizmaximized:
        case mir_window_state_vertmaximized:
        case mir_window_state_minimized:
        case mir_window_state_hidden: // Hidden window requests from the client are NOT respected.
            mods.state() = mir_window_state_restored;
            break;
        default:
            break;
        }

        if (hidden)
        {
            // This container's workspace is not being rendered. Defer the state change
            // until the container is shown again (via restore_result) so the workspace
            // stays hidden; non-state modifications below are still applied immediately.
            if (restore_result_)
                restore_result_->state = mods.state().value();
            else
                restore_result_ = RestoreResult { mods.state().value() };
            window_helpers::reset_optional(mods.state());
        }
        else
        {
            cur_state = mods.state().value();
            mods.depth_layer() = get_depth_layer(
                mods.state().value() == mir_window_state_fullscreen);

            if (info.state() != mods.state().value() && mods.state().value() == mir_window_state_restored)
            {
                /// If the next state if restored, set the area and depth layer.
                mods.top_left() = visible_area.top_left;
                mods.size() = visible_area.size;
            }

            if (cur_state == mir_window_state_fullscreen
                || window_controller->get_state(w) == mir_window_state_fullscreen)
            {
                for_each_observer([this](ContainerListener* observer)
                {
                    observer->on_container_fullscreen(*this);
                });
            }

            if (cur_state == mir_window_state_fullscreen)
                window_controller->noclip(w);
            else
                window_controller->clip(w, visible_area);
        }
    }

    if (cur_state == mir_window_state_restored)
    {
        if (mods.size() && mods.size().value() != visible_area.size)
            window_helpers::reset_optional(mods.size());
        if (mods.top_left() && mods.top_left().value() != visible_area.top_left)
            window_helpers::reset_optional(mods.top_left());
    }

    window_controller->modify(w, mods);
}

void LeafContainer::handle_raise()
{
}

bool LeafContainer::resize(Direction direction, int pixels)
{
    switch (direction)
    {
    case Direction::left:
        if (neighbor_east())
            execute_resize(this, mir_resize_edge_east, -pixels, 0, true); // Shrink in east
        else if (neighbor_west())
            execute_resize(this, mir_resize_edge_west, pixels, 0, true); // Grow in west
        break;
    case Direction::right:
        if (neighbor_east())
            execute_resize(this, mir_resize_edge_east, pixels, 0, true); // Grow in east
        else if (neighbor_west())
            execute_resize(this, mir_resize_edge_west, -pixels, 0, true); // Shrink in west
        break;
    case Direction::down:
        if (neighbor_south())
            execute_resize(this, mir_resize_edge_south, 0, pixels, true); // Grow in south
        else if (neighbor_north())
            execute_resize(this, mir_resize_edge_north, 0, -pixels, true); // Shrink in north
        break;
    case Direction::up:
        if (neighbor_south())
            execute_resize(this, mir_resize_edge_south, 0, -pixels, true); // Shrink in south
        else if (neighbor_north())
            execute_resize(this, mir_resize_edge_north, 0, pixels, true); // Grow in north
        break;
    default:
        break;
    }
    return true;
}

bool LeafContainer::set_size(std::optional<int> const& width, std::optional<int> const& height)
{
    auto rectangle = get_visible_area();
    int new_width = width ? width.value() : rectangle.size.width.as_int();
    int new_height = height ? height.value() : rectangle.size.height.as_int();
    int diff_x = new_width - rectangle.size.width.as_int();
    int diff_y = new_height - rectangle.size.height.as_int();

    if (diff_x < 0)
        resize(Direction::left, -diff_x);
    else
        resize(Direction::right, diff_x);

    if (diff_y < 0)
        resize(Direction::up, -diff_y);
    else
        resize(Direction::down, diff_y);

    return true;
}

void LeafContainer::show()
{
    auto const w = window_;
    auto restore = restore_result_;
    restore_result_.reset();
    if (restore)
        window_controller->show(w, restore.value());
    window_controller->raise(w);
}

void LeafContainer::hide()
{
    auto const w = window_;
    restore_result_ = window_controller->hide(w);
    window_controller->send_to_back(w);
}

bool LeafContainer::toggle_fullscreen()
{
    {
        if (is_fullscreen())
        {
            next_state_ = mir_window_state_restored;
            next_logical_area_ = get_logical_area();
        }
        else
        {
            next_state_ = mir_window_state_fullscreen;
        }

        next_depth_layer_ = get_depth_layer(
            next_state_ == mir_window_state_fullscreen);
    }
    commit_changes();
    return true;
}

mir::geometry::Rectangle LeafContainer::confirm_placement(
    MirWindowState state, mir::geometry::Rectangle const& placement)
{
    return placement;
}

void LeafContainer::on_move_to(geom::Point const&)
{
}

bool LeafContainer::is_fullscreen() const
{
    return window_controller->get_state(window_) == mir_window_state_fullscreen;
}

void LeafContainer::commit_changes()
{
    auto const w = window_;
    auto const render_id = render_id_;

    {
        if (next_state_)
        {
            bool const entering_fs = next_state_.value() == mir_window_state_fullscreen;
            bool const leaving_fs = window_controller->get_state(w) == mir_window_state_fullscreen;

            if (entering_fs || leaving_fs)
            {
                for_each_observer([this](ContainerListener* observer)
                {
                    observer->on_container_fullscreen(*this);
                });
            }

            window_controller->change_state(w, next_state_.value());

            if (entering_fs || leaving_fs)
                update_window_margins(config->get_border_config().size, entering_fs);

            state->render_data_manager()->needs_outline_change(render_id.value(), next_state_ != mir_window_state_fullscreen);
            next_state_.reset();

            if (next_depth_layer_)
            {
                miral::WindowSpecification spec;
                spec.depth_layer() = next_depth_layer_.value();
                window_controller->modify(w, spec);
                next_depth_layer_.reset();
            }

            constrain();
            return;
        }
    }

    {
        if (next_depth_layer_)
        {
            miral::WindowSpecification spec;
            spec.depth_layer() = next_depth_layer_.value();
            window_controller->modify(w, spec);
            next_depth_layer_.reset();
        }
    }

    {
        if (next_logical_area_)
        {
            auto previous = get_visible_area();
            logical_area_ = next_logical_area_.value();
            next_logical_area_.reset();
            bool const animate = next_with_animations_;
            bool const dragging = is_dragging_;
            invalidate_visible_area_cache();
            if (!is_fullscreen() && !dragging)
            {
                auto next_visible_area = get_visible_area();
                window_controller->set_rectangle(w, previous, next_visible_area, animate);
                next_with_animations_ = true;

                for_each_observer([this](ContainerListener* observer)
                {
                    observer->on_container_moved(*this);
                });
            }
        }
    }
}

void LeafContainer::handle_request_move(MirInputEvent const* input_event)
{
}

void LeafContainer::request_horizontal_layout()
{
    handle_layout_scheme(this, LayoutScheme::horizontal);
}

void LeafContainer::request_vertical_layout()
{
    handle_layout_scheme(this, LayoutScheme::vertical);
}

void LeafContainer::toggle_layout(bool cycle_thru_all)
{
    auto sh_parent = parent_.lock();
    if (!sh_parent)
    {
        mir::log_error("toggle_layout: unable to get parent container");
        return;
    }

    if (cycle_thru_all)
        handle_layout_scheme(this, get_next_layout(sh_parent->get_scheme()));
    else
    {
        if (sh_parent->get_scheme() == LayoutScheme::horizontal)
            handle_layout_scheme(this, LayoutScheme::vertical);
        else if (sh_parent->get_scheme() == LayoutScheme::vertical)
            handle_layout_scheme(this, LayoutScheme::horizontal);
        else
            mir::log_error("Parent with stack layout scheme cannot be toggled");
    }
}

std::shared_ptr<AbstractWorkspace> LeafContainer::get_workspace() const
{
    return workspace_.lock();
}

void LeafContainer::set_workspace(std::shared_ptr<AbstractWorkspace> const& in)
{
    workspace_ = in;

    state->render_data_manager()->output_area_change(
        render_id_.value(),
        in->get_output()->get_area());
    set_workspace_transform(in->transform());
    for_each_observer([this](ContainerListener* observer)
    {
        observer->on_container_workspace_changed(*this);
    });
}

std::shared_ptr<AbstractOutput> LeafContainer::get_output() const
{
    if (workspace_.expired())
        return nullptr;
    return workspace_.lock()->get_output();
}

bool LeafContainer::is_focused() const
{
    if (state->focused_container().get() == this)
        return true;

    if (auto locked_parent = parent_.lock())
        if (locked_parent->is_focused())
            return true;

    return false;
}

bool LeafContainer::select_next(Direction direction)
{
    if (is_fullscreen())
        return false;

    auto next = handle_select(*this, direction);
    if (!next)
    {
        mir::log_warning("Unable to select the next window: handle_select failed");
        return false;
    }

    window_controller->select_active_window(next->window().value());
    return true;
}

std::shared_ptr<LeafContainer> LeafContainer::handle_select(
    Container& from,
    Direction direction)
{
    // Algorithm:
    //  1. Retrieve the parent
    //  2. If the parent matches the target direction, then
    //     we select the next node in the direction
    //  3. If the current_node does NOT match the target direction,
    //     then we climb the tree until we find a current_node who matches
    //  4. If none match, we return nullptr
    bool is_vertical = is_vertical_direction(direction);
    bool is_negative = is_negative_direction(direction);
    auto current_node = from.shared_from_this();
    auto parent = current_node->get_parent().lock();
    if (!parent)
    {
        mir::log_warning("Cannot handle_select the root node");
        return nullptr;
    }

    do
    {
        auto grandparent_direction = parent->get_scheme();
        auto index = parent->get_index_of_node(current_node).value();
        if ((is_vertical && (grandparent_direction == LayoutScheme::vertical || grandparent_direction == LayoutScheme::stacking))
            || (!is_vertical && (grandparent_direction == LayoutScheme::horizontal || grandparent_direction == LayoutScheme::tabbing)))
        {
            if (is_negative)
            {
                if (index > 0)
                    return get_closest_window_to_select_from_node(parent->at(index - 1), direction);
            }
            else
            {
                if (index < parent->num_children() - 1)
                    return get_closest_window_to_select_from_node(parent->at(index + 1), direction);
            }
        }

        current_node = parent;
        parent = Container::as_parent(parent->get_parent().lock());
    } while (parent != nullptr);

    return nullptr;
}

bool LeafContainer::pinned(bool value)
{
    if (auto sh_parent = parent_.lock())
        return sh_parent->pinned(value);
    return false;
}

bool LeafContainer::pinned() const
{
    if (auto sh_parent = parent_.lock())
        return sh_parent->pinned();
    return false;
}

bool LeafContainer::move(Direction direction)
{
    return workspace_.lock()->move_container(direction, *this);
}

bool LeafContainer::move_by(Direction, int)
{
    return false;
}

bool LeafContainer::move_by(float dx, float dy)
{
    if (auto sh_parent = parent_.lock())
        return sh_parent->move_by(dx, dy);
    return false;
}

/// Move this container to the position of the [target].
/// \returns true if the move was successful, otherwise false.
bool LeafContainer::move_to(Container& target)
{
    auto target_parent = target.get_parent().lock();
    if (!target_parent)
    {
        mir::log_warning("Unable to move active window: second_window has no second_parent");
        return false;
    }

    auto active_parent = Container::as_parent(get_parent().lock());
    if (active_parent == target_parent)
    {
        active_parent->swap_within_container(shared_from_this(), target.shared_from_this());
        active_parent->commit_changes();
        return true;
    }

    // Transfer the node to the new parent.
    auto [first, second] = transfer_node(shared_from_this(), target.shared_from_this());
    first->commit_changes();
    second->commit_changes();
    return true;
}

bool LeafContainer::move_to(int x, int y, bool with_animations)
{
    if (auto sh_parent = parent_.lock())
        return sh_parent->move_to(x, y, with_animations);
    return false;
}

bool LeafContainer::toggle_tabbing()
{
    if (auto sh_parent = parent_.lock())
    {
        if (sh_parent->get_scheme() == LayoutScheme::tabbing)
            request_horizontal_layout();
        else
            handle_layout_scheme(this, LayoutScheme::tabbing);
    }
    return true;
}

bool LeafContainer::toggle_stacking()
{
    if (auto sh_parent = parent_.lock())
    {
        if (sh_parent->get_scheme() == LayoutScheme::stacking)
            request_horizontal_layout();
        else
            handle_layout_scheme(this, LayoutScheme::stacking);
    }
    return true;
}

bool LeafContainer::drag_start()
{
    if (is_dragging_)
        mir::log_error("Attempting to start a drag when we are already dragging");

    is_dragging_ = true;
    constrain();
    return true;
}

void LeafContainer::drag(int x, int y)
{
    if (!is_dragging_)
        return;

    miral::WindowSpecification spec;
    spec.top_left() = { x, y };
    dragged_position_ = { x, y };
    window_controller->modify(window_, spec);
}

bool LeafContainer::drag_stop()
{
    auto const w = window_;
    if (!is_dragging_)
        mir::log_error("Attempting to stop a drag when we are not dragging");

    is_dragging_ = false;
    auto const dragged_pos = dragged_position_;

    miral::WindowSpecification spec;
    auto visible_area = get_visible_area();
    geom::Rectangle previous = { dragged_pos, visible_area.size };
    window_controller->set_rectangle(w, previous, visible_area);
    for_each_observer([&](ContainerListener* observer)
    {
        observer->on_container_moved(*this);
    });
    constrain();
    return true;
}

bool LeafContainer::set_layout(LayoutScheme scheme)
{
    handle_layout_scheme(this, scheme);
    return true;
}

ScratchpadState LeafContainer::scratchpad_state() const
{
    if (!parent_.expired())
        return parent_.lock()->scratchpad_state();

    return ScratchpadState::none;
}

void LeafContainer::scratchpad_state(ScratchpadState next_scratchpad_state)
{
    if (!parent_.expired())
        return parent_.lock()->scratchpad_state(next_scratchpad_state);
}

void LeafContainer::handle_layout_scheme(Container* container, LayoutScheme scheme)
{
    auto parent = container->get_parent().lock();
    if (!parent)
    {
        mir::log_warning("handle_layout_scheme: parent is not set");
        return;
    }

    // If the parent already has more than just [container] as a child AND
    // the parent is NOT a tabbing/stacking parent, then we create a new parent for this
    // single [container].
    if (parent->num_children() > 1
        && parent->get_scheme() != LayoutScheme::tabbing
        && parent->get_scheme() != LayoutScheme::stacking)
        parent = parent->convert_to_parent(container->shared_from_this());

    parent->set_layout(scheme);
}

LayoutScheme LeafContainer::get_layout() const
{
    auto sh_parent = parent_.lock().get();
    if (!sh_parent)
        return LayoutScheme::none;

    if (sh_parent->num_children() == 1)
        return sh_parent->get_layout();

    return LayoutScheme::none;
}

bool LeafContainer::matches(ContainerScope const& scope) const
{
    typedef jpcre2::select<char> jp;
    auto const w = window_;

    switch (scope.type)
    {
    case ContainerScopeType::all:
        return true;
    case ContainerScopeType::app_id:
    {
        auto const& info = window_controller->info_for(w);
        jp::Regex re;
        re.setPattern(scope.value).compile();
        return re.match(info.application_id());
    }
    case ContainerScopeType::window_type:
    {
        auto const& info = window_controller->info_for(w);
        if (scope.value == "normal")
            return info.type() == mir_window_type_normal;
        else if (scope.value == "dialog")
            return info.type() == mir_window_type_dialog;
        else if (scope.value == "utility")
            return info.type() == mir_window_type_utility;
        else if (scope.value == "toolbar")
            return info.type() == mir_window_type_decoration;
        else if (scope.value == "splash")
            return false; // Unsupported
        else if (scope.value == "menu")
            return info.type() == mir_window_type_decoration;
        else if (scope.value == "dropdown_menu")
            return info.type() == mir_window_type_menu;
        else if (scope.value == "popup_menu")
            return info.type() == mir_window_type_menu;
        else if (scope.value == "tooltip")
            return info.type() == mir_window_type_tip;
        else if (scope.value == "notification")
            return info.type() == mir_window_type_freestyle;
        return false;
    }
    case ContainerScopeType::title:
    {
        if (scope.value == "__focused__")
        {
            if (!state->focused_container())
            {
                mir::log_warning("LeafContainer::matches: title is __focused__ but nothing is focused");
                return false;
            }

            if (auto const focused_window = state->focused_container()->window())
            {
                auto const& info = window_controller->info_for(w);
                auto const& focused_info = window_controller->info_for(focused_window.value());
                return focused_info.name() == info.name();
            }
            else
            {
                mir::log_error("LeafContainer::matches: title matcher, focused container lacks a window");
                return false;
            }
        }

        auto const& info = window_controller->info_for(w);
        jp::Regex re;
        re.setPattern(scope.value).compile();
        return re.match(info.name());
    }
    case ContainerScopeType::pid:
    {
        int int_num;
        try
        {
            int_num = std::stoi(scope.value);
        }
        catch (const std::invalid_argument& e)
        {
            mir::log_error("Invalid argument: %s", e.what());
            return false;
        }
        catch (const std::out_of_range& e)
        {
            mir::log_error("Out of range: %s", e.what());
            return false;
        }

        auto const& app = window_controller->app_info(w);
        return app.application()->process_id() == int_num;
    }
    case ContainerScopeType::workspace:
    {
        if (scope.value == "__focused__")
        {
            if (state->focused_container() == nullptr)
            {
                mir::log_warning("LeafContainer::matches: workspace is __focused__ but nothing is focused");
                return false;
            }

            return workspace_.lock() == state->focused_container()->get_workspace();
        }

        jp::Regex re;
        re.setPattern(scope.value).compile();
        return !workspace_.expired() && re.match(workspace_.lock()->display_name());
    }
    case ContainerScopeType::con_id:
    {
        std::uintptr_t int_num;
        if (scope.value == "__focused__")
        {
            if (state->focused_container() == nullptr)
            {
                mir::log_warning("LeafContainer::matches: con_id is __focused__ but nothing is focused");
                return false;
            }

            int_num = reinterpret_cast<std::uintptr_t>(state->focused_container().get());
        }
        else
        {
            try
            {
                int_num = std::stoul(scope.value);
            }
            catch (const std::invalid_argument& e)
            {
                mir::log_error("Invalid argument: %s", e.what());
                return false;
            }
            catch (const std::out_of_range& e)
            {
                mir::log_error("Out of range: %s", e.what());
                return false;
            }
        }

        auto const id = reinterpret_cast<std::uintptr_t>(this);
        return int_num == id;
    }
    case ContainerScopeType::floating:
        return false;
    case ContainerScopeType::tiling:
        return true;
    case ContainerScopeType::con_mark:
    {
        jp::Regex re;
        re.setPattern(scope.value).compile();
        for (auto const& mark : get_marks())
        {
            if (re.match(mark))
                return true;
        }

        return false;
    }
    case ContainerScopeType::floating_from:
    case ContainerScopeType::tiling_from:
        mir::log_error("Unsupported because these are mostly useless");
        return false;
    case ContainerScopeType::urgent:
    case ContainerScopeType::class_:
    case ContainerScopeType::id:
    case ContainerScopeType::window_role:
    case ContainerScopeType::instance:
    case ContainerScopeType::machine:
        mir::log_error("Unsupported because this is an X11 value");
        return false;
    default:
        return false;
    }
}

MirDepthLayer LeafContainer::get_depth_layer(bool is_fullscreen)
{
    if (is_fullscreen)
        return mir_depth_layer_above;
    else
        return mir_depth_layer_application;
}

void LeafContainer::invalidate_visible_area_cache()
{
    visible_area_dirty = true;
    cached_visible_area.reset();
}

nlohmann::json LeafContainer::to_json(bool is_workspace_visible) const
{
    auto const w = window_;
    auto const app = w.application();
    auto const& win_info = window_controller->info_for(w);
    auto visible_area = get_visible_area();
    auto locked_parent = parent_.lock();
    auto const logical_area = logical_area_;
    bool visible = true;

    if (!is_workspace_visible)
        visible = false;

    if (locked_parent == nullptr)
        visible = false;
    else if ((locked_parent->get_scheme() == LayoutScheme::stacking || locked_parent->get_scheme() == LayoutScheme::tabbing) && !is_focused())
        visible = false;

    nlohmann::json properties = nlohmann::json::object();
    return {
        { "id",                   reinterpret_cast<std::uintptr_t>(this)         },
        { "name",                 app->name()                                    },
        { "rect",                 {
                      { "x", logical_area.top_left.x.as_int() },
                      { "y", logical_area.top_left.y.as_int() },
                      { "width", logical_area.size.width.as_int() },
                      { "height", logical_area.size.height.as_int() },
                  }                                             },
        { "focused",              visible && is_focused()                        },
        { "focus",                std::vector<int>()                             },
        { "border",               "normal"                                       },
        { "current_border_width", config->get_border_config().size               },
        { "layout",               "none"                                         },
        { "orientation",          "none"                                         },
        { "percent",              get_percent_of_parent()                        },
        { "window_rect",          {
                             { "x", visible_area.top_left.x.as_int() - logical_area.top_left.x.as_int() },
                             { "y", visible_area.top_left.y.as_int() - logical_area.top_left.y.as_int() },
                             { "width", visible_area.size.width.as_int() },
                             { "height", visible_area.size.height.as_int() },
                         }                               },
        { "deco_rect",            {
                           { "x", 0 },
                           { "y", 0 },
                           { "width", logical_area.size.width.as_int() },
                           { "height", logical_area.size.height.as_int() },
                       }                                   },
        { "geometry",             {
                          { "x", 0 },
                          { "y", 0 },
                          { "width", logical_area.size.width.as_int() },
                          { "height", logical_area.size.height.as_int() },
                      }                                     },
        { "window",               reinterpret_cast<std::uintptr_t>(this)         },
        { "urgent",               false                                          },
        { "floating_nodes",       std::vector<int>()                             },
        { "sticky",               false                                          },
        { "type",                 "con"                                          },
        { "fullscreen_mode",      is_fullscreen() ? 1 : 0                        }, // TODO: Support value 2
        { "pid",                  app->process_id()                              },
        { "app_id",               win_info.application_id()                      },
        { "visible",              visible                                        },
        { "shell",                "miracle-wm"                                   }, // TODO
        { "inhibit_idle",         false                                          },
        { "idle_inhibitors",      {
                                 { "application", "none" },
                                 { "user", "visible" },
                             }                       },
        { "window_properties",    properties                                     }, // TODO
        { "nodes",                std::vector<int>()                             },
        { "scratchpad_state",     scratchpad_state_to_string(scratchpad_state()) },
    };
}
