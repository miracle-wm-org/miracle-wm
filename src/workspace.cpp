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
#include "window_metadata.h"
#include "floating_container.h"
#include <mir/log.h>
#include <mir/scene/surface.h>
#include <miral/zone.h>

using namespace miracle;

namespace
{
class OutputTilingWindowTreeInterface : public TilingWindowTreeInterface
{
public:
    explicit OutputTilingWindowTreeInterface(miracle::Output* screen) :
        screen { screen }
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

private:
    miracle::Output* screen;
};

}

Workspace::Workspace(
    miracle::Output* output,
    miral::WindowManagerTools const& tools,
    int workspace,
    std::shared_ptr<MiracleConfig> const& config,
    WindowController& window_controller,
    CompositorState const& state,
    miral::MinimalWindowManager& floating_window_manager) :
    output { output },
    tools { tools },
    workspace { workspace },
    window_controller { window_controller },
    state { state },
    config { config },
    floating_window_manager { floating_window_manager },
    tree(std::make_shared<TilingWindowTree>(
        std::make_unique<OutputTilingWindowTreeInterface>(output),
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

WindowType Workspace::allocate_position(
    miral::ApplicationInfo const& app_info,
    miral::WindowSpecification& requested_specification,
    WindowType hint)
{
    // If there's no ideal layout type, use the one provided by the workspace
    auto layout = hint == WindowType::none
        ? config->get_workspace_config(workspace).layout
        : hint;
    switch (layout)
    {
    case WindowType::tiled:
    {
        requested_specification = tree->place_new_window(requested_specification, get_layout_container());
        return WindowType::tiled;
    }
    case WindowType::floating:
    {
        requested_specification = floating_window_manager.place_new_window(app_info, requested_specification);
        requested_specification.server_side_decorated() = false;
        return WindowType::floating;
    }
    default:
        return layout;
    }
}

std::shared_ptr<WindowMetadata> Workspace::advise_new_window(
    miral::WindowInfo const& window_info, WindowType type)
{
    std::shared_ptr<WindowMetadata> metadata = nullptr;
    switch (type)
    {
    case WindowType::tiled:
    {
        auto container = tree->confirm_window(window_info, get_layout_container());
        metadata = std::make_shared<WindowMetadata>(WindowType::tiled, window_info.window(), this);
        metadata->associate_container(container);
        break;
    }
    case WindowType::floating:
    {
        floating_window_manager.advise_new_window(window_info);
        metadata = std::make_shared<WindowMetadata>(WindowType::floating, window_info.window(), this);
        metadata->associate_container(add_floating_window(window_info.window()));
        break;
    }
    case WindowType::other:
        if (window_info.state() == MirWindowState::mir_window_state_attached)
        {
            window_controller.select_active_window(window_info.window());
        }
        metadata = std::make_shared<WindowMetadata>(WindowType::other, window_info.window());
        break;
    default:
        mir::log_error("Unsupported window type: %d", (int)type);
        break;
    }

    if (metadata)
    {
        miral::WindowSpecification spec;
        spec.userdata() = metadata;
        spec.min_width() = mir::geometry::Width(0);
        spec.min_height() = mir::geometry::Height(0);
        window_controller.modify(window_info.window(), spec);

        // Warning: We need to advise fullscreen only after we've associated the userdata() appropriately
        if (type == WindowType::tiled && window_helpers::is_window_fullscreen(window_info.state()))
        {
            tree->advise_fullscreen_container(metadata->get_container());
        }
        return metadata;
    }
    else
    {
        mir::log_error("Window failed to set metadata");
        return nullptr;
    }
}

mir::geometry::Rectangle Workspace::confirm_placement_on_display(
    std::shared_ptr<miracle::WindowMetadata> const& metadata,
    MirWindowState new_state,
    mir::geometry::Rectangle const& new_placement)
{
    mir::geometry::Rectangle modified_placement = new_placement;
    switch (metadata->get_type())
    {
    case WindowType::tiled:
    {
        tree->confirm_placement_on_display(
            metadata->get_container(), new_state, modified_placement);
        break;
    }
    case WindowType::floating:
        return floating_window_manager.confirm_placement_on_display(
            window_controller.info_for(metadata->get_window()), new_state, new_placement);
    default:
        mir::log_error("Unsupported window type: %d", (int)metadata->get_type());
        break;
    }
    return modified_placement;
}

void Workspace::handle_window_ready(
    miral::WindowInfo& window_info, std::shared_ptr<miracle::WindowMetadata> const& metadata)
{
    switch (metadata->get_type())
    {
    case WindowType::tiled:
    {
        tree->handle_container_ready(metadata->get_container());

        // Note: By default, new windows are raised. To properly maintain the ordering, we must
        // raise floating windows and then raise fullscreen windows.
        for (auto const& window : floating_windows)
            window_controller.raise(window->window());

        if (tree->has_fullscreen_window())
            window_controller.raise(window_info.window());
        break;
    }
    case WindowType::floating:
        floating_window_manager.handle_window_ready(window_info);
        break;
    default:
        mir::log_error("Unsupported window type: %d", (int)metadata->get_type());
        return;
    }
}

void Workspace::advise_focus_gained(const std::shared_ptr<miracle::WindowMetadata>& metadata)
{
    switch (metadata->get_type())
    {
    case WindowType::tiled:
    {
        tree->advise_focus_gained(metadata->get_container());
        break;
    }
    case WindowType::floating:
        floating_window_manager.advise_focus_gained(window_controller.info_for(metadata->get_window()));
        break;
    default:
        mir::log_error("Unsupported window type: %d", (int)metadata->get_type());
        return;
    }
}

void Workspace::advise_focus_lost(const std::shared_ptr<miracle::WindowMetadata>& metadata)
{
    switch (metadata->get_type())
    {
    case WindowType::tiled:
        break;
    case WindowType::floating:
        floating_window_manager.advise_focus_lost(window_controller.info_for(metadata->get_window()));
        break;
    default:
        mir::log_error("Unsupported window type: %d", (int)metadata->get_type());
        return;
    }
}

void Workspace::advise_delete_window(std::shared_ptr<miracle::WindowMetadata> const& metadata)
{
    switch (metadata->get_type())
    {
    case WindowType::tiled:
    {
        metadata->get_container()->get_tree()->advise_delete_window(metadata->get_container());
        break;
    }
    case WindowType::floating:
        floating_window_manager.advise_delete_window(window_controller.info_for(metadata->get_window()));
        remove_floating_window(metadata->get_window());
        break;
    default:
        mir::log_error("Unsupported window type: %d", (int)metadata->get_type());
        return;
    }
}

void Workspace::advise_move_to(
    std::shared_ptr<miracle::WindowMetadata> const& metadata,
    geom::Point const& top_left)
{
    if (metadata->get_type() == WindowType::floating)
        floating_window_manager.advise_move_to(window_controller.info_for(metadata->get_window()), top_left);
}

void Workspace::handle_request_move(
    const std::shared_ptr<miracle::WindowMetadata>& metadata,
    const MirInputEvent* input_event)
{
    if (metadata->get_type() == WindowType::floating)
        floating_window_manager.handle_request_move(
            window_controller.info_for(metadata->get_window()), input_event);
}

void Workspace::handle_request_resize(
    const std::shared_ptr<miracle::WindowMetadata>& metadata,
    const MirInputEvent* input_event,
    MirResizeEdge edge)
{
    if (metadata->get_type() == WindowType::floating)
        floating_window_manager.handle_request_resize(
            window_controller.info_for(metadata->get_window()), input_event, edge);
}

void Workspace::handle_modify_window(
    const std::shared_ptr<miracle::WindowMetadata>& metadata,
    const miral::WindowSpecification& modifications)
{
    switch (metadata->get_type())
    {
    case WindowType::tiled:
    {
        auto& window = metadata->get_window();
        auto node = metadata->get_container();
        auto const& info = window_controller.info_for(window);
        if (tree.get() != node->get_tree())
            break;

        auto mods = modifications;
        if (mods.state().is_set() && mods.state().value() != info.state())
        {
            node->set_state(mods.state().value());
            node->commit_changes();

            if (window_helpers::is_window_fullscreen(mods.state().value()))
                tree->advise_fullscreen_container(node);
            else if (mods.state().value() == mir_window_state_restored)
                tree->advise_restored_container(node);
        }

        // If we are trying to set the window size to something that we don't want it
        // to be, then let's consume it.
        if (!node->is_fullscreen()
            && mods.size().is_set()
            && node->get_visible_area().size != mods.size().value())
        {
            mods.size().consume();
        }

        window_controller.modify(window, mods);
        break;
    }
    case WindowType::floating:
        if (!has_floating_window(metadata->get_window()))
            break;

        floating_window_manager.handle_modify_window(window_controller.info_for(metadata->get_window()), modifications);
        break;
    default:
        mir::log_error("Unsupported window type: %d", (int)metadata->get_type());
        return;
    }
}

void Workspace::handle_raise_window(std::shared_ptr<miracle::WindowMetadata> const& metadata)
{
    switch (metadata->get_type())
    {
    case WindowType::tiled:
        window_controller.select_active_window(metadata->get_window());
        break;
    case WindowType::floating:
        floating_window_manager.handle_raise_window(window_controller.info_for(metadata->get_window()));
        break;
    default:
        mir::log_error("handle_raise_window: unsupported window type: %d", (int)metadata->get_type());
        return;
    }
}

bool Workspace::move_active_window(Direction direction)
{
    auto metadata = window_helpers::get_metadata(state.active_window, tools);
    if (!metadata)
        return false;

    switch (metadata->get_type())
    {
    case WindowType::floating:
        return move_active_window_by_amount(direction, 10);
    case WindowType::tiled:
        return tree->move_container(direction, metadata->get_container());
    default:
        mir::log_error("move_active_window is not defined for window of type %d", (int)metadata->get_type());
        return false;
    }
}

bool Workspace::move_active_window_by_amount(Direction direction, int pixels)
{
    auto metadata = window_helpers::get_metadata(state.active_window, tools);
    if (!metadata)
        return false;

    if (metadata->get_type() != WindowType::floating)
    {
        mir::log_warning("Cannot move a non-floating window by an amount, type=%d", (int)metadata->get_type());
        return false;
    }

    auto& info = window_controller.info_for(state.active_window);
    auto prev_pos = state.active_window.top_left();
    miral::WindowSpecification spec;
    switch (direction)
    {
    case Direction::down:
        spec.top_left() = {
            prev_pos.x.as_int(), prev_pos.y.as_int() + pixels
        };
        break;
    case Direction::up:
        spec.top_left() = {
            prev_pos.x.as_int(), prev_pos.y.as_int() - pixels
        };
        break;
    case Direction::left:
        spec.top_left() = {
            prev_pos.x.as_int() - pixels, prev_pos.y.as_int()
        };
        break;
    case Direction::right:
        spec.top_left() = {
            prev_pos.x.as_int() + pixels, prev_pos.y.as_int()
        };
        break;
    default:
        mir::log_warning("Unknown direction to move_active_window_by_amount: %d\n", (int)direction);
        return false;
    }

    window_controller.modify(info.window(), spec);
    return true;
}

bool Workspace::move_active_window_to(int x, int y)
{
    auto metadata = window_helpers::get_metadata(state.active_window, tools);
    if (!metadata)
        return false;

    if (metadata->get_type() != WindowType::floating)
    {
        mir::log_warning("Cannot move a non-floating window to a position, type=%d", (int)metadata->get_type());
        return false;
    }

    miral::WindowSpecification spec;
    spec.top_left() = { x, y };
    window_controller.modify(state.active_window, spec);
    return true;
}

void Workspace::show()
{
    auto fullscreen_node = tree->show();
    for (auto const& floating : floating_windows)
    {
        // Pinned windows don't require restoration
        if (floating->pinned())
        {
            tools.raise_tree(floating->window());
            continue;
        }

        auto metadata = window_helpers::get_metadata(floating->window(), tools);
        if (!metadata)
        {
            mir::log_error("show: floating window lacks metadata");
            continue;
        }

        if (auto state = metadata->consume_restore_state())
        {
            miral::WindowSpecification spec;
            spec.state() = state.value();
            tools.modify_window(floating->window(), spec);
            tools.raise_tree(floating->window());
        }
    }

    // TODO: ugh that's ugly. Fullscreen nodes should show above floating nodes
    if (fullscreen_node)
    {
        window_controller.select_active_window(fullscreen_node->get_window());
        window_controller.raise(fullscreen_node->get_window());
    }
}

void Workspace::for_each_window(std::function<void(std::shared_ptr<WindowMetadata>)> const& f)
{
    for (auto const& window : floating_windows)
    {
        auto metadata = window_helpers::get_metadata(window->window(), tools);
        if (metadata)
            f(metadata);
    }

    tree->foreach_node([&](std::shared_ptr<Container> const& node)
    {
        if (auto leaf = Container::as_leaf(node))
        {
            auto metadata = window_helpers::get_metadata(leaf->get_window(), tools);
            if (metadata)
                f(metadata);
        }
    });
}

bool Workspace::select_window_from_point(int x, int y)
{
    if (tree->has_fullscreen_window())
        return false;

    for (auto const& floating : floating_windows)
    {
        auto window = floating->window();
        geom::Rectangle window_area(window.top_left(), window.size());
        if (window == state.active_window && window_area.contains(geom::Point(x, y)))
            return false;
        else if (window_area.contains(geom::Point(x, y)))
        {
            window_controller.select_active_window(window);
            return true;
        }
    }

    auto node = tree->select_window_from_point(x, y);
    if (node && node->get_window() != state.active_window)
    {
        window_controller.select_active_window(node->get_window());
        return true;
    }

    return false;
}

bool Workspace::resize_active_window(miracle::Direction direction)
{
    auto metadata = window_controller.get_metadata(state.active_window);
    if (!metadata)
        return false;

    return tree->resize_container(direction, metadata->get_container());
}

bool Workspace::select(miracle::Direction direction)
{
    auto metadata = window_controller.get_metadata(state.active_window);
    if (!metadata)
        return false;

    return tree->select_next(direction, metadata->get_container());
}

void Workspace::request_horizontal_layout()
{
    auto metadata = window_controller.get_metadata(state.active_window);
    if (!metadata)
        return;

    tree->request_horizontal_layout(metadata->get_container());
}

void Workspace::request_vertical_layout()
{
    auto metadata = window_controller.get_metadata(state.active_window);
    if (!metadata)
        return;

    tree->request_vertical_layout(metadata->get_container());
}

void Workspace::toggle_layout()
{
    auto metadata = window_controller.get_metadata(state.active_window);
    if (!metadata)
        return;

    tree->toggle_layout(metadata->get_container());
}

bool Workspace::try_toggle_active_fullscreen()
{
    auto metadata = window_controller.get_metadata(state.active_window);
    if (!metadata)
        return false;

    return tree->toggle_fullscreen(metadata->get_container());
}

void Workspace::toggle_floating(std::shared_ptr<WindowMetadata> const& metadata)
{
    WindowType new_type = WindowType::none;
    auto window = metadata->get_window();
    switch (metadata->get_type())
    {
    case WindowType::tiled:
    {
        if (tree->has_fullscreen_window())
        {
            mir::log_warning("request_toggle_active_float: cannot float fullscreen window");
            return;
        }

        // First, remove the window from the tiling window tree
        advise_delete_window(window_helpers::get_metadata(window, tools));

        // Next, ask the floating window manager to place the new window
        auto& prev_info = window_controller.info_for(window);
        auto spec = window_helpers::copy_from(prev_info);
        spec.top_left() = geom::Point { window.top_left().x.as_int() + 20, window.top_left().y.as_int() + 20 };
        window_controller.noclip(window);
        auto new_spec = floating_window_manager.place_new_window(
            tools.info_for(window.application()),
            spec);
        tools.modify_window(window, new_spec);

        new_type = WindowType::floating;
        break;
    }
    case WindowType::floating:
    {
        // First, remove the floating window
        advise_delete_window(window_helpers::get_metadata(window, tools));

        // Next, ask the tiling tree to place the new window
        auto& prev_info = window_controller.info_for(window);
        miral::WindowSpecification spec = window_helpers::copy_from(prev_info);
        auto new_spec = tree->place_new_window(spec, nullptr);
        tools.modify_window(window, new_spec);

        new_type = WindowType::tiled;
        break;
    }
    default:
        mir::log_warning("toggle_floating: has no effect on window of type: %d", (int)metadata->get_type());
        return;
    }

    // In all cases, advise a new window and pretend like it is ready again
    auto& info = window_controller.info_for(window);
    auto new_metadata = advise_new_window(info, new_type);
    handle_window_ready(info, new_metadata);
    window_controller.select_active_window(state.active_window);
}

void Workspace::hide()
{
    tree->hide();

    for (auto const& floating : floating_windows)
    {
        auto window = floating->window();
        auto metadata = window_helpers::get_metadata(window, tools);
        if (!metadata)
        {
            mir::log_error("hide: floating window lacks metadata");
            continue;
        }

        metadata->set_restore_state(tools.info_for(window).state());
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
        auto metadata = window_helpers::get_metadata(it->get()->window(), tools);
        if (!metadata)
        {
            mir::log_error("transfer_pinned_windows_to: floating window lacks metadata");
            it++;
            continue;
        }

        if (it->get()->pinned())
        {
            metadata->associate_container(other->add_floating_window(it->get()->window()));
            it = floating_windows.erase(it);
        }
        else
            it++;
    }
}

bool Workspace::has_floating_window(miral::Window const& window)
{
    for (auto const& other : floating_windows)
    {
        if (other->window() == window)
            return true;
    }

    return false;
}

std::shared_ptr<LeafContainer> Workspace::add_floating_window(miral::Window const& window)
{
    auto floating = std::make_shared<FloatingContainer>();
    auto leaf = std::make_shared<LeafContainer>(
        window_controller,
        geom::Rectangle{window.top_left(), window.size()},
        config,
        nullptr,
        floating
    );
    floating->add_leaf(leaf);
    floating_windows.push_back(floating);
    return leaf;
}

void Workspace::remove_floating_window(miral::Window const& window)
{
    floating_windows.erase(std::remove_if(floating_windows.begin(), floating_windows.end(), [&window](std::shared_ptr<FloatingContainer> const& floating)
    {
        return floating->window() == window;
    }));
}

Output* Workspace::get_output()
{
    return output;
}

void Workspace::trigger_rerender()
{
    // TODO: Ugh, sad. I am forced to set the surface transform so that the surface is rerendered
    for_each_window([&](std::shared_ptr<WindowMetadata> const& metadata)
    {
        auto& window = metadata->get_window();
        auto surface = window.operator std::shared_ptr<mir::scene::Surface>();
        if (surface)
            surface->set_transformation(metadata->get_transform());
    });
}

bool Workspace::is_empty() const
{
    return tree->is_empty() && floating_windows.empty();
}

int Workspace::workspace_to_number(int workspace)
{
    if (workspace == 0)
        return 10;

    return workspace - 1;
}

std::shared_ptr<ParentContainer> Workspace::get_layout_container()
{
    if (!state.active_window)
        return nullptr;

    auto metadata = window_controller.get_metadata(state.active_window);
    if (!metadata)
        return nullptr;

    auto container = metadata->get_container();
    if (!container)
        return nullptr;

    auto parent = container->get_parent().lock();
    if (!parent)
        return nullptr;

    return Container::as_parent(parent);
}