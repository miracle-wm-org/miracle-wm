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

#define MIR_LOG_COMPONENT "parent_container"
#include "parent_container.h"
#include "compositor_state.h"
#include "config.h"
#include "container.h"
#include "feature_flags.h"
#include "leaf_container.h"
#include "output_interface.h"
#include "shell_application_manager.h"
#include "tiling_algorithms.h"
#include "workspace_interface.h"
#include <cmath>
#include <mir/log.h>

#include "auto_restarting_launcher.h"

using namespace miracle;

namespace
{
geom::Rectangle get_background_area(geom::Rectangle const& area)
{
    const int padding = 8;
    return geom::Rectangle(
        geom::Point(
            area.top_left.x.as_int() - padding,
            area.top_left.y.as_int() - padding),
        geom::Size(
            area.size.width.as_int() + 2 * padding,
            area.size.height.as_int() + 2 * padding));
}
};

ParentContainer::ParentContainerBackgroundPositioner::ParentContainerBackgroundPositioner(ParentContainer* parent) :
    parent { parent }
{
}

void ParentContainer::ParentContainerBackgroundPositioner::place_window(miral::WindowSpecification& specification)
{
    specification.focus_mode() = mir_focus_mode_disabled;
}

void ParentContainer::ParentContainerBackgroundPositioner::handle_ready(std::shared_ptr<Container> const& in)
{
    container = in;
    in->set_logical_area(get_background_area(parent->get_logical_area()), false);
}

void ParentContainer::ParentContainerBackgroundPositioner::set_area(mir::geometry::Rectangle const& area)
{
    if (auto const sh_container = container.lock())
    {
        sh_container->set_logical_area(get_background_area(area), false);
    }
}

ParentContainer::ParentContainer(
    std::shared_ptr<ShellApplicationManager> const& shell_application_manager,
    std::shared_ptr<CompositorState> const& state,
    std::shared_ptr<WindowController> const& window_controller,
    std::shared_ptr<Config> const& config,
    geom::Rectangle area,
    std::shared_ptr<WorkspaceInterface> const& workspace,
    std::shared_ptr<ParentContainer> const& parent,
    bool is_anchored) :
    shell_application_manager { shell_application_manager },
    state { state },
    window_controller { window_controller },
    config { config },
    logical_area { std::move(area) },
    workspace { workspace },
    parent { parent },
    scheme { config->get_default_layout_scheme() },
    is_anchored { is_anchored }
{
    // Parents typically hold around 2 to 8 sub containers inside
    // of them. Reserving at least 4 spots is a relatively safe
    // optimization.
    container_list.reserve(4);
    update_background_client_area();
}

ParentContainer::~ParentContainer()
{
    try_remove_background_client();
}

geom::Rectangle ParentContainer::get_area() const
{
    return logical_area;
}

void ParentContainer::try_remove_background_client()
{
    if (shell_application_id)
    {
        shell_application_manager->stop(shell_application_id.value());
        shell_application_id.reset();
    }
    shell_application_positioner.reset();
}

void ParentContainer::update_background_client_area()
{
    try_remove_background_client();
    if (parent.expired() && !is_anchored && feature::parent_container_wallpapers)
    {
        // Start up the internal client that will display the background for this floating parent
        mir::log_info("Spawning ParentBackgroundInternalClient for unanchored root parent");
        auto const positioner = std::make_shared<ParentContainerBackgroundPositioner>(this);
        shell_application_id = shell_application_manager->spawn(ShellApplicationRole::parent_container_background, positioner);
        shell_application_positioner = positioner;
    }
}

