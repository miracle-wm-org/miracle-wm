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

#define MIR_LOG_COMPONENT "command_controller"

#include "command_controller.h"
#include "config.h"
#include "container_listener.h"
#include "leaf_container.h"
#include "mode_observer.h"
#include "output_manager.h"
#include "parent_container.h"
#include "scratchpad.h"
#include "workspace_manager.h"

#include <mir/log.h>
#include <miral/runner.h>

using namespace miracle;

CommandController::CommandController(
    std::shared_ptr<Config> const& config,
    std::recursive_mutex& mutex,
    std::shared_ptr<CompositorState> const& state,
    std::shared_ptr<WindowController> const& window_controller,
    std::shared_ptr<WorkspaceManager> const& workspace_manager,
    std::shared_ptr<ModeObserverRegistrar> const& mode_observer_registrar,
    std::unique_ptr<CommandControllerInterface> interface,
    std::shared_ptr<Scratchpad> const& scratchpad_,
    std::shared_ptr<OutputManager> const& output_manager) :
    config { config },
    mutex { mutex },
    state { state },
    window_controller { window_controller },
    workspace_manager { workspace_manager },
    mode_observer_registrar { mode_observer_registrar },
    interface { std::move(interface) },
    scratchpad_ { scratchpad_ },
    output_manager { output_manager }
{
}

void CommandController::try_toggle_resize_mode()
{
    std::lock_guard lock(mutex);
    if (!state->focused_container())
    {
        set_mode(WindowManagerMode::normal);
        return;
    }

    if (state->focused_container()->get_type() != ContainerType::leaf)
    {
        set_mode(WindowManagerMode::normal);
        return;
    }

    if (state->mode() != WindowManagerMode::normal)
        set_mode(WindowManagerMode::normal);
    else
        set_mode(WindowManagerMode::resizing);
}

bool CommandController::try_request_vertical(std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    if (state->mode() != WindowManagerMode::normal)
        return false;

    for (auto const& container : resolve_scope(scope))
    {
        container->request_vertical_layout();
    }
    return true;
}

bool CommandController::try_toggle_layout(bool cycle_thru_all, std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    if (state->mode() != WindowManagerMode::normal)
        return false;

    auto containers = resolve_scope(scope);
    if (containers.empty())
        return false;

    for (auto const& container : containers)
    {
        container->toggle_layout(cycle_thru_all);
    }
    return true;
}

bool CommandController::try_cycle_through_request_types(
    std::vector<LayoutRequestType> const& request_types,
    std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    if (state->mode() != WindowManagerMode::normal)
        return false;

    if (request_types.empty())
        return false;

    auto const containers = resolve_scope(scope);
    if (containers.empty())
        return false;

    for (auto const& container : containers)
    {
        auto const current_type = container->get_layout();
        size_t i = -1;
        bool found = false;
        for (; i < request_types.size(); i++)
        {
            switch (request_types[i])
            {
            case LayoutRequestType::split:
                if (current_type == LayoutScheme::horizontal || current_type == LayoutScheme::vertical)
                    found = true;
                break;
            case LayoutRequestType::tabbed:
                if (current_type == LayoutScheme::tabbing)
                    found = true;
                break;
            case LayoutRequestType::stacking:
                if (current_type == LayoutScheme::stacking)
                    found = true;
                break;
            case LayoutRequestType::splith:
                if (current_type == LayoutScheme::horizontal)
                    found = true;
                break;
            case LayoutRequestType::splitv:
                if (current_type == LayoutScheme::vertical)
                    found = true;
                break;
            }

            if (found)
                break;
        }

        i++;
        if (i == request_types.size())
            i = 0;
        switch (request_types[i])
        {
        case LayoutRequestType::split:
            container->toggle_layout(false);
            break;
        case LayoutRequestType::splitv:
            container->set_layout(LayoutScheme::vertical);
            break;
        case LayoutRequestType::splith:
            container->set_layout(LayoutScheme::horizontal);
            break;
        case LayoutRequestType::stacking:
            container->set_layout(LayoutScheme::stacking);
            break;
        case LayoutRequestType::tabbed:
            container->set_layout(LayoutScheme::tabbing);
            break;
        }
    }

    return true;
}

bool CommandController::try_request_horizontal(std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    if (state->mode() != WindowManagerMode::normal)
        return false;

    for (auto const& container : resolve_scope(scope))
    {
        container->request_horizontal_layout();
    }

    return true;
}

bool CommandController::try_resize(Direction direction, int pixels, std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    auto containers = resolve_scope(scope);
    if (containers.empty())
        return false;

    bool result = true;
    for (auto const& container : containers)
    {
        if (!container->resize(direction, pixels))
            result = false;
    }
    return result;
}

bool CommandController::try_resize_ppt(Direction direction, float ppt, std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    auto const containers = resolve_scope(scope);
    if (containers.empty())
        return false;

    bool result = true;
    for (auto const& container : containers)
    {
        auto const output = container->get_output();
        if (!output)
        {
            result = false;
            continue;
        }

        float total_size = 0;
        switch (direction)
        {
        case Direction::down:
        case Direction::up:
            total_size = output->get_area().size.height.as_value();
            break;
        default:
            total_size = output->get_area().size.width.as_value();
            break;
        }

        if (!container->resize(direction, ppt * total_size))
            result = false;
    }
    return result;
}

bool CommandController::try_set_size(
    std::optional<int> const& width,
    bool is_width_ppt,
    std::optional<int> const& height,
    bool is_height_ppt,
    std::vector<ContainerScope> const& scope)
{
    // TODO: Account for ppt here
    std::lock_guard lock(mutex);
    auto const containers = resolve_scope(scope);
    if (containers.empty())
        return false;

    bool result = true;
    for (auto const& container : containers)
    {
        auto const output = container->get_output();
        if (!output)
        {
            result = false;
            continue;
        }

        if (!container->set_size(width, height))
            result = false;
    }
    return result;
}

