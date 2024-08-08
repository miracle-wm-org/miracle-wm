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

#define MIR_LOG_COMPONENT "workspace_content"

#include "workspace.h"
#include "compositor_state.h"
#include "leaf_container.h"
#include "miracle_config.h"
#include "output.h"
#include "tiling_window_tree.h"
#include "window_helpers.h"
#include "parent_container.h"

#include "floating_window_container.h"
#include "shell_component_container.h"
#include <mir/log.h>
#include <mir/scene/surface.h>
#include <miral/zone.h>

using namespace miracle;

namespace
{
class OutputTilingWindowTreeInterface : public TilingWindowTreeInterface
{
public:
    explicit OutputTilingWindowTreeInterface(Output* screen, Workspace* workspace) :
        screen { screen },
        workspace { workspace }
    {
    }

    geom::Rectangle const& get_area() override
    {
        return screen->get_area();
    }

    std::vector<miral::Zone> const& get_zones() override
    {
        return screen->get_app_zones();
    }

    Workspace* get_workspace() const override
    {
        return workspace;
    }

private:
    Output* screen;
    Workspace* workspace;
};

}

Workspace::Workspace(
    miracle::Output* output,
    miral::WindowManagerTools const& tools,
    int workspace,
    std::shared_ptr<MiracleConfig> const& config,
    WindowController& window_controller,
    CompositorState const& state,
    std::shared_ptr<miral::MinimalWindowManager> const& floating_window_manager) :
    output { output },
    tools { tools },
    workspace { workspace },
    window_controller { window_controller },
    state { state },
    config { config },
    floating_window_manager { floating_window_manager },
    tree(std::make_shared<TilingWindowTree>(
        std::make_unique<OutputTilingWindowTreeInterface>(output, this),
        window_controller, state, config))
{
}

int Workspace::get_workspace() const
{
    return workspace;
}

void Workspace::set_area(mir::geometry::Rectangle const& area)
{
    tree->set_area(area);
}

void Workspace::recalculate_area()
{
    tree->recalculate_root_node_area();
}

ContainerType Workspace::allocate_position(
    miral::ApplicationInfo const& app_info,
    miral::WindowSpecification& requested_specification,
    ContainerType hint)
{
    // If there's no ideal layout type, use the one provided by the workspace
    auto layout = hint == ContainerType::none
        ? config->get_workspace_config(workspace).layout
        : hint;
    switch (layout)
    {
    case ContainerType::leaf:
    {
        requested_specification = tree->place_new_window(requested_specification, get_layout_container());
        return ContainerType::leaf;
    }
    case ContainerType::floating_window:
    {
        requested_specification = floating_window_manager->place_new_window(app_info, requested_specification);
        requested_specification.server_side_decorated() = false;
        return ContainerType::floating_window;
    }
    default:
        return layout;
    }
}

std::shared_ptr<Container> Workspace::create_container(
    miral::WindowInfo const& window_info, ContainerType type)
{
    std::shared_ptr<Container> container = nullptr;
    switch (type)
    {
    case ContainerType::leaf:
    {
        container = tree->confirm_window(window_info, get_layout_container());
        break;
    }
    case ContainerType::floating_window:
    {
        floating_window_manager->advise_new_window(window_info);
        container = add_floating_window(window_info.window());
        break;
    }
    case ContainerType::shell:
        if (window_info.state() == MirWindowState::mir_window_state_attached)
        {
            window_controller.select_active_window(window_info.window());
        }

        container = std::make_shared<ShellComponentContainer>(window_info.window(), window_controller);
        break;
    default:
        mir::log_error("Unsupported window type: %d", (int)type);
        break;
    }

    miral::WindowSpecification spec;
    spec.userdata() = container;
    spec.min_width() = mir::geometry::Width(0);
    spec.min_height() = mir::geometry::Height(0);
    window_controller.modify(window_info.window(), spec);

    // TODO: hack
    //  Warning: We need to advise fullscreen only after we've associated the userdata() appropriately
    if (type == ContainerType::leaf && window_helpers::is_window_fullscreen(window_info.state()))
    {
        tree->advise_fullscreen_container(*Container::as_leaf(container));
    }
    return container;
}