geom::Rectangle ParentContainer::get_logical_area() const
{
    // Unanchored parents should not employ outer gaps in their layout.
    if (parent.lock() == nullptr && is_anchored)
    {
        auto outer_gaps = config->get_outer_gaps();
        if (auto sh_workspace = workspace.lock())
        {
            if (auto const workspace_outer_gaps = sh_workspace->outer_gaps())
                outer_gaps = *workspace_outer_gaps;
        }

        return geom::Rectangle(
            geom::Point(
                logical_area.top_left.x.as_int() + static_cast<int>(outer_gaps.left),
                logical_area.top_left.y.as_int() + static_cast<int>(outer_gaps.top)),
            geom::Size(
                logical_area.size.width.as_int() - static_cast<int>(outer_gaps.left + outer_gaps.right),
                logical_area.size.height.as_int() - static_cast<int>(outer_gaps.top + outer_gaps.bottom)));
    }

    return logical_area;
}

geom::Rectangle ParentContainer::get_visible_area() const
{
    return get_logical_area();
}

size_t ParentContainer::num_nodes() const
{
    return container_list.size();
}

geom::Rectangle ParentContainer::create_space(std::optional<size_t> index)
{
    auto const placement_area = get_logical_area();
    geom::Rectangle pending_logical_rect;
    auto const pending_index = index.value_or(container_list.size());
    if (scheme == LayoutScheme::horizontal)
        pending_logical_rect = insert_node<false>(container_list,
            placement_area,
            pending_index);
    else if (scheme == LayoutScheme::vertical)
        pending_logical_rect = insert_node<true>(container_list,
            placement_area,
            pending_index);
    else if (scheme == LayoutScheme::tabbing || scheme == LayoutScheme::stacking)
        pending_logical_rect = placement_area;
    else
        mir::fatal_error("Invalid scheme during create_space");

    return pending_logical_rect;
}

miral::WindowSpecification ParentContainer::place_new_window(
    miral::WindowSpecification const& requested_specification,
    std::optional<size_t> index)
{
    if (!index)
    {
        for (size_t i = 0; i < container_list.size(); i++)
        {
            if (container_list[i] == state->focused_container())
            {
                index = i + 1;
                break;
            }
        }
    }

    auto const container = create_space_for_window(index);
    auto const rect = container->get_visible_area();

    miral::WindowSpecification new_spec = requested_specification;
    new_spec.server_side_decorated() = false;
    new_spec.min_width() = geom::Width { 0 };
    new_spec.max_width() = geom::Width { std::numeric_limits<int>::max() };
    new_spec.min_height() = geom::Height { 0 };
    new_spec.max_height() = geom::Height { std::numeric_limits<int>::max() };
    new_spec.size() = rect.size;
    new_spec.top_left() = rect.top_left;

    if (new_spec.state().is_set())
    {
        // We will not respect any request for a maximized window. Only fullscreen is valid.
        switch (new_spec.state().value())
        {
        case mir_window_state_maximized:
        case mir_window_state_horizmaximized:
        case mir_window_state_vertmaximized:
        case mir_window_state_minimized:
            new_spec.state() = mir_window_state_restored;
            break;
        default:
            break;
        }
    }

    new_spec.depth_layer() = LeafContainer::get_depth_layer(
        new_spec.state().is_set() && new_spec.state() == mir_window_state_fullscreen,
        anchored());

    return new_spec;
}

std::shared_ptr<LeafContainer> ParentContainer::create_space_for_window(std::optional<size_t> pending_index)
{
    auto const index = pending_index.value_or(container_list.size());
    pending_node = std::make_shared<LeafContainer>(
        workspace.lock(),
        window_controller,
        create_space(index),
        config,
        as_parent(shared_from_this()),
        state);
    container_list.insert(container_list.begin() + static_cast<std::vector<std::shared_ptr<Container>>::difference_type>(index), pending_node);
    return pending_node;
}

std::shared_ptr<Container> ParentContainer::confirm_window(miral::Window const& window)
{
    if (pending_node == nullptr)
    {
        mir::log_error("confirm_window: create_space_for_window wasn't called, so we will call it, but this is odd!");
        pending_node = create_space_for_window(-1);
    }

    mir::log_debug("Parent on workspace %s receiving new window", !workspace.expired() ? workspace.lock()->display_name().c_str() : "nullptr");
    auto retval = pending_node;
    retval->associate_to_window(window);
    retval->set_parent(as_parent(shared_from_this()));
    pending_node = nullptr;
    commit_changes();
    return retval;
}