bool CommandController::try_move_by_direction(Direction direction, std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    if (state->mode() != WindowManagerMode::normal)
        return false;

    auto containers = resolve_scope(scope);
    if (containers.empty())
        return false;

    bool result = true;
    for (auto const& container : containers)
    {
        if (!container->move(direction))
            result = false;
    }
    return result;
}

bool CommandController::try_move_by_pixels(miracle::Direction direction, int pixels, std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    if (state->mode() != WindowManagerMode::normal)
        return false;

    auto containers = resolve_scope(scope);
    if (containers.empty())
        return false;

    bool result = true;
    for (auto const& container : containers)
    {
        if (!container->move_by(direction, pixels))
            result = false;
    }
    return result;
}

bool CommandController::try_move_by_ppt(Direction direction, float ppt, std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    if (state->mode() != WindowManagerMode::normal)
        return false;

    auto const containers = resolve_scope(scope);
    if (containers.empty())
        return false;

    bool result = true;
    for (auto const& container : containers)
    {
        auto const output = container->get_output();
        if (!output)
        {
            mir::log_error("try_move_by_ppt: container does not have an output");
            result = false;
            continue;
        }

        float total_size = 0;
        switch (direction)
        {
        case Direction::up:
        case Direction::down:
            total_size = output->get_area().size.height.as_value();
            break;
        case Direction::left:
        case Direction::right:
        default:
            total_size = output->get_area().size.width.as_value();
            break;
        }

        if (!container->move_by(direction, total_size * ppt))
            result = false;
    }
    return result;
}

bool CommandController::try_move_to(float x, bool is_x_ppt, float y, bool is_y_ppt, std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    if (state->mode() != WindowManagerMode::normal)
        return false;

    auto const containers = resolve_scope(scope);
    if (containers.empty())
        return false;

    bool result = true;
    for (auto const& container : containers)
    {
        auto const output = container->get_output();
        if (!output)
        {
            mir::log_error("try_move_to: container does not have an output");
            result = false;
            continue;
        }

        float resolved_x = x;
        float resolved_y = y;
        if (is_x_ppt)
            resolved_x = output->get_area().size.width.as_value() * x;
        if (is_y_ppt)
            resolved_y = output->get_area().size.height.as_value() * y;

        if (!container->move_to(resolved_x, resolved_y))
            result = false;
    }
    return result;
}

bool CommandController::try_move_to_center_of_active_output(std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    auto const& active_output = output_manager->focused();
    auto const active = state->focused_container().get();
    auto const area = active_output->get_area();
    float const x = static_cast<float>(area.size.width.as_int()) / 2.f - static_cast<float>(active->get_visible_area().size.width.as_int()) / 2.f;
    float const y = static_cast<float>(area.size.height.as_int()) / 2.f - static_cast<float>(active->get_visible_area().size.height.as_int()) / 2.f;
    return try_move_to(static_cast<int>(x), false, static_cast<int>(y), false, scope);
}

bool CommandController::try_move_to_absolute_center(std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    float x = 0, y = 0;
    for (auto const& output : output_manager->outputs())
    {
        auto area = output->get_area();
        float const end_x = static_cast<float>(area.size.width.as_int() + area.top_left.x.as_int());
        float const end_y = static_cast<float>(area.size.height.as_int() + area.top_left.y.as_int());
        if (end_x > x)
            x = end_x;
        if (end_y > y)
            y = end_y;
    }

    auto const active = state->focused_container();
    float const x_pos = x / 2.f - static_cast<float>(active->get_visible_area().size.width.as_int()) / 2.f;
    float const y_pos = y / 2.f - static_cast<float>(active->get_visible_area().size.height.as_int()) / 2.f;
    return try_move_to(static_cast<int>(x_pos), false, static_cast<int>(y_pos), false, scope);
}

bool CommandController::try_move_to_cursor(std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    auto const& position = state->cursor_position;
    return try_move_to(position.x.as_int(), false, position.y.as_int(), false, scope);
}

bool CommandController::try_swap(std::vector<ContainerScope> const& scope, ContainerScope swap_with_scope)
{
    auto const first_containers = resolve_scope(scope);
    if (first_containers.empty())
        return false;

    auto const second_containers = resolve_scope({ swap_with_scope });
    if (second_containers.empty())
        return false;

    auto const first_container = first_containers.front();
    auto const second_container = second_containers.front();
    if (first_container == second_container)
        return false;

    auto const first_parent = first_container->get_parent().lock();
    if (!first_parent)
        return false;

    auto const second_parent = second_container->get_parent().lock();
    if (!second_parent)
        return false;

    auto const first_index = first_parent->get_index_of_node(first_container).value();
    auto const second_index = second_parent->get_index_of_node(second_container).value();
    ParentContainer::swap(first_parent, first_index, second_parent, second_index);
    return true;
}

void CommandController::select_container(std::shared_ptr<Container> const& container)
{
    std::lock_guard lock(mutex);
    if (container->window())
        window_controller->select_active_window(container->window().value());
    else
    {
        window_controller->select_active_window(miral::Window {});
        state->focus_container(container, true);
    }
}

bool CommandController::try_select(std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    if (state->mode() != WindowManagerMode::normal)
        return false;

    auto containers = resolve_scope(scope);
    if (containers.empty())
        return false;

    select_container(containers[0]);
    return true;
}

bool CommandController::try_select(miracle::Direction direction, std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    if (state->mode() != WindowManagerMode::normal)
        return false;

    auto containers = resolve_scope(scope);
    if (containers.empty())
        return false;

    bool result = true;
    for (auto const& container : containers)
    {
        if (!container->select_next(direction))
            result = false;
    }
    return result;
}