void Workspace::handle_ready_hack(LeafContainer& container)
{
    // TODO: Hack
    //  By default, new windows are raised. To properly maintain the ordering, we must
    //  raise floating windows and then raise fullscreen windows.
    for (auto const& window : floating_windows)
        window_controller.raise(window->window().value());

    if (tree->has_fullscreen_window())
        window_controller.raise(state.active->window().value());
}

void Workspace::delete_container(std::shared_ptr<Container> const &container)
{
    switch (container->get_type())
    {
    case ContainerType::leaf:
    {
        tree->advise_delete_window(container);
        break;
    }
    case ContainerType::floating_window:
    {
        auto floating = Container::as_floating(container);
        floating_window_manager->advise_delete_window(window_controller.info_for(floating->window().value()));
        floating_windows.erase(
            std::remove(floating_windows.begin(), floating_windows.end(), floating), floating_windows.end());
        break;
    }
    default:
        mir::log_error("Unsupported window type: %d", (int)container->get_type());
        return;
    }
}

void Workspace::show()
{
    auto fullscreen_node = tree->show();
    for (auto const& floating : floating_windows)
    {
        // Pinned windows don't require restoration
        if (floating->pinned())
        {
            tools.raise_tree(floating->window().value());
            continue;
        }

        auto container = window_controller.get_container(floating->window().value());
        if (!container)
        {
            mir::log_error("show: floating window lacks container");
            continue;
        }

        if (auto restore_state = container->restore_state())
        {
            miral::WindowSpecification spec;
            spec.state() = restore_state.value();
            tools.modify_window(floating->window().value(), spec);
            tools.raise_tree(floating->window().value());
        }
    }

    // TODO: ugh that's ugly. Fullscreen nodes should show above floating nodes
    if (fullscreen_node)
    {
        window_controller.select_active_window(fullscreen_node->window().value());
        window_controller.raise(fullscreen_node->window().value());
    }
}

void Workspace::for_each_window(std::function<void(std::shared_ptr<Container>)> const& f)
{
    for (auto const& window : floating_windows)
    {
        auto container = window_controller.get_container(window->window().value());
        if (container)
            f(container);
    }

    tree->foreach_node([&](std::shared_ptr<Container> const& node)
    {
        if (auto leaf = Container::as_leaf(node))
        {
            auto container = window_controller.get_container(leaf->window().value());
            if (container)
                f(container);
        }
    });
}

std::shared_ptr<Container> Workspace::select_from_point(int x, int y)
{
    for (auto const& floating : floating_windows)
    {
        auto window = floating->window().value();
        geom::Rectangle window_area(window.top_left(), window.size());

        if (window_area.contains(geom::Point(x, y)))
            return floating;
    }

    return tree->select_window_from_point(x, y);
}

void Workspace::toggle_floating(std::shared_ptr<Container> const& container)
{
    ContainerType new_type = ContainerType::none;
    auto window = container->window();
    if (!window)
        return;

    switch (container->get_type())
    {
    case ContainerType::leaf:
    {
        if (tree->has_fullscreen_window())
        {
            mir::log_warning("request_toggle_active_float: cannot float fullscreen window");
            return;
        }

        // First, remove the window from the tiling window tree
        delete_container(window_controller.get_container(*window));

        // Next, ask the floating window manager to place the new window
        auto& prev_info = window_controller.info_for(*window);
        auto spec = window_helpers::copy_from(prev_info);
        spec.top_left() = geom::Point { window->top_left().x.as_int() + 20, window->top_left().y.as_int() + 20 };
        window_controller.noclip(*window);
        auto new_spec = floating_window_manager->place_new_window(
            tools.info_for(window->application()),
            spec);
        tools.modify_window(*window, new_spec);

        new_type = ContainerType::floating_window;
        break;
    }
    case ContainerType::floating_window:
    {
        // First, remove the floating window
        delete_container(window_controller.get_container(*window));

        // Next, ask the tiling tree to place the new window
        auto& prev_info = window_controller.info_for(*window);
        miral::WindowSpecification spec = window_helpers::copy_from(prev_info);
        auto new_spec = tree->place_new_window(spec, nullptr);
        tools.modify_window(*window, new_spec);

        new_type = ContainerType::leaf;
        break;
    }
    default:
        mir::log_warning("toggle_floating: has no effect on window of type: %d", (int)container->get_type());
        return;
    }

    // In all cases, advise a new window and pretend like it is ready again
    auto& info = window_controller.info_for(*window);
    auto new_container = create_container(info, new_type);
    new_container->handle_ready();
    window_controller.select_active_window(state.active->window().value());
}

