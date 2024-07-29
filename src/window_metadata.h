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

#ifndef MIRACLEWM_WINDOW_METADATA_H
#define MIRACLEWM_WINDOW_METADATA_H

#include <glm/glm.hpp>
#include <memory>
#include <miral/window.h>
#include <miral/window_manager_tools.h>

namespace miracle
{
class Workspace;
class Container;
class TilingWindowTree;
class Output;

enum class WindowType
{
    none,
    tiled,
    floating,
    other
};

WindowType window_type_from_string(std::string const&);

class WindowMetadata
{
public:
    WindowMetadata(WindowType type, miral::Window const& window);
    WindowMetadata(WindowType type, miral::Window const& window, Workspace* workspace);
    void associate_container(std::shared_ptr<Container> const&);
    miral::Window& get_window() { return window; }
    [[nodiscard]] std::shared_ptr<Container> get_container() const;
    [[nodiscard]] WindowType get_type() const { return type; }
    void set_restore_state(MirWindowState state);
    std::optional<MirWindowState> consume_restore_state();
    [[nodiscard]] bool is_focused() const;
    [[nodiscard]] Workspace* get_workspace() const;
    [[nodiscard]] uint32_t get_animation_handle() const;
    void set_animation_handle(uint32_t);
    [[nodiscard]] Output* get_output() const;
    [[nodiscard]] glm::mat4 const& get_transform() const { return transform; }
    void set_transform(glm::mat4 const& in) { transform = in; }
    [[nodiscard]] glm::mat4 get_workspace_transform() const;
    [[nodiscard]] glm::mat4 get_output_transform() const;

private:
    WindowType type;
    miral::Window window;
    Workspace* workspace;
    std::shared_ptr<Container> container;
    std::optional<MirWindowState> restore_state;
    uint32_t animation_handle = 0;
    glm::mat4 transform = glm::mat4(1.f);
};

}

#endif // MIRACLEWM_WINDOW_METADATA_H
