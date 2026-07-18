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
#include "abstract_output.h"
#include "abstract_workspace.h"
#include "compositor_state.h"
#include "config.h"
#include "container.h"
#include "feature_flags.h"
#include "leaf_container.h"
#include "shell_application_manager.h"
#include "tiling_algorithms.h"
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
    std::shared_ptr<AbstractWorkspace> const& workspace,
    std::shared_ptr<ParentContainer> const& parent) :
    CollectionContainer(state->next_container_id()),
    shell_application_manager {
        shell_application_manager
    },
    state { state },
    window_controller { window_controller },
    config { config },
    parent_ { parent },
    logical_area_ { std::move(area) },
    workspace_ { workspace },
    scheme_ { config->get_default_layout_scheme() }
{
    // Parents typically hold around 2 to 8 sub containers inside
    // of them. Reserving at least 4 spots is a relatively safe
    // optimization.
    container_list_.reserve(4);
    update_background_client_area();
}

ParentContainer::~ParentContainer()
{
    try_remove_background_client();
}

void ParentContainer::try_remove_background_client()
{
    if (shell_application_id_)
    {
        shell_application_manager->stop(shell_application_id_.value());
        shell_application_id_.reset();
    }
    shell_application_positioner_.reset();
}

void ParentContainer::update_background_client_area()
{
    try_remove_background_client();
    if (parent_.expired() && feature::parent_container_wallpapers)
    {
        // Start up the internal client that will display the background for this floating parent
        mir::log_info("Spawning ParentBackgroundInternalClient for unanchored root parent");
        auto const positioner = std::make_shared<ParentContainerBackgroundPositioner>(this);
        shell_application_id_ = shell_application_manager->spawn(ShellApplicationRole::parent_container_background, positioner);
        shell_application_positioner_ = positioner;
    }
}

geom::Rectangle ParentContainer::get_logical_area() const
{
    // Unanchored parents should not employ outer gaps in their layout.
    if (parent_.lock() == nullptr)
    {
        auto outer_gaps = config->get_outer_gaps();
        if (auto sh_workspace = workspace_.lock())
        {
            if (auto const workspace_outer_gaps = sh_workspace->outer_gaps())
                outer_gaps = *workspace_outer_gaps;
        }

        return geom::Rectangle(
            geom::Point(
                logical_area_.top_left.x.as_int() + static_cast<int>(outer_gaps.left),
                logical_area_.top_left.y.as_int() + static_cast<int>(outer_gaps.top)),
            geom::Size(
                logical_area_.size.width.as_int() - static_cast<int>(outer_gaps.left + outer_gaps.right),
                logical_area_.size.height.as_int() - static_cast<int>(outer_gaps.top + outer_gaps.bottom)));
    }

    return logical_area_;
}

geom::Rectangle ParentContainer::create_space(std::optional<size_t> index)
{
    auto const placement_area = get_logical_area();
    geom::Rectangle pending_logical_rect;
    auto const pending_index = index.value_or(container_list_.size());
    if (scheme_ == LayoutScheme::horizontal)
        pending_logical_rect = insert_node<false>(container_list_,
            placement_area,
            pending_index);
    else if (scheme_ == LayoutScheme::vertical)
        pending_logical_rect = insert_node<true>(container_list_,
            placement_area,
            pending_index);
    else if (scheme_ == LayoutScheme::tabbing || scheme_ == LayoutScheme::stacking)
        pending_logical_rect = placement_area;
    else
        mir::log_error("Invalid scheme during create_space");

    return pending_logical_rect;
}