void Workspace::hide()
{
    tree->hide();

    for (auto const& floating : floating_windows)
    {
        auto window = floating->window().value();
        auto container = window_controller.get_container(window);
        if (!container)
        {
            mir::log_error("hide: floating window lacks container");
            continue;
        }

        container->restore_state(tools.info_for(window).state());
        miral::WindowSpecification spec;
        spec.state() = mir_window_state_hidden;
        tools.modify_window(window, spec);
        window_controller.send_to_back(window);
    }
}

void Workspace::transfer_pinned_windows_to(std::shared_ptr<Workspace> const& other)
{
    for (auto it = floating_windows.begin(); it != floating_windows.end();)
    {
        auto container = window_controller.get_container(it->get()->window().value());
        if (!container)
        {
            mir::log_error("transfer_pinned_windows_to: floating window lacks container");
            it++;
            continue;
        }

        auto floating = Container::as_floating(container);
        if (floating && floating->pinned())
        {
            other->floating_windows.push_back(floating);
            it = floating_windows.erase(it);
        }
        else
            it++;
    }
}

bool Workspace::has_floating_window(std::shared_ptr<Container> const& container)
{
    for (auto const& other : floating_windows)
    {
        if (other == container)
            return true;
    }

    return false;
}

std::shared_ptr<FloatingWindowContainer> Workspace::add_floating_window(miral::Window const& window)
{
    auto floating = std::make_shared<FloatingWindowContainer>(
        window, floating_window_manager, window_controller, this, state);
    floating_windows.push_back(floating);
    return floating;
}

Output* Workspace::get_output()
{
    return output;
}

void Workspace::trigger_rerender()
{
    // TODO: Ugh, sad. I am forced to set the surface transform so that the surface is rerendered
    for_each_window([&](std::shared_ptr<Container> const& container)
    {
        auto window = container->window();
        if (window)
        {
            auto surface = window->operator std::shared_ptr<mir::scene::Surface>();
            if (surface)
                surface->set_transformation(container->get_transform());
        }
    });
}

bool Workspace::is_empty() const
{
    return tree->is_empty() && floating_windows.empty();
}

void Workspace::graft(std::shared_ptr<Container> const& container)
{
    switch (container->get_type())
    {
        case ContainerType::floating_window:
        {
            auto floating = Container::as_floating(container);
            floating->set_workspace(this);
            floating_windows.push_back(floating);
            break;
        }
        case ContainerType::parent:
            tree->graft(Container::as_parent(container));
            break;
        case ContainerType::leaf:
            tree->graft(Container::as_leaf(container));
            break;
        default:
            mir::log_error("Workspace::graft: ungraftable container type: %d", (int)container->get_type());
            break;
    }
}

int Workspace::workspace_to_number(int workspace)
{
    if (workspace == 0)
        return 10;

    return workspace - 1;
}

std::shared_ptr<ParentContainer> Workspace::get_layout_container()
{
    if (!state.active)
        return nullptr;

    auto parent = state.active->get_parent().lock();
    if (!parent)
        return nullptr;

    if (parent->get_workspace() != this)
        return nullptr;

    return parent;
}