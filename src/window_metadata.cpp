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
#include "compositor_state.h"
#include "output.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

using namespace miracle;

WindowType miracle::window_type_from_string(std::string const& str)
{
    if (str == "tiled")
        return WindowType::tiled;
    else if (str == "floating")
        return WindowType::floating;
    else if (str == "other")
        return WindowType::other;
    else
        return WindowType::none;
}

WindowMetadata::WindowMetadata(WindowType type, miral::Window const& window) :
    WindowMetadata(type, window, nullptr)
{
}

WindowMetadata::WindowMetadata(
    miracle::WindowType type,
    miral::Window const& window,
    Workspace* workspace) :
    type { type },
    window { window },
    workspace { workspace }
{
}

void WindowMetadata::associate_container(std::shared_ptr<Container> const& node)
{
    container = node;
}

void WindowMetadata::set_restore_state(MirWindowState state)
{
    restore_state = state;
}

std::optional<MirWindowState> WindowMetadata::consume_restore_state()
{
    auto state = restore_state;
    restore_state.reset();
    return state;
}

bool WindowMetadata::is_focused() const
{
    if (!workspace)
        return false;

    auto output = workspace->get_output();
    if (!output)
        return false;

    return output->get_state().active_window == window;
}

Workspace* WindowMetadata::get_workspace() const
{
    return workspace;
}

std::shared_ptr<Container> WindowMetadata::get_container() const
{
    return container;
}

uint32_t WindowMetadata::get_animation_handle() const
{
    return animation_handle;
}

void WindowMetadata::set_animation_handle(uint32_t handle)
{
    animation_handle = handle;
}

Output* WindowMetadata::get_output() const
{
    if (!workspace)
        return nullptr;

    return workspace->get_output();
}

glm::mat4 WindowMetadata::get_workspace_transform() const
{
    auto output = get_output();
    if (!output)
        return glm::mat4(1.f);

    auto const workspace_rect = output->get_workspace_rectangle(workspace->get_workspace());
    return glm::translate(
        glm::vec3(workspace_rect.top_left.x.as_int(), workspace_rect.top_left.y.as_int(), 0));
}

glm::mat4 WindowMetadata::get_output_transform() const
{
    auto output = get_output();
    if (!output)
        return glm::mat4(1.f);

    return output->get_transform();
}