void ParentContainer::graft_existing(std::shared_ptr<Container> const& node, int index)
{
    auto const rectangle = create_space(index);
    node->set_parent(as_parent(shared_from_this()));
    node->set_workspace(workspace.lock());
    node->set_logical_area(rectangle, true);
    container_list.insert(container_list.begin() + index, node);
    relayout();
    constrain();
}

std::shared_ptr<ParentContainer> ParentContainer::convert_to_parent(std::shared_ptr<Container> const& container)
{
    auto const index = get_index_of_node(container);
    if (!index.has_value())
    {
        mir::fatal_error("Attempting to convert a node to lane with an incorrect parent");
        return nullptr;
    }

    auto new_parent_node = std::make_shared<ParentContainer>(
        shell_application_manager,
        state,
        window_controller,
        config,
        container->get_logical_area(),
        workspace.lock(),
        Container::as_parent(shared_from_this()),
        true);
    new_parent_node->container_list.push_back(container);
    container->set_parent(new_parent_node);
    container_list[index.value()] = new_parent_node;
    return new_parent_node;
}

void ParentContainer::set_logical_area(const geom::Rectangle& target_rect, bool with_animations)
{
    // We are setting the size of the lane, but each window might have an idea of how
    // its own height relates to the lane (e.g. I take up 300px of 900px lane while my
    // neighbor takes up the remaining 600px, horizontally).
    // We need to look at the target dimension and scale everyone relative to that.
    // However, the "non-main-axis" dimension will be consistent across each node.
    auto current_logical_area = get_logical_area();
    logical_area = target_rect;
    auto target_placement_area = get_logical_area();

    if (auto const background_positioner_sh = shell_application_positioner.lock())
    {
        background_positioner_sh->set_area(target_placement_area);
    }

    std::vector<geom::Rectangle> pending_size_updates;
    pending_size_updates.reserve(container_list.size());
    if (scheme == LayoutScheme::horizontal)
    {
        int total_width = 0;
        for (size_t idx = 0; idx < container_list.size(); idx++)
        {
            auto item = container_list[idx];
            auto item_rect = item->get_logical_area();
            double percent_width_taken = (double)item_rect.size.width.as_int() / (double)current_logical_area.size.width.as_int();
            int new_width = (int)ceil((double)target_placement_area.size.width.as_int() * percent_width_taken);

            geom::Rectangle new_item_rect;
            new_item_rect.size = geom::Size {
                geom::Width { new_width },
                target_placement_area.size.height
            };
            if (idx == 0)
            {
                new_item_rect.top_left = geom::Point {
                    target_placement_area.top_left.x,
                    target_placement_area.top_left.y
                };
            }
            else
            {
                auto const& prev_rect = pending_size_updates[idx - 1];
                new_item_rect.top_left = geom::Point {
                    geom::X { prev_rect.top_left.x.as_int() + prev_rect.size.width.as_int() },
                    target_placement_area.top_left.y
                };
            }

            pending_size_updates.push_back(new_item_rect);
            total_width += new_width;
        }

        if (!pending_size_updates.empty())
        {
            int leftover_width = target_placement_area.size.width.as_int() - total_width;
            pending_size_updates.back().size.width = geom::Width { pending_size_updates.back().size.width.as_int() + leftover_width };
        }
    }
    else if (scheme == LayoutScheme::vertical)
    {
        int total_height = 0;
        for (size_t idx = 0; idx < container_list.size(); idx++)
        {
            auto item = container_list[idx];
            auto item_rect = item->get_logical_area();
            double percent_height_taken = static_cast<double>(item_rect.size.height.as_int()) / current_logical_area.size.height.as_int();
            int new_height = (int)floor((double)target_placement_area.size.height.as_int() * percent_height_taken);

            geom::Rectangle new_item_rect;
            new_item_rect.size = geom::Size {
                target_placement_area.size.width,
                geom::Height { new_height },
            };
            if (idx == 0)
            {
                new_item_rect.top_left = geom::Point {
                    target_placement_area.top_left.x,
                    target_placement_area.top_left.y
                };
            }
            else
            {
                auto const& prev_rect = pending_size_updates[idx - 1];
                new_item_rect.top_left = geom::Point {
                    target_placement_area.top_left.x,
                    geom::Y { prev_rect.top_left.y.as_int() + prev_rect.size.height.as_int() },
                };
            }

            pending_size_updates.push_back(new_item_rect);
            total_height += new_height;
        }

        if (!pending_size_updates.empty())
        {
            int leftover_height = target_placement_area.size.height.as_int() - total_height;
            pending_size_updates.back().size.height = geom::Height { pending_size_updates.back().size.height.as_int() + leftover_height };
        }
    }
    else if (scheme == LayoutScheme::tabbing || scheme == LayoutScheme::stacking)
    {
        for (size_t idx = 0; idx < container_list.size(); idx++)
        {
            pending_size_updates.push_back(target_placement_area);
        }
    }
    else
    {
        mir::log_error("Cannot set_logical_area with invalid scheme");
    }

    for (size_t i = 0; i < container_list.size(); i++)
    {
        container_list[i]->set_logical_area(pending_size_updates[i], with_animations);
    }
}