bool CommandController::try_select_parent(std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    if (state->mode() != WindowManagerMode::normal)
        return false;

    auto containers = resolve_scope(scope);
    if (containers.empty())
        return false;

    bool result = true;
    for (auto const& container : containers)
    {
        if (!container->get_parent().expired())
        {
            select_container(container->get_parent().lock());
        }
        else
        {
            mir::log_error("try_select_parent: no parent to select");
            result = false;
        }
    }
    return result;
}

bool CommandController::try_select_child(std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    if (state->mode() != WindowManagerMode::normal)
        return false;

    auto containers = resolve_scope(scope);
    if (containers.empty())
        return false;

    bool result = true;
    for (auto const& container : containers)
    {
        if (container->get_type() != ContainerType::parent)
        {
            mir::log_info("CommandController::try_select_child: parent is not selected");
            result = false;
            continue;
        }

        for (auto const& child : state->containers())
        {
            if (!child.expired())
            {
                auto const lock_child = child.lock();
                if (lock_child->get_parent().expired())
                    continue;

                if (lock_child->get_parent().lock() == container)
                    select_container(lock_child);
            }
        }

        if (!container->get_parent().expired())
        {
            state->focus_container(container->get_parent().lock());
        }
        else
        {
            mir::log_error("try_select_parent: no parent to select");
            result = false;
        }
    }
    return result;
}

bool CommandController::try_select_prev(std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    auto const container = state->focused_container();
    if (!container)
        return false;

    if (container->get_type() != ContainerType::leaf)
        return false;

    if (auto const parent = Container::as_parent(container->get_parent().lock()))
    {
        auto const index = parent->get_index_of_node(container).value();
        if (index != 0)
        {
            auto const node_to_select = parent->get_nth_window(index - 1);
            window_controller->select_active_window(node_to_select->window().value());
        }
    }
}

bool CommandController::try_select_next(std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    auto const container = state->focused_container();
    if (!container)
        return false;

    if (container->get_type() != ContainerType::leaf)
        return false;

    if (auto const parent = Container::as_parent(container->get_parent().lock()))
    {
        auto const index = parent->get_index_of_node(container).value();
        if (index != parent->num_nodes() - 1)
        {
            auto node_to_select = parent->get_nth_window(index + 1);
            window_controller->select_active_window(node_to_select->window().value());
        }
    }

    return true;
}

bool CommandController::try_select_floating(std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    if (state->mode() != WindowManagerMode::normal)
        return false;

    auto containers = resolve_scope(scope);
    if (containers.empty())
        return false;

    bool result = true;
    for (auto const& container : containers)
    {
        if (auto to_select = state->first_floating())
        {
            if (auto const& window = to_select->window())
            {
                window_controller->select_active_window(window.value());
            }
            else
            {
                result = false;
            }
        }
        else
        {
            result = false;
        }
    }
    return result;
}

bool CommandController::try_select_tiling(std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    if (state->mode() != WindowManagerMode::normal)
        return false;

    auto containers = resolve_scope(scope);
    if (containers.empty())
        return false;

    bool result = true;
    for (auto const& container : containers)
    {
        if (auto to_select = state->first_tiling())
        {
            if (auto const& window = to_select->window())
            {
                window_controller->select_active_window(window.value());
            }
            else
            {
                result = false;
            }
        }
        else
        {
            result = false;
        }
    }
    return result;
}

bool CommandController::try_select_toggle(std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    if (state->mode() != WindowManagerMode::normal)
        return false;

    auto containers = resolve_scope(scope);
    if (containers.empty())
        return false;

    bool result = true;
    for (auto const& container : containers)
    {
        if (container->anchored())
            result = try_select_floating(scope) && result;
        else
            result = try_select_tiling(scope) && result;
    }
    return result;
}

bool CommandController::try_close_window(std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    auto containers = resolve_scope(scope);
    if (containers.empty())
        return false;

    bool result = true;
    for (auto const& container : containers)
    {
        if (auto window = container->window())
            window_controller->close(window.value());
        else
            result = false;
    }
    return result;
}

bool CommandController::quit()
{
    interface->quit();
    return true;
}

bool CommandController::try_toggle_fullscreen(std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    if (state->mode() != WindowManagerMode::normal)
        return false;

    auto containers = resolve_scope(scope);
    if (containers.empty())
        return false;

    bool result = true;
    for (auto const& container : containers)
    {
        if (!container->toggle_fullscreen())
            result = false;
    }
    return result;
}

bool CommandController::select_workspace(int number, bool back_and_forth)
{
    std::lock_guard lock(mutex);
    if (state->mode() != WindowManagerMode::normal)
        return false;

    if (!output_manager->focused())
    {
        mir::log_warning("select_workspace %d: no focused output", number);
        return false;
    }

    mir::log_info("select_workspace: %d", number);
    workspace_manager->request_workspace(output_manager->focused(), number, back_and_forth);
    return true;
}

bool CommandController::select_workspace(std::string const& name, bool back_and_forth)
{
    std::lock_guard lock(mutex);
    if (state->mode() != WindowManagerMode::normal)
        return false;

    return workspace_manager->request_workspace(output_manager->focused(), name, back_and_forth);
}

bool CommandController::select_workspace_with_scope(std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    if (state->mode() != WindowManagerMode::normal)
        return false;

    auto containers = resolve_scope(scope);
    if (containers.empty())
        return false;

    if (!containers[0]->get_workspace())
        return false;

    return workspace_manager->request_focus(containers[0]->get_workspace()->id());
}

bool CommandController::next_workspace()
{
    std::lock_guard lock(mutex);
    if (state->mode() != WindowManagerMode::normal)
        return false;

    workspace_manager->request_next(output_manager->focused());
    return true;
}

bool CommandController::prev_workspace()
{
    std::lock_guard lock(mutex);
    if (state->mode() != WindowManagerMode::normal)
        return false;

    workspace_manager->request_prev(output_manager->focused());
    return true;
}

