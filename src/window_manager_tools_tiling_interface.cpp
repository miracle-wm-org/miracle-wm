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

#include "window_manager_tools_tiling_interface.h"
#include "animator.h"
#include "compositor_state.h"
#include "leaf_node.h"
#include "window_helpers.h"
#include "window_metadata.h"
#include <mir/scene/surface.h>

#define MIR_LOG_COMPONENT "window_manager_tools_tiling_interface"
#include <glm/gtc/matrix_transform.hpp>
#include <mir/log.h>

using namespace miracle;

WindowManagerToolsTilingInterface::WindowManagerToolsTilingInterface(
    miral::WindowManagerTools const& tools,
    Animator& animator,
    CompositorState& state) :
    tools { tools },
    animator { animator },
    state { state }
{
}

void WindowManagerToolsTilingInterface::open(miral::Window const& window)
{
    auto metadata = get_metadata(window);
    if (!metadata)
    {
        mir::log_error("Cannot set rectangle of window that lacks metadata");
        return;
    }

    animator.window_open(
        metadata->get_animation_handle(),
        [this, metadata = metadata](miracle::AnimationStepResult const& result)
    {
        on_animation(result, metadata);
    });
}

bool WindowManagerToolsTilingInterface::is_fullscreen(miral::Window const& window)
{
    auto& info = tools.info_for(window);
    return window_helpers::is_window_fullscreen(info.state());
}

void WindowManagerToolsTilingInterface::set_rectangle(
    miral::Window const& window, geom::Rectangle const& from, geom::Rectangle const& to)
{
    auto metadata = get_metadata(window);
    if (!metadata)
    {
        mir::log_error("Cannot set rectangle of window that lacks metadata");
        return;
    }

    animator.window_move(
        metadata->get_animation_handle(),
        from,
        to,
        geom::Rectangle { window.top_left(), window.size() },
        [this, metadata = metadata](miracle::AnimationStepResult const& result)
    {
        on_animation(result, metadata);
    });
}

MirWindowState WindowManagerToolsTilingInterface::get_state(miral::Window const& window)
{
    auto& window_info = tools.info_for(window);
    return window_info.state();
}

void WindowManagerToolsTilingInterface::change_state(miral::Window const& window, MirWindowState state)
{
    auto& window_info = tools.info_for(window);
    miral::WindowSpecification spec;
    spec.state() = state;
    tools.place_and_size_for_state(spec, window_info);
    tools.modify_window(window, spec);
}

void WindowManagerToolsTilingInterface::clip(miral::Window const& window, geom::Rectangle const& r)
{
    auto& window_info = tools.info_for(window);
    window_info.clip_area(r);
}

void WindowManagerToolsTilingInterface::noclip(miral::Window const& window)
{
    auto& window_info = tools.info_for(window);
    window_info.clip_area(mir::optional_value<geom::Rectangle>());
}

void WindowManagerToolsTilingInterface::select_active_window(miral::Window const& window)
{
    if (state.mode == WindowManagerMode::resizing)
        return;

    tools.select_active_window(window);
}

std::shared_ptr<WindowMetadata> WindowManagerToolsTilingInterface::get_metadata(miral::Window const& window)
{
    auto& info = tools.info_for(window);
    if (info.userdata())
        return static_pointer_cast<WindowMetadata>(info.userdata());

    return nullptr;
}

std::shared_ptr<WindowMetadata> WindowManagerToolsTilingInterface::get_metadata(
    miral::Window const& window, TilingWindowTree const* tree)
{
    auto node = get_metadata(window);
    if (auto tiling_node = node->get_tiling_node())
    {
        if (tiling_node->get_tree() == tree)
            return node;
    }

    return nullptr;
}

void WindowManagerToolsTilingInterface::raise(miral::Window const& window)
{
    tools.raise_tree(window);
}

void WindowManagerToolsTilingInterface::send_to_back(miral::Window const& window)
{
    tools.send_tree_to_back(window);
}

void WindowManagerToolsTilingInterface::on_animation(
    miracle::AnimationStepResult const& result, std::shared_ptr<WindowMetadata> const& metadata)
{
    auto window = metadata->get_window();
    auto surface = window.operator std::shared_ptr<mir::scene::Surface>();
    if (!surface)
        return;

    bool needs_modify = false;
    miral::WindowSpecification spec;
    if (auto node = metadata->get_tiling_node())
    {
        spec.top_left() = node->get_visible_area().top_left;
        spec.size() = node->get_visible_area().size;
    }
    else
    {
        spec.top_left() = window.top_left();
        spec.size() = window.size();
    }

    spec.min_width() = mir::geometry::Width(0);
    spec.min_height() = mir::geometry::Height(0);

    if (result.position)
    {
        spec.top_left() = mir::geometry::Point(
            result.position.value().x,
            result.position.value().y);
        needs_modify = true;
    }

    if (result.size)
    {
        spec.size() = mir::geometry::Size(
            result.size.value().x,
            result.size.value().y);
        needs_modify = true;
    }

    if (needs_modify)
    {
        tools.modify_window(window, spec);

        auto& window_info = tools.info_for(window);
        for (auto const& child : window_info.children())
        {
            miral::WindowSpecification sub_spec;
            sub_spec.top_left() = spec.top_left();
            sub_spec.size() = spec.size();
            tools.modify_window(child, sub_spec);
        }
    }

    if (result.transform && result.transform.value() != metadata->get_transform())
    {
        metadata->set_transform(result.transform.value());
        surface->set_transformation(result.transform.value());
    }

    // NOTE: The clip area needs to reflect the current position + transform of the window.
    // Failing to set a clip area will cause overflowing windows to briefly disregard their
    // compacted size.
    // TODO: When we have rotation in our transforms, then we need to handle rotations.
    //  At that point, the top_left corner will change. We will need to find an AABB
    //  to represent the clip area.
    auto transform = metadata->get_transform();
    auto width = spec.size().value().width.as_int();
    auto height = spec.size().value().height.as_int();

    glm::vec4 scale = transform * glm::vec4(width, height, 0, 1);

    mir::geometry::Rectangle new_rectangle(
        { spec.top_left().value().x.as_int(), spec.top_left().value().y.as_int() },
        { scale.x, scale.y });

    if (metadata->get_type() == WindowType::tiled)
        clip(window, new_rectangle);
    else
        noclip(window);
}