void ParentContainer::commit_changes()
{
    for (auto& node : container_list)
        node->commit_changes();
}

std::shared_ptr<Container> ParentContainer::at(size_t i) const
{
    if (i >= num_nodes())
        return nullptr;

    return container_list[i];
}

std::shared_ptr<LeafContainer> ParentContainer::get_nth_window(size_t i) const
{
    if (i >= container_list.size())
        return nullptr;

    if (container_list[i]->is_leaf())
        return as_leaf(container_list[i]);

    // The lane is correct, so let's get the first window in that lane.
    return as_parent(container_list[i])->get_nth_window(0);
}

std::shared_ptr<Container> ParentContainer::find_where(std::function<bool(std::shared_ptr<Container> const&)> func) const
{
    for (auto node : container_list)
        if (func(node))
            return node;

    for (auto const& node : container_list)
    {
        if (node->is_lane())
        {
            if (auto retval = as_parent(node)->find_where(func))
                return retval;
        }
    }

    return nullptr;
}

std::vector<std::shared_ptr<Container>> const& ParentContainer::get_sub_nodes() const
{
    return container_list;
}

void ParentContainer::swap_within_container(std::shared_ptr<Container> const& first, std::shared_ptr<Container> const& second)
{
    auto const first_opt = get_index_of_node(first);
    if (!first_opt)
    {
        mir::log_error("Cannot find first index of container to swap with, meaning that it is not in this container");
        return;
    }

    auto const second_opt = get_index_of_node(second);
    if (!second_opt)
    {
        mir::log_error("Cannot find second index of container to swap with, meaning that it is not in this container");
        return;
    }

    auto const first_index = first_opt.value();
    auto const second_index = second_opt.value();

    container_list[second_index] = first;
    container_list[first_index] = second;
    relayout();
    constrain();
}

void ParentContainer::remove(const std::shared_ptr<Container>& node)
{
    container_list.erase(
        std::remove_if(container_list.begin(), container_list.end(), [&](std::shared_ptr<Container> const& content)
    {
        return content == node;
    }),
        container_list.end());

    // If we have one child AND it is a lane, THEN we can absorb all of it's children
    if (container_list.size() == 1 && container_list[0]->is_lane())
    {
        auto dying_lane = as_parent(container_list[0]);
        container_list.clear();
        for (auto const& sub_node : dying_lane->get_sub_nodes())
        {
            container_list.push_back(sub_node);
            sub_node->set_parent(as_parent(shared_from_this()));
        }
        set_layout(dying_lane->get_direction());
    }

    relayout();
}