bool CommandController::back_and_forth_workspace()
{
    std::lock_guard lock(mutex);
    if (state->mode() != WindowManagerMode::normal)
        return false;

    workspace_manager->request_back_and_forth();
    return true;
}

bool CommandController::next_workspace_on_output()
{
    std::lock_guard lock(mutex);
    if (state->mode() != WindowManagerMode::normal)
        return false;

    if (auto const focused = output_manager->focused())
        return workspace_manager->request_next_on_output(*focused);

    return false;
}

bool CommandController::prev_workspace_on_output()
{
    std::lock_guard lock(mutex);
    if (state->mode() != WindowManagerMode::normal)
        return false;

    if (auto const focused = output_manager->focused())
        return workspace_manager->request_prev_on_output(*focused);

    return false;
}

bool CommandController::try_move_to_workspace(std::vector<ContainerScope> const& scope, int number, bool back_and_forth)
{
    std::lock_guard lock(mutex);
    if (!can_move_container())
        return false;

    // Note: it is important that we resolve the scope before we select
    // the workspace, as selecting a workspace may cause the selected
    // container to change.
    auto const containers = resolve_scope(scope);
    if (containers.empty())
        return false;

    if (!select_workspace(number, back_and_forth))
        return false;

    auto const o = output_manager->focused();

    for (auto const& container : containers)
    {
        if (container->get_workspace()->num() == number)
            continue;

        container->get_output()->delete_container(container);
        o->graft(container);
        if (container->window().value())
            window_controller->select_active_window(container->window().value());
    }

    return true;
}

bool CommandController::try_move_to_workspace_named(std::vector<ContainerScope> const& scope, std::string const& name, bool back_and_forth)
{
    std::lock_guard lock(mutex);
    if (!can_move_container())
        return false;

    auto const containers = resolve_scope(scope);
    if (containers.empty())
        return false;

    for (auto const& container : containers)
    {
        if (container->get_workspace()->name() == name)
            return false;

        container->get_output()->delete_container(container);
        state->unfocus_container(container);

        if (workspace_manager->request_workspace(output_manager->focused(), name, back_and_forth))
            output_manager->focused()->graft(container);
    }

    return true;
}

bool CommandController::try_move_to_current_workspace(std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    if (!can_move_container())
        return false;

    auto const containers = resolve_scope(scope);
    if (containers.empty())
        return false;

    if (!output_manager->focused())
        return false;

    for (auto const& container : containers)
    {
        if (container->get_workspace() == output_manager->focused()->active().get())
            continue;

        container->get_output()->delete_container(container);
        state->unfocus_container(container);

        if (workspace_manager->request_next(output_manager->focused()))
            output_manager->focused()->graft(container);
    }

    return true;
}

bool CommandController::try_move_to_next_workspace(std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    if (!can_move_container())
        return false;

    auto const containers = resolve_scope(scope);
    if (containers.empty())
        return false;

    for (auto const& container : containers)
    {
        container->get_output()->delete_container(container);
        state->unfocus_container(container);

        if (workspace_manager->request_next(output_manager->focused()))
            output_manager->focused()->graft(container);
    }

    return true;
}

bool CommandController::try_move_to_prev_workspace(std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    if (!can_move_container())
        return false;

    auto const containers = resolve_scope(scope);
    if (containers.empty())
        return false;

    for (auto const& container : containers)
    {
        container->get_output()->delete_container(container);
        state->unfocus_container(container);

        if (workspace_manager->request_prev(output_manager->focused()))
            output_manager->focused()->graft(container);
    }

    return true;
}

bool CommandController::try_move_to_back_and_forth(std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    if (!can_move_container())
        return false;

    auto const containers = resolve_scope(scope);
    if (containers.empty())
        return false;

    for (auto const& container : containers)
    {
        container->get_output()->delete_container(container);
        state->unfocus_container(container);

        if (workspace_manager->request_back_and_forth())
            output_manager->focused()->graft(container);
    }

    return true;
}

bool CommandController::try_move_to_scratchpad(std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    if (!can_move_container())
        return false;

    // Only floating or tiled windows can be moved to the scratchpad
    auto const containers = resolve_scope(scope);
    if (containers.empty())
        return false;

    for (auto const& container : containers)
    {
        if (!scratchpad_->move_to(container))
            return false;
    }

    return true;
}

bool CommandController::show_scratchpad()
{
    std::lock_guard lock(mutex);
    // TODO: Only show the window that meets the criteria
    return scratchpad_->toggle_show_all();
}

bool CommandController::can_move_container() const
{
    std::lock_guard lock(mutex);
    if (state->mode() != WindowManagerMode::normal)
        return false;

    if (state->focused_container() && state->focused_container()->is_fullscreen())
        return false;

    return true;
}

std::shared_ptr<ParentContainer> CommandController::toggle_floating_internal(std::shared_ptr<Container> const& container)
{
    switch (container->get_type())
    {
    case ContainerType::leaf:
    {
        auto focused_output = output_manager->focused();
        if (!focused_output)
            return nullptr;

        // Walk up the parent tree to get the root node.
        auto parent = container->get_parent().lock();
        if (!parent)
            return nullptr;

        while (!parent->get_parent().expired())
            parent = parent->get_parent().lock();

        // Remove the container from whatever workspace it is on.
        auto const workspace = container->get_workspace();
        workspace->delete_container(container);

        // If the parent is anchored, we move [container] to a new floating tree.
        if (parent->anchored())
        {
            geom::Rectangle new_area = {
                geom::Point {
                             container->get_logical_area().top_left.x.as_int() + 50,
                             container->get_logical_area().top_left.y.as_int() + 50 },
                geom::Size {
                             container->get_logical_area().size.width,
                             container->get_logical_area().size.height              }
            };
            auto new_parent = workspace->create_floating_tree(new_area);
            new_parent->graft_existing(container, new_parent->num_nodes());
            container->set_workspace(workspace);
            new_parent->commit_changes();
            return new_parent;
        }
        else
        {
            // Otherwise, we move the container to the root
            workspace->graft(container);
            return container->get_parent().lock();
        }
    }
    default:
        mir::log_warning("toggle_floating: has no effect on window of type: %d", (int)container->get_type());
        return nullptr;
    }
}