miral::WindowSpecification ParentContainer::place_new_window(
    miral::WindowSpecification const& requested_specification,
    std::optional<size_t> index)
{
    if (!index)
    {
        for (size_t i = 0; i < container_list_.size(); i++)
        {
            if (container_list_[i] == state->focused_container())
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

    if (new_spec.state())
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
        new_spec.state() && new_spec.state() == mir_window_state_fullscreen);

    return new_spec;
}

std::shared_ptr<LeafContainer> ParentContainer::create_space_for_window(std::optional<size_t> pending_index)
{
    auto const index = pending_index.value_or(container_list_.size());
    pending_node_ = std::make_shared<LeafContainer>(
        workspace_.lock(),
        window_controller,
        create_space(index),
        config,
        as_parent(shared_from_this()),
        state);
    container_list_.insert(container_list_.begin() + static_cast<std::vector<std::shared_ptr<Container>>::difference_type>(index), pending_node_);
    return pending_node_;
}

std::shared_ptr<Container> ParentContainer::confirm_window(miral::Window const& window)
{
    bool needs_relayout = false;
    if (pending_node_ == nullptr)
        pending_node_ = create_space_for_window(std::nullopt);

    mir::log_debug("Parent on workspace %s receiving new window", !workspace_.expired() ? workspace_.lock()->display_name().c_str() : "nullptr");
    auto retval = pending_node_;
    pending_node_ = nullptr;
    retval->associate_to_window(window);
    retval->set_parent(as_parent(shared_from_this()));
    commit_changes();
    return retval;
}

void ParentContainer::add_child(std::shared_ptr<Container> const& node, size_t index)
{
    auto const rectangle = create_space(index);
    node->set_parent(as_parent(shared_from_this()));
    node->set_workspace(workspace_.lock());
    node->set_logical_area(rectangle, true);
    container_list_.insert(container_list_.begin() + index, node);
    relayout();
    constrain();
}

std::shared_ptr<ParentContainer> ParentContainer::convert_to_parent(std::shared_ptr<Container> const& container)
{
    auto const index = get_index_of_node(container);
    if (!index.has_value())
    {
        mir::log_error("Attempting to convert a node to lane with an incorrect parent");
        return nullptr;
    }

    auto new_parent_node = std::make_shared<ParentContainer>(
        shell_application_manager,
        state,
        window_controller,
        config,
        container->get_logical_area(),
        workspace_.lock(),
        Container::as_parent(shared_from_this()));
    new_parent_node->container_list_.push_back(container);
    container->set_parent(new_parent_node);
    container_list_[index.value()] = new_parent_node;
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
    logical_area_ = target_rect;
    auto target_placement_area = get_logical_area();

    if (auto const background_positioner_sh = shell_application_positioner_.lock())
    {
        background_positioner_sh->set_area(target_placement_area);
    }

    std::vector<geom::Rectangle> pending_size_updates;
    pending_size_updates.reserve(container_list_.size());
    if (scheme_ == LayoutScheme::horizontal)
    {
        int total_width = 0;
        for (size_t idx = 0; idx < container_list_.size(); idx++)
        {
            auto item = container_list_[idx];
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
    else if (scheme_ == LayoutScheme::vertical)
    {
        int total_height = 0;
        for (size_t idx = 0; idx < container_list_.size(); idx++)
        {
            auto item = container_list_[idx];
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
    else if (scheme_ == LayoutScheme::tabbing || scheme_ == LayoutScheme::stacking)
    {
        for (size_t idx = 0; idx < container_list_.size(); idx++)
        {
            pending_size_updates.push_back(target_placement_area);
        }
    }
    else
    {
        mir::log_error("Cannot set_logical_area with invalid scheme");
    }

    for (size_t i = 0; i < container_list_.size(); i++)
    {
        container_list_[i]->set_logical_area(pending_size_updates[i], with_animations);
    }
}

void ParentContainer::commit_changes()
{
    auto const nodes = container_list_; // snapshot
    for (auto& node : nodes)
        node->commit_changes();
}

std::shared_ptr<LeafContainer> ParentContainer::get_nth_window(size_t i) const
{
    if (i >= container_list_.size())
        return nullptr;

    if (auto const leaf = as_leaf(container_list_[i]))
        return leaf;

    // The lane is correct, so let's get the first window in that lane.
    return as_parent(container_list_[i])->get_nth_window(0);
}

std::shared_ptr<Container> ParentContainer::find_where(std::function<bool(std::shared_ptr<Container> const&)> func) const
{
    auto const nodes = container_list_; // snapshot
    for (auto node : nodes)
        if (func(node))
            return node;

    for (auto const& node : nodes)
    {
        if (auto const parent_node = as_parent(node))
        {
            if (auto retval = parent_node->find_where(func))
                return retval;
        }
    }

    return nullptr;
}

std::vector<std::shared_ptr<Container>> ParentContainer::children() const
{
    return container_list_;
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

    container_list_[second_index] = first;
    container_list_[first_index] = second;
    relayout();
    constrain();
}

void ParentContainer::remove_child(const std::shared_ptr<Container>& container)
{
    {
        std::erase_if(container_list_, [&](std::shared_ptr<Container> const& content)
        {
            return content == container;
        });

        // If we have one child AND it is a lane, THEN we can absorb all of it's children
        if (container_list_.size() == 1)
        {
            auto const dying_lane = as_parent(container_list_[0]);
            if (dying_lane)
            {
                container_list_.clear();
                for (auto const& sub_node : dying_lane->children())
                {
                    container_list_.push_back(sub_node);
                    sub_node->set_parent(as_parent(shared_from_this()));
                }
                scheme_ = dying_lane->get_scheme();
            }
        }
    }

    relayout();
}

std::optional<size_t> ParentContainer::get_index_of_node(Container const* node) const
{
    for (size_t i = 0; i < container_list_.size(); i++)
        if (container_list_[i].get() == node)
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
    auto const nodes = container_list_; // snapshot
    for (auto& node : nodes)
        node->constrain();
}

size_t ParentContainer::get_min_width() const
{
    auto const nodes = container_list_;
    size_t size = 0;
    for (auto const& node : nodes)
        size += node->get_min_width();
    return size;
}

size_t ParentContainer::get_min_height() const
{
    auto const nodes = container_list_;
    size_t size = 0;
    for (auto const& node : nodes)
        size += node->get_min_height();
    return size;
}

std::weak_ptr<ParentContainer> ParentContainer::get_parent() const
{
    return parent_;
}

void ParentContainer::set_parent(std::shared_ptr<ParentContainer> const& in_parent)
{
    parent_ = in_parent;
}

void ParentContainer::relayout()
{
    auto const placement_area = get_logical_area();
    if (scheme_ == LayoutScheme::horizontal)
    {
        int total_width = 0;
        for (auto const& node : container_list_)
        {
            total_width += node->get_logical_area().size.width.as_int();
        }

        int const diff_width = placement_area.size.width.as_value() - total_width;
        int const diff_per_node = static_cast<int>(floor(diff_width / static_cast<double>(container_list_.size())));
        for (auto const& node : container_list_)
        {
            auto rectangle = node->get_logical_area();
            rectangle.size.width = geom::Width { rectangle.size.width.as_int() + diff_per_node };
            rectangle.size.height = geom::Height { placement_area.size.height };
            node->set_logical_area(rectangle, true);
        }
    }
    else if (scheme_ == LayoutScheme::vertical)
    {
        int total_height = 0;
        for (auto const& node : container_list_)
        {
            total_height += node->get_logical_area().size.height.as_int();
        }

        int const diff_height = placement_area.size.height.as_value() - total_height;
        int const diff_per_node = static_cast<int>(floor(diff_height / static_cast<double>(container_list_.size())));
        for (auto const& node : container_list_)
        {
            auto rectangle = node->get_logical_area();
            rectangle.size.width = geom::Width { placement_area.size.width };
            rectangle.size.height = geom::Height { rectangle.size.height.as_int() + diff_per_node };
            node->set_logical_area(rectangle, true);
        }
    }
    else if (scheme_ == LayoutScheme::tabbing || scheme_ == LayoutScheme::stacking)
    {
        for (auto const& node : container_list_)
            node->set_logical_area(placement_area, true);
    }
    else
    {
        mir::log_error("Invalid scheme during relayout");
    }

    // Note that it is important to use the logical_area here instead of the placement area
    auto const logical_area = logical_area_;
    set_logical_area(logical_area, true);
}

void ParentContainer::handle_raise()
{
    auto const nodes = container_list_;
    for (auto const& node : nodes)
        node->handle_raise();
}

bool ParentContainer::set_size(std::optional<int> const& width, std::optional<int> const& height)
{
    auto area = get_logical_area();
    area.size.width = geom::Width { width.value_or(area.size.width.as_int()) };
    area.size.height = geom::Height { height.value_or(area.size.height.as_int()) };
    set_logical_area(area, true);
    commit_changes();
    return true;
}

void ParentContainer::request_horizontal_layout()
{
    scheme_ = LayoutScheme::horizontal;
    relayout();
}

void ParentContainer::request_vertical_layout()
{
    scheme_ = LayoutScheme::vertical;
    relayout();
}

void ParentContainer::toggle_layout(bool cycle_thru_all)
{
    {
        if (cycle_thru_all)
            scheme_ = get_next_layout(scheme_);
        else
        {
            if (scheme_ == LayoutScheme::vertical)
                scheme_ = LayoutScheme::horizontal;
            else if (scheme_ == LayoutScheme::horizontal)
                scheme_ = LayoutScheme::vertical;
        }
    }
    relayout();
}

void ParentContainer::raise_children()
{
    auto const nodes = container_list_;
    for (auto const& container : nodes)
    {
        if (auto const window = container->window())
            window_controller->raise(*window);
        else if (auto const parent_node = as_parent(container))
            parent_node->raise_children();
    }
}

void ParentContainer::on_focus_gained()
{
    if (scheme_ == LayoutScheme::tabbing || scheme_ == LayoutScheme::stacking)
    {
        for (auto const& container : container_list_)
        {
            if (container != state->focused_container() && container->window())
                window_controller->send_to_back(container->window().value());
        }
    }

    auto const sh_parent = parent_.lock();

    if (sh_parent)
        sh_parent->on_focus_gained();
    else
        raise_children();
}

void ParentContainer::show()
{
    auto const nodes = container_list_;
    for (auto const& c : nodes)
        c->show();
    is_shown_ = true;
}

void ParentContainer::hide()
{
    auto const nodes = container_list_;
    for (auto const& c : nodes)
        c->hide();
    is_shown_ = false;
}

std::shared_ptr<AbstractWorkspace> ParentContainer::get_workspace() const
{
    return workspace_.lock();
}

void ParentContainer::set_workspace(std::shared_ptr<AbstractWorkspace> const& next)
{
    workspace_ = next;
    auto const nodes = container_list_;
    for (auto const& node : nodes)
        node->set_workspace(next);
}

void ParentContainer::set_workspace_transform(glm::mat4 const& t)
{
    auto const nodes = container_list_;
    for (auto const& node : nodes)
        node->set_workspace_transform(t);
}

void ParentContainer::set_workspace_alpha(float a)
{
    auto const nodes = container_list_;
    for (auto const& node : nodes)
        node->set_workspace_alpha(a);
}

std::shared_ptr<AbstractOutput> ParentContainer::get_output() const
{
    return get_workspace()->get_output();
}

glm::mat4 ParentContainer::get_animation_transform() const
{
    return glm::mat4(1.f);
}

void ParentContainer::set_animation_transform(glm::mat4)
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

void ParentContainer::set_animation_alpha(float const)
{
}

uint32_t ParentContainer::animation_handle() const
{
    return 0;
}

void ParentContainer::animation_handle(uint32_t)
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

bool ParentContainer::move_by(float dx, float dy)
{
    if (auto const sh_parent = parent_.lock())
        return sh_parent->move_by(dx, dy);

    auto area = logical_area_;
    area.top_left.x = geom::X { static_cast<float>(area.top_left.x.as_int()) + dx };
    area.top_left.y = geom::Y { static_cast<float>(area.top_left.y.as_int()) + dy };
    set_logical_area(area, false);
    commit_changes();
    return true;
}

bool ParentContainer::move_to(int x, int y, bool with_animations)
{
    constexpr bool can_move_parent_container = false;
    if (!can_move_parent_container)
        return false;

    if (auto const sh_parent = parent_.lock())
        return sh_parent->move_to(x, y, with_animations);

    auto area = logical_area_;
    area.top_left.x = geom::X { x };
    area.top_left.y = geom::Y { y };
    set_logical_area(area, with_animations);
    commit_changes();
    return true;
}

bool ParentContainer::toggle_tabbing()
{
    {
        if (scheme_ == LayoutScheme::tabbing)
            scheme_ = LayoutScheme::horizontal;
        else
            scheme_ = LayoutScheme::tabbing;
    }
    relayout();
    return true;
}

bool ParentContainer::toggle_stacking()
{
    {
        if (scheme_ == LayoutScheme::stacking)
            scheme_ = LayoutScheme::horizontal;
        else
            scheme_ = LayoutScheme::stacking;
    }
    relayout();
    return true;
}

bool ParentContainer::set_layout(LayoutScheme new_scheme)
{
    scheme_ = new_scheme;
    relayout();
    constrain();
    commit_changes();
    return true;
}

LayoutScheme ParentContainer::get_layout() const
{
    return scheme_;
}

ScratchpadState ParentContainer::scratchpad_state() const
{
    if (auto sh_parent = parent_.lock())
        return sh_parent->scratchpad_state();
    return scratchpad_state_;
}

void ParentContainer::scratchpad_state(ScratchpadState next_scratchpad_state)
{
    if (auto sh_parent = parent_.lock())
        return sh_parent->scratchpad_state(next_scratchpad_state);
    scratchpad_state_ = next_scratchpad_state;
}

nlohmann::json ParentContainer::to_json(bool is_workspace_visible) const
{
    auto const logical_area = get_logical_area();
    auto const container_list = container_list_;
    nlohmann::json containers_json;
    for (auto const& container : container_list)
        containers_json.push_back(container->to_json(is_workspace_visible));

    auto locked_parent = parent_.lock();
    auto const scheme = scheme_;

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
                                                                                                                                                                                                                                                               { "x", 0 },
                                                                                                                                                                                                                                                               { "y", 0 },
                                                                                                                                                                                                                                                               { "width", logical_area.size.width.as_int() },
                                                                                                                                                                                                                                                               { "height", logical_area.size.height.as_int() },
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
        { "fullscreen_mode",      0                                                                                                                                                                                                                                                  }, // TODO: Support value 2
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
    auto const first_container = first_parent->container_list_[first_index];
    auto const second_container = second_parent->container_list_[second_index];
    if (first_parent == second_parent)
    {
        first_parent->swap_within_container(first_container, second_container);
        first_parent->commit_changes();
        return;
    }

    auto const first_logical_area = first_container->get_logical_area();

    // TODO: We can probably split some of this out and make it more accessible
    first_parent->container_list_[first_index] = second_container;
    second_parent->container_list_[second_index] = first_container;
    first_container->set_parent(second_parent);
    first_container->set_workspace(second_parent->get_workspace());
    second_container->set_parent(first_parent);
    second_container->set_workspace(first_parent->get_workspace());
    first_container->set_logical_area(second_container->get_logical_area(), true);
    second_container->set_logical_area(first_logical_area, true);
    first_parent->commit_changes();
    second_parent->commit_changes();

    // Next, apply the proper visibility to each window
    if (first_parent->is_shown_)
        first_parent->show();
    else
        first_parent->hide();

    if (second_parent->is_shown_)
        second_parent->show();
    else
        second_parent->hide();
}