std::optional<size_t> ParentContainer::get_index_of_node(Container const* node) const
{
    for (size_t i = 0; i < container_list.size(); i++)
        if (container_list[i].get() == node)
            return i;

    return std::nullopt;
}

std::optional<size_t> ParentContainer::get_index_of_node(std::shared_ptr<Container> const& node) const
{
    return get_index_of_node(node.get());
}

std::optional<size_t> ParentContainer::get_index_of_node(Container const& node) const
{
    return get_index_of_node(node.shared_from_this().get());
}

void ParentContainer::constrain()
{
    for (auto& node : container_list)
        node->constrain();
}

size_t ParentContainer::get_min_width() const
{
    size_t size = 0;
    for (auto const& node : container_list)
        size += node->get_min_width();
    return size;
}

size_t ParentContainer::get_min_height() const
{
    size_t size = 0;
    for (auto const& node : container_list)
        size += node->get_min_height();
    return size;
}

std::weak_ptr<ParentContainer> ParentContainer::get_parent() const
{
    return parent;
}

void ParentContainer::set_parent(std::shared_ptr<ParentContainer> const& in_parent)
{
    parent = in_parent;
}

void ParentContainer::relayout()
{
    auto const placement_area = get_logical_area();
    if (scheme == LayoutScheme::horizontal)
    {
        int total_width = 0;
        for (auto const& node : container_list)
        {
            total_width += node->get_logical_area().size.width.as_int();
        }

        int const diff_width = placement_area.size.width.as_value() - total_width;
        int const diff_per_node = static_cast<int>(floor(diff_width / static_cast<double>(container_list.size())));
        for (auto const& node : container_list)
        {
            auto rectangle = node->get_logical_area();
            rectangle.size.width = geom::Width { rectangle.size.width.as_int() + diff_per_node };
            rectangle.size.height = geom::Height { placement_area.size.height };
            node->set_logical_area(rectangle, true);
        }
    }
    else if (scheme == LayoutScheme::vertical)
    {
        int total_height = 0;
        for (auto const& node : container_list)
        {
            total_height += node->get_logical_area().size.height.as_int();
        }

        int const diff_height = placement_area.size.height.as_value() - total_height;
        int const diff_per_node = static_cast<int>(floor(diff_height / static_cast<double>(container_list.size())));
        for (auto const& node : container_list)
        {
            auto rectangle = node->get_logical_area();
            rectangle.size.width = geom::Width { placement_area.size.width };
            rectangle.size.height = geom::Height { rectangle.size.height.as_int() + diff_per_node };
            node->set_logical_area(rectangle, true);
        }
    }
    else if (scheme == LayoutScheme::tabbing || scheme == LayoutScheme::stacking)
    {
        for (auto const& node : container_list)
            node->set_logical_area(placement_area, true);
    }
    else
    {
        mir::log_error("Invalid scheme during relayout");
    }

    // Note that it is important to use the logical_area here instead of the placement area
    set_logical_area(logical_area);
}

void ParentContainer::handle_ready()
{
}

void ParentContainer::handle_modify(miral::WindowSpecification const& specification)
{
}

void ParentContainer::handle_request_move(MirInputEvent const* input_event)
{
}

void ParentContainer::handle_raise()
{
    for (auto const& node : container_list)
        node->handle_raise();
}

bool ParentContainer::resize(Direction direction, int pixels)
{
    return false;
}

bool ParentContainer::set_size(std::optional<int> const& width, std::optional<int> const& height)
{
    if (is_anchored)
        return false;

    auto area = get_logical_area();
    area.size.width = geom::Width { width.value_or(area.size.width.as_int()) };
    area.size.height = geom::Height { height.value_or(area.size.height.as_int()) };
    set_logical_area(area);
    commit_changes();
    return true;
}