bool CommandController::toggle_floating(std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    if (state->mode() != WindowManagerMode::normal)
        return false;

    auto containers = resolve_scope(scope);
    if (containers.empty())
        return false;

    bool result = true;
    for (auto const& container : containers)
    {
        if (!toggle_floating_internal(container))
            result = false;
        else
        {
            container->for_each_observer([container](ContainerListener* listener)
            {
                listener->on_container_float(*container);
            });
        }
    }
    return result;
}

bool CommandController::toggle_pinned_to_workspace(std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    if (state->mode() != WindowManagerMode::normal)
        return false;

    auto containers = resolve_scope(scope);
    if (containers.empty())
        return false;

    bool result = true;
    for (auto const& container : containers)
    {
        if (!container->pinned(!container->pinned()))
            result = false;
    }
    return result;
}

bool CommandController::set_is_pinned(bool pinned, std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    if (state->mode() != WindowManagerMode::normal)
        return false;

    auto containers = resolve_scope(scope);
    if (containers.empty())
        return false;

    bool result = true;
    for (auto const& container : containers)
    {
        if (!container->pinned(pinned))
            result = false;
    }
    return result;
}

bool CommandController::toggle_tabbing(std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    if (!can_set_layout())
        return false;

    auto containers = resolve_scope(scope);
    if (containers.empty())
        return false;

    bool result = true;
    for (auto const& container : containers)
    {
        if (container->is_fullscreen())
        {
            result = false;
            continue;
        }

        if (!container->toggle_tabbing())
            result = false;
    }
    return result;
}

bool CommandController::toggle_stacking(std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    if (!can_set_layout())
        return false;

    auto containers = resolve_scope(scope);
    if (containers.empty())
        return false;

    bool result = true;
    for (auto const& container : containers)
    {
        if (container->is_fullscreen())
        {
            result = false;
            continue;
        }

        if (!container->toggle_stacking())
            result = false;
    }
    return result;
}

bool CommandController::set_layout(LayoutScheme scheme, std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    if (!can_set_layout())
        return false;

    auto containers = resolve_scope(scope);
    if (containers.empty())
        return false;

    bool result = true;
    for (auto const& container : containers)
    {
        if (container->is_fullscreen())
        {
            result = false;
            continue;
        }

        if (!container->set_layout(scheme))
            result = false;
    }
    return result;
}

bool CommandController::set_layout_default(std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    if (!can_set_layout())
        return false;

    auto containers = resolve_scope(scope);
    if (containers.empty())
        return false;

    bool result = true;
    for (auto const& container : containers)
    {
        if (container->is_fullscreen())
        {
            result = false;
            continue;
        }

        if (!container->set_layout(config->get_default_layout_scheme()))
            result = false;
    }
    return result;
}

void CommandController::move_cursor_to_output(OutputInterface const& output)
{
    auto const& extents = output.get_area();
    window_controller->move_cursor_to(
        extents.top_left.x.as_int() + extents.size.width.as_int() / 2.f,
        extents.top_left.y.as_int() + extents.size.height.as_int() / 2.f);
    output_manager->focus(output.id());
}

bool CommandController::try_select_next_output()
{
    std::lock_guard lock(mutex);
    for (size_t i = 0; i < output_manager->outputs().size(); i++)
    {
        if (output_manager->outputs()[i].get() == output_manager->focused())
        {
            size_t j = i + 1;
            if (j == output_manager->outputs().size())
                j = 0;

            move_cursor_to_output(*output_manager->outputs()[j]);
            return true;
        }
    }

    return false;
}

bool CommandController::try_select_prev_output()
{
    std::lock_guard lock(mutex);
    for (int i = output_manager->outputs().size() - 1; i >= 0; i++)
    {
        if (output_manager->outputs()[i].get() == output_manager->focused())
        {
            size_t j = i - 1;
            if (j < 0)
                j = output_manager->outputs().size() - 1;

            move_cursor_to_output(*output_manager->outputs()[j]);
            return true;
        }
    }

    return false;
}

bool CommandController::try_select_output(Direction direction)
{
    std::lock_guard lock(mutex);
    auto const next = output_manager->next(direction);
    if (next != output_manager->focused())
    {
        move_cursor_to_output(*next);
        return true;
    }

    return false;
}

std::vector<std::shared_ptr<Container>> CommandController::resolve_scope(std::vector<ContainerScope> const& scope_list)
{
    if (scope_list.empty())
    {
        if (auto focused = state->focused_container())
            return { focused };
        else
            return {};
    }

    // Check if we have a direct container reference
    for (auto const& scope : scope_list)
    {
        if (!scope.container.expired())
        {
            auto container = scope.container.lock();
            if (container)
                return { container };
        }
    }

    std::vector<std::shared_ptr<Container>> result;
    for (auto const& container : state->containers())
    {
        if (container.expired())
            continue;

        auto const& container_ptr = container.lock();
        bool matches = true;

        for (auto const& scope : scope_list)
        {
            if (!container_ptr->matches(scope))
            {
                matches = false;
                break;
            }
        }

        if (matches)
            result.push_back(container_ptr);
    }

    return result;
}

bool CommandController::try_select_output(std::vector<std::string> const& names)
{
    std::lock_guard lock(mutex);
    if (!output_manager->focused())
        return false;

    auto const output = output_manager->next_in_list(names);
    if (output != output_manager->focused())
        move_cursor_to_output(*output);
    return true;
}

bool CommandController::try_move_to_output_by_direction(Direction direction, std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    if (!output_manager->focused())
        return false;

    if (!can_move_container())
        return false;

    auto const containers = resolve_scope(scope);
    if (containers.empty())
        return false;

    auto const& next = output_manager->next(direction);
    if (next != output_manager->focused())
    {
        for (auto const& container : containers)
        {
            container->get_output()->delete_container(container);
            state->unfocus_container(container);

            next->graft(container);
            if (container->window().value())
                window_controller->select_active_window(container->window().value());
        }
        return true;
    }

    return false;
}

bool CommandController::try_move_to_current_output(std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    if (!output_manager->focused())
        return false;

    if (!can_move_container())
        return false;

    auto const containers = resolve_scope(scope);
    if (containers.empty())
        return false;

    for (auto const& container : containers)
    {
        if (container->get_output() == output_manager->focused())
            continue;

        container->get_output()->delete_container(container);
        state->unfocus_container(container);

        output_manager->focused()->graft(container);
        if (container->window().value())
            window_controller->select_active_window(container->window().value());
    }

    return true;
}

bool CommandController::try_move_to_primary_output(std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    if (output_manager->outputs().empty())
        return false;

    if (!can_move_container())
        return false;

    auto const containers = resolve_scope(scope);
    if (containers.empty())
        return false;

    auto const primary = output_manager->primary();
    for (auto const& container : containers)
    {
        if (container->get_output() == primary)
            continue;

        container->get_output()->delete_container(container);
        state->unfocus_container(container);

        primary->graft(container);
        if (container->window().value())
            window_controller->select_active_window(container->window().value());
    }

    return true;
}

bool CommandController::try_move_to_nonprimary_output(std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    auto const nonprimary = output_manager->non_primary();
    if (!nonprimary)
        return false;

    if (!can_move_container())
        return false;

    auto const containers = resolve_scope(scope);
    if (containers.empty())
        return false;

    for (auto const& container : containers)
    {
        container->get_output()->delete_container(container);
        state->unfocus_container(container);

        nonprimary->graft(container);
        if (container->window().value())
            window_controller->select_active_window(container->window().value());
    }

    return true;
}

bool CommandController::try_move_to_next_output(std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    if (!can_move_container())
        return false;
    auto const containers = resolve_scope(scope);
    if (containers.empty())
        return false;

    auto it = std::find_if(output_manager->outputs().begin(), output_manager->outputs().end(), [output_manager = output_manager](std::unique_ptr<OutputInterface> const& output)
    {
        return output.get() == output_manager->focused();
    });

    if (it == output_manager->outputs().end())
    {
        mir::log_error("CommandController::try_move_active_to_next: cannot find active output in list");
        return false;
    }

    it++;
    if (it == output_manager->outputs().end())
        it = output_manager->outputs().begin();

    if (it->get() == output_manager->focused())
        return false;

    if ((*it).get() == state->focused_container()->get_output())
        return false;

    for (auto const& container : containers)
    {
        container->get_output()->delete_container(container);
        state->unfocus_container(container);

        (*it)->graft(container);
        if (container->window().value())
            window_controller->select_active_window(container->window().value());
    }
    return true;
}

bool CommandController::try_move_to_output_by_name_list(std::vector<std::string> const& names, std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    if (!can_move_container())
        return false;

    auto const containers = resolve_scope(scope);
    if (containers.empty())
        return false;

    auto const& output = output_manager->next_in_list(names);
    if (output != state->focused_container()->get_output())
    {
        for (auto const& container : containers)
        {
            container->get_output()->delete_container(container);
            state->unfocus_container(container);

            output->graft(container);
            if (container->window().value())
                window_controller->select_active_window(container->window().value());
        }
    }

    return true;
}

bool CommandController::try_move_to_mark(std::string const& mark, std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    if (!can_move_container())
        return false;

    auto const containers = resolve_scope(scope);
    if (containers.empty())
        return false;

    // Find the first container matching the mark
    std::shared_ptr<Container> marked_container;
    for (auto const& container : state->containers())
    {
        if (auto const sh = container.lock())
        {
            if (std::ranges::find(sh->get_marks(), mark) != sh->get_marks().end())
            {
                marked_container = sh;
                break;
            }
        }
    }

    if (!marked_container)
        return false;

    // Graft the container onto the parent
    auto const parent = marked_container->get_parent().lock();
    auto const index = parent->get_index_of_node(marked_container);
    for (auto const& container : containers)
    {
        container->get_output()->delete_container(container);
        parent->graft_existing(container, static_cast<int>(index.value_or(-1) + 1)); // Insert at the position after!
    }

    return true;
}

bool CommandController::can_set_layout() const
{
    if (state->mode() != WindowManagerMode::normal)
        return false;

    return true;
}

bool CommandController::reload_config()
{
    std::lock_guard lock(mutex);
    config->reload();
    return true;
}

void CommandController::set_mode(WindowManagerMode mode)
{
    state->mode(mode);
    mode_observer_registrar->advise_changed(state->mode());
}

void CommandController::mark(
    std::vector<ContainerScope> const& scope,
    std::string const& mark,
    bool add,
    bool toggle)
{
    std::lock_guard lock(mutex);
    auto const containers = resolve_scope(scope);
    if (containers.empty())
        return;

    for (auto const& container : containers)
        container->mark(mark, add, toggle);
}

void CommandController::unmark(
    std::vector<ContainerScope> const& scope,
    std::string const& mark)
{
    std::lock_guard lock(mutex);
    auto const containers = resolve_scope(scope);
    if (containers.empty())
        return;

    for (auto const& container : containers)
        container->unmark(mark);
}

void CommandController::unmark_all(std::vector<ContainerScope> const& scope)
{
    std::lock_guard lock(mutex);
    auto const containers = resolve_scope(scope);
    if (containers.empty())
        return;

    for (auto const& container : containers)
        container->unmark_all();
}