bool ParentContainer::toggle_fullscreen()
{
    return false;
}

void ParentContainer::request_horizontal_layout()
{
    scheme = LayoutScheme::horizontal;
    relayout();
}

void ParentContainer::request_vertical_layout()
{
    scheme = LayoutScheme::vertical;
    relayout();
}

void ParentContainer::toggle_layout(bool cycle_thru_all)
{
    if (cycle_thru_all)
        scheme = get_next_layout(scheme);
    else
    {
        if (scheme == LayoutScheme::vertical)
            scheme = LayoutScheme::horizontal;
        else if (scheme == LayoutScheme::horizontal)
            scheme = LayoutScheme::vertical;
    }

    relayout();
}

void ParentContainer::raise_children()
{
    for (auto const& container : container_list)
    {
        if (auto const window = container->window())
            window_controller->raise(*window);
        else if (container->is_lane())
            as_parent(container)->raise_children();
    }
}

void ParentContainer::on_focus_gained()
{
    if (scheme == LayoutScheme::tabbing || scheme == LayoutScheme::stacking)
    {
        for (auto const& container : container_list)
        {
            if (container != state->focused_container() && container->window())
                window_controller->send_to_back(container->window().value());
        }
    }

    if (auto const sh_parent = parent.lock())
        sh_parent->on_focus_gained();
    else if (!anchored())
        raise_children();
}

void ParentContainer::on_focus_lost()
{
}

void ParentContainer::on_move_to(mir::geometry::Point const& top_left)
{
}

void ParentContainer::on_resize(geom::Size const& size)
{
}

mir::geometry::Rectangle
ParentContainer::confirm_placement(MirWindowState state, mir::geometry::Rectangle const& rectangle)
{
    return rectangle;
}

ContainerType ParentContainer::get_type() const
{
    return ContainerType::parent;
}

void ParentContainer::show()
{
    for (auto const& c : container_list)
        c->show();

    is_shown = true;
}

void ParentContainer::hide()
{
    for (auto const& c : container_list)
        c->hide();

    is_shown = false;
}

void ParentContainer::on_open()
{
}

std::shared_ptr<WorkspaceInterface> ParentContainer::get_workspace() const
{
    return workspace.lock();
}

void ParentContainer::set_workspace(std::shared_ptr<WorkspaceInterface> const& next)
{
    workspace = next;
    for (auto const& node : container_list)
        node->set_workspace(next);
}

void ParentContainer::set_workspace_transform(glm::mat4 const& t)
{
    for (auto const& node : container_list)
        node->set_workspace_transform(t);
}

void ParentContainer::set_workspace_alpha(float a)
{
    for (auto const& node : container_list)
        node->set_workspace_alpha(a);
}

std::shared_ptr<OutputInterface> ParentContainer::get_output() const
{
    return get_workspace()->get_output();
}

glm::mat4 ParentContainer::get_transform() const
{
    return glm::mat4(1.f);
}

void ParentContainer::set_transform(glm::mat4 transform)
{
}

glm::mat4 ParentContainer::get_workspace_transform() const
{
    return glm::mat4(1.f);
}

glm::mat4 ParentContainer::get_output_transform() const
{
    return glm::mat4(1.f);
}

void ParentContainer::set_alpha(float const alpha)
{
}

uint32_t ParentContainer::animation_handle() const
{
    return 0;
}

void ParentContainer::animation_handle(uint32_t uint_32)
{
}

bool ParentContainer::is_focused() const
{
    return false;
}

std::optional<miral::Window> ParentContainer::window() const
{
    return std::nullopt;
}

bool ParentContainer::select_next(miracle::Direction)
{
    return false;
}

bool ParentContainer::pinned(bool value)
{
    if (auto sh_parent = parent.lock())
        return sh_parent->pinned(value);

    if (is_anchored)
        return false;

    pinned_ = value;
    return true;
}