std::unordered_set<std::string> CommandController::get_all_marks() const
{
    std::lock_guard lock(mutex);
    std::unordered_set<std::string> marks;
    for (auto const& container : state->containers())
    {
        if (auto const locked = container.lock())
        {
            for (auto const& mark : locked->get_marks())
                marks.insert(mark);
        }
    }

    return marks;
}

bool CommandController::rename_selected_workspace(WorkspaceIdentifier const& new_identifier)
{
    std::lock_guard lock(mutex);
    auto const selected_workspace = output_manager->focused()->active();
    if (!selected_workspace)
    {
        mir::log_error("rename_selected_workspace: could not find selected workspace");
        return false;
    }

    workspace_manager->set_workspace_num(selected_workspace->id(), new_identifier.number);
    workspace_manager->set_workspace_name(selected_workspace->id(), new_identifier.name);
    workspace_manager->request_focus(selected_workspace->id());
    return true;
}

bool CommandController::rename_existing_workspace(
    WorkspaceIdentifier const& existing_identifier,
    WorkspaceIdentifier const& new_identifier)
{
    std::lock_guard lock(mutex);
    auto const selected_workspace = output_manager->focused()->active();
    for (auto const& workspace : workspace_manager->workspaces())
    {
        if (workspace->num() == existing_identifier.number
            && workspace->name() == existing_identifier.name)
        {
            if (selected_workspace == workspace)
                return rename_selected_workspace(new_identifier);

            workspace_manager->set_workspace_num(workspace->id(), new_identifier.number);
            workspace_manager->set_workspace_name(workspace->id(), new_identifier.name);
            return true;
        }
    }

    mir::log_error("rename_existing_workspace: could not find requested workspace");
    return false;
}

bool CommandController::set_inner_gaps(size_t px, GapsChangeType type, bool current_workspace_only)
{
    std::unique_lock lock(mutex);
    auto const gaps_opt = [&]() -> std::optional<Gaps>
    {
        if (current_workspace_only)
        {
            auto const output = output_manager->focused();
            if (!output)
                return std::nullopt;

            auto const workspace = output->active();
            if (!workspace)
                return std::nullopt;

            if (auto const inner_gaps = workspace->inner_gaps())
                return inner_gaps;

            return Gaps();
        }

        return config->get_inner_gaps();
    }();

    if (!gaps_opt)
        return false;

    auto const current_inner_gaps = gaps_opt.value();
    auto const gaps_change = Gaps { px, px, px, px };
    Gaps next_inner_gaps;
    switch (type)
    {
    case GapsChangeType::set:
        next_inner_gaps = gaps_change;
        break;
    case GapsChangeType::plus:
        next_inner_gaps = current_inner_gaps + gaps_change;
        break;
    case GapsChangeType::minus:
        next_inner_gaps = current_inner_gaps - gaps_change;
        break;
    }

    lock.unlock();
    if (current_workspace_only)
        output_manager->focused()->active()->inner_gaps(next_inner_gaps);
    else
        config->override_inner_gaps(next_inner_gaps);
    return true;
}

bool CommandController::set_outer_gaps(
    size_t px,
    OuterGapsChange outer_gaps_change,
    GapsChangeType type,
    bool current_workspace_only)
{
    std::unique_lock lock(mutex);
    auto const gaps_opt = [&]() -> std::optional<Gaps>
    {
        if (current_workspace_only)
        {
            auto const output = output_manager->focused();
            if (!output)
                return std::nullopt;

            auto const workspace = output->active();
            if (!workspace)
                return std::nullopt;

            if (auto const outer_gaps = workspace->outer_gaps())
                return outer_gaps;
            return Gaps();
        }

        return config->get_outer_gaps();
    }();

    if (!gaps_opt)
        return false;

    Gaps gaps_change;
    switch (outer_gaps_change)
    {
    case OuterGapsChange::outer:
        gaps_change = Gaps { px, px, px, px };
        break;
    case OuterGapsChange::horizontal:
        gaps_change.left = px;
        gaps_change.right = px;
        break;
    case OuterGapsChange::vertical:
        gaps_change.top = px;
        gaps_change.bottom = px;
        break;
    case OuterGapsChange::top:
        gaps_change.top = px;
        break;
    case OuterGapsChange::right:
        gaps_change.right = px;
        break;
    case OuterGapsChange::bottom:
        gaps_change.bottom = px;
        break;
    case OuterGapsChange::left:
        gaps_change.left = px;
        break;
    }

    Gaps next_outer_gaps;
    switch (type)
    {
    case GapsChangeType::set:
    {
        next_outer_gaps = gaps_opt.value();
        switch (outer_gaps_change)
        {
        case OuterGapsChange::outer:
            next_outer_gaps = gaps_change;
            break;
        case OuterGapsChange::horizontal:
            next_outer_gaps.left = gaps_change.left;
            next_outer_gaps.right = gaps_change.right;
            break;
        case OuterGapsChange::vertical:
            next_outer_gaps.top = gaps_change.top;
            next_outer_gaps.bottom = gaps_change.bottom;
            break;
        case OuterGapsChange::top:
            next_outer_gaps.top = gaps_change.top;
            break;
        case OuterGapsChange::right:
            next_outer_gaps.right = gaps_change.right;
            break;
        case OuterGapsChange::bottom:
            next_outer_gaps.bottom = gaps_change.bottom;
            break;
        case OuterGapsChange::left:
            next_outer_gaps.left = gaps_change.left;
            break;
        }
        break;
    }
    case GapsChangeType::plus:
        next_outer_gaps = gaps_opt.value() + gaps_change;
        break;
    case GapsChangeType::minus:
        next_outer_gaps = gaps_opt.value() - gaps_change;
        break;
    }

    lock.unlock();

    if (current_workspace_only)
        output_manager->focused()->active()->outer_gaps(next_outer_gaps);
    else
        config->override_outer_gaps(next_outer_gaps);
    return true;
}

bool CommandController::try_move_workspace_to_output(OutputSelection selection)
{
    std::lock_guard lock(mutex);
    if (!output_manager->focused())
    {
        mir::log_warning("CommandController::try_move_workspace_to_output: cannot find focused output");
        return false;
    }

    auto const workspace = output_manager->focused()->active();
    if (!workspace)
    {
        mir::log_warning("CommandController::try_move_workspace_to_output: cannot find focused workspace");
        return false;
    }

    OutputInterface* output = nullptr;
    switch (selection)
    {
    case OutputSelection::left:
        output = output_manager->next(Direction::left);
        break;
    case OutputSelection::right:
        output = output_manager->next(Direction::right);
        break;
    case OutputSelection::down:
        output = output_manager->next(Direction::down);
        break;
    case OutputSelection::up:
        output = output_manager->next(Direction::up);
        break;
    case OutputSelection::current:
        output = output_manager->focused();
        break;
    case OutputSelection::primary:
        output = output_manager->primary();
        break;
    case OutputSelection::nonprimary:
        output = output_manager->non_primary();
        break;
    case OutputSelection::next:
        output = output_manager->next();
        break;
    }

    if (output == nullptr)
    {
        mir::log_warning("try_move_workspace_to_output: could not find output to move workspace to: %%d", selection);
        return false;
    }

    workspace_manager->move_workspace_to_output(workspace->id(), output);
    return true;
}

bool CommandController::try_move_workspace_to_outputs_by_name(std::vector<std::string> const& outputs)
{
    std::lock_guard lock(mutex);
    if (!output_manager->focused())
    {
        mir::log_warning("CommandController::try_move_workspace_to_output: cannot find focused output");
        return false;
    }

    auto const workspace = output_manager->focused()->active();
    if (!workspace)
    {
        mir::log_warning("CommandController::try_move_workspace_to_output: cannot find focused workspace");
        return false;
    }

    auto const output = output_manager->next_in_list(outputs);
    if (!output)
    {
        mir::log_warning("try_move_workspace_to_outputs_by_name: could not find output to move workspace to");
        return false;
    }

    workspace_manager->move_workspace_to_output(workspace->id(), output);
    return true;
}

nlohmann::json CommandController::to_json() const
{
    std::lock_guard lock(mutex);
    geom::Point top_left { INT_MAX, INT_MAX };
    geom::Point bottom_right { 0, 0 };
    nlohmann::json outputs_json = nlohmann::json::array();
    for (auto const& output : output_manager->outputs())
    {
        if (output->is_defunct())
            continue;

        auto& area = output->get_area();

        // Recalculate the total extents of the tree
        if (area.top_left.x.as_int() < top_left.x.as_int())
            top_left.x = geom::X { area.top_left.x.as_int() };
        if (area.top_left.y.as_int() < top_left.y.as_int())
            top_left.y = geom::Y { area.top_left.y.as_int() };

        int bottom_x = area.top_left.x.as_int() + area.size.width.as_int();
        int bottom_y = area.top_left.y.as_int() + area.size.height.as_int();
        if (bottom_x > bottom_right.x.as_int())
            bottom_right.x = geom::X { bottom_x };
        if (bottom_y > bottom_right.y.as_int())
            bottom_right.y = geom::Y { bottom_y };

        outputs_json.push_back(output->to_json(output_manager->focused() == output.get()));
    }

    geom::Rectangle total_area {
        top_left,
        geom::Size {
                    geom::Width(bottom_right.x.as_int() - top_left.x.as_int()),
                    geom::Height(bottom_right.y.as_int() - top_left.y.as_int()) }
    };
    nlohmann::json root = {
        { "id", 0 },
        { "name", "root" },
        {
         "rect",
         { { "x", total_area.top_left.x.as_int() }, { "y", total_area.top_left.y.as_int() }, { "width", total_area.size.width.as_int() }, { "height", total_area.size.height.as_int() } },
         },
        { "nodes", outputs_json },
        { "type", "root" }
    };
    return root;
}

nlohmann::json CommandController::outputs_json() const
{
    std::lock_guard lock(mutex);
    nlohmann::json j = nlohmann::json::array();
    for (auto const& output : output_manager->outputs())
    {
        if (output->is_defunct())
            continue;

        j.push_back(output->get_outputs_json(output_manager->focused() == output.get()));
    }
    return j;
}

nlohmann::json CommandController::workspaces_json() const
{
    std::lock_guard lock(mutex);
    nlohmann::json j = nlohmann::json::array();
    for (auto workspace : workspace_manager->workspaces())
    {
        if (workspace->get_output()->is_defunct())
            continue;

        j.push_back(workspace->get_workspaces_json(output_manager->focused() == workspace->get_output()));
    }
    return j;
}

nlohmann::json CommandController::workspace_to_json(uint32_t id) const
{
    std::lock_guard lock(mutex);
    auto workspace = workspace_manager->workspace(id);
    return workspace->to_json(output_manager->focused() == workspace->get_output());
}

nlohmann::json CommandController::mode_to_json() const
{
    std::lock_guard lock(mutex);
    switch (state->mode())
    {
    case WindowManagerMode::normal:
        return {
            { "name", "default" }
        };
    case WindowManagerMode::resizing:
        return {
            { "name", "resize" }
        };
    case WindowManagerMode::selecting:
        return {
            { "name", "selecting" }
        };
    case WindowManagerMode::dragging:
        return {
            { "name", "dragging" }
        };
    case WindowManagerMode::moving:
        return {
            { "name", "moving" }
        };
    default:
    {
        mir::fatal_error("handle_command: unknown binding state: %d", (int)state->mode());
        return {};
    }
    }
}