bool ParentContainer::pinned() const
{
    if (auto sh_parent = parent.lock())
        return sh_parent->pinned();
    return pinned_;
}

bool ParentContainer::move(Direction direction)
{
    return false;
}

bool ParentContainer::move_by(float dx, float dy)
{
    if (auto const sh_parent = parent.lock())
        return sh_parent->move_by(dx, dy);

    if (is_anchored)
        return false;

    auto area = logical_area;
    area.top_left.x = geom::X { static_cast<float>(area.top_left.x.as_int()) + dx };
    area.top_left.y = geom::Y { static_cast<float>(area.top_left.y.as_int()) + dy };
    set_logical_area(area, false);
    commit_changes();
    return true;
}

bool ParentContainer::move_by(Direction direction, int pixels)
{
    return false;
}

bool ParentContainer::move_to(int x, int y, bool with_animations)
{
    if (auto const sh_parent = parent.lock())
        return sh_parent->move_to(x, y, with_animations);

    if (is_anchored)
        return false;

    auto area = logical_area;
    area.top_left.x = geom::X { x };
    area.top_left.y = geom::Y { y };
    set_logical_area(area, with_animations);
    commit_changes();
    return true;
}

bool ParentContainer::move_to(Container& other)
{
    return false;
}

bool ParentContainer::is_fullscreen() const
{
    return false;
}

bool ParentContainer::toggle_tabbing()
{
    if (scheme == LayoutScheme::tabbing)
        scheme = LayoutScheme::horizontal;
    else
        scheme = LayoutScheme::tabbing;

    relayout();
    return true;
}

bool ParentContainer::toggle_stacking()
{
    if (scheme == LayoutScheme::stacking)
        scheme = LayoutScheme::horizontal;
    else
        scheme = LayoutScheme::stacking;

    relayout();
    return true;
}

bool ParentContainer::set_layout(LayoutScheme new_scheme)
{
    scheme = new_scheme;
    relayout();
    constrain();
    commit_changes();
    return true;
}

LayoutScheme ParentContainer::get_layout() const
{
    return scheme;
}

bool ParentContainer::matches(ContainerScope const&) const
{
    return false;
}

bool ParentContainer::set_anchored(bool anchor)
{
    is_anchored = anchor;
    update_background_client_area();
    return true;
}

bool ParentContainer::anchored() const
{
    if (auto sh_parent = parent.lock())
        return sh_parent->anchored();

    return is_anchored;
}

ScratchpadState ParentContainer::scratchpad_state() const
{
    if (auto sh_parent = parent.lock())
        return sh_parent->scratchpad_state();

    return scratchpad_state_;
}

void ParentContainer::scratchpad_state(ScratchpadState next_scratchpad_state)
{
    if (auto sh_parent = parent.lock())
        return sh_parent->scratchpad_state(next_scratchpad_state);

    scratchpad_state_ = next_scratchpad_state;
}

nlohmann::json ParentContainer::to_json(bool is_workspace_visible) const
{
    auto const visible_area = get_visible_area();
    auto const logical_area = get_logical_area();
    nlohmann::json containers_json;
    for (auto const& container : container_list)
        containers_json.push_back(container->to_json(is_workspace_visible));

    auto locked_parent = parent.lock();
    bool visible = true;

    if (!is_workspace_visible)
        visible = false;

    if (locked_parent == nullptr)
        visible = false;

    auto const id = reinterpret_cast<std::uintptr_t>(this);
    return {
        { "id",                   id                                                                                                                                                                                                                                                 },
        { "name",                 to_string(scheme)                                                                                                                                                                                                                                  },
        { "rect",                 {
                      { "x", logical_area.top_left.x.as_int() },
                      { "y", logical_area.top_left.y.as_int() },
                      { "width", logical_area.size.width.as_int() },
                      { "height", logical_area.size.height.as_int() },
                  }                                                                                                                                                                                                                                                 },
        { "focused",              visible && is_focused()                                                                                                                                                                                                                            },
        { "focus",                std::vector<int>()                                                                                                                                                                                                                                 },
        { "border",               "none"                                                                                                                                                                                                                                             },
        { "current_border_width", 0                                                                                                                                                                                                                                                  },
        { "layout",               to_string(scheme)                                                                                                                                                                                                                                  },
        { "orientation",          "none"                                                                                                                                                                                                                                             },
        { "percent",              get_percent_of_parent()                                                                                                                                                                                                                            },
        { "window_rect",          {
                                                                                                                                                                                                                                                               { "x", visible_area.top_left.x.as_int() - logical_area.top_left.x.as_int() },
                                                                                                                                                                                                                                                               { "y", visible_area.top_left.y.as_int() - logical_area.top_left.y.as_int() },
                                                                                                                                                                                                                                                               { "width", visible_area.size.width.as_int() },
                                                                                                                                                                                                                                                               { "height", visible_area.size.height.as_int() },
                                                                                                                                                                                                                                                           } },
        { "deco_rect",            {
                           { "x", 0 },
                           { "y", 0 },
                           { "width", logical_area.size.width.as_int() },
                           { "height", logical_area.size.height.as_int() },
                       }                                                                                                                                                                                                                                       },
        { "geometry",             {
                          { "x", 0 },
                          { "y", 0 },
                          { "width", logical_area.size.width.as_int() },
                          { "height", logical_area.size.height.as_int() },
                      }                                                                                                                                                                                                                                         },
        { "window",               nullptr                                                                                                                                                                                                                                            }, // TODO
        { "urgent",               false                                                                                                                                                                                                                                              },
        { "floating_nodes",       std::vector<int>()                                                                                                                                                                                                                                 },
        { "sticky",               false                                                                                                                                                                                                                                              },
        { "type",                 "con"                                                                                                                                                                                                                                              },
        { "fullscreen_mode",      is_fullscreen() ? 1 : 0                                                                                                                                                                                                                            }, // TODO: Support value 2
        { "visible",              visible                                                                                                                                                                                                                                            },
        { "shell",                "miracle-wm"                                                                                                                                                                                                                                       }, // TODO
        { "inhibit_idle",         false                                                                                                                                                                                                                                              },
        { "idle_inhibitors",      {}                                                                                                                                                                                                                                                 },
        { "window_properties",    nlohmann::json::object()                                                                                                                                                                                                                           }, // TODO
        { "nodes",                containers_json                                                                                                                                                                                                                                    }
    };
}

void ParentContainer::swap(
    std::shared_ptr<ParentContainer> const& first_parent,
    size_t first_index,
    std::shared_ptr<ParentContainer> const& second_parent,
    size_t second_index)
{
    // First, swap the two containers at a positional and tree level
    auto const first_container = first_parent->container_list[first_index];
    auto const second_container = second_parent->container_list[second_index];
    if (first_parent == second_parent)
    {
        first_parent->swap_within_container(first_container, second_container);
        first_parent->commit_changes();
        return;
    }

    auto const first_logical_area = first_container->get_logical_area();

    // TODO: We can probably split some of this out and make it more accessible
    first_parent->container_list[first_index] = second_container;
    second_parent->container_list[second_index] = first_container;
    first_container->set_parent(second_parent);
    first_container->set_workspace(second_parent->get_workspace());
    second_container->set_parent(first_parent);
    second_container->set_workspace(first_parent->get_workspace());
    first_container->set_logical_area(second_container->get_logical_area(), true);
    second_container->set_logical_area(first_logical_area, true);
    first_parent->commit_changes();
    second_parent->commit_changes();

    // Next, apply the proper visibility to each window
    if (first_parent->is_shown)
        first_parent->show();
    else
        first_parent->hide();

    if (second_parent->is_shown)
        second_parent->show();
    else
        second_parent->hide();
}
