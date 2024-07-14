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
class WorkspaceContent;
class LeafContainer;
class TilingWindowTree;
class OutputContent;

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
    WindowMetadata(WindowType type, miral::Window const& window, WorkspaceContent* workspace);
    void associate_to_node(std::shared_ptr<LeafContainer> const&);
    miral::Window& get_window() { return window; }
    std::shared_ptr<LeafContainer> get_tiling_node() const;
    WindowType get_type() const { return type; }
    bool get_is_pinned() const { return is_pinned; }
    void set_restore_state(MirWindowState state);
    std::optional<MirWindowState> consume_restore_state();
    void toggle_pin_to_desktop();
    void set_is_pinned(bool is_pinned);
    [[nodiscard]] bool is_focused() const;
    [[nodiscard]] WorkspaceContent* get_workspace() const;
    [[nodiscard]] uint32_t get_animation_handle() const;
    void set_animation_handle(uint32_t);
    [[nodiscard]] OutputContent* get_output() const;
    [[nodiscard]] glm::mat4 const& get_transform() const { return transform; }
    void set_transform(glm::mat4 const& in) { transform = in; }
    [[nodiscard]] glm::mat4 get_workspace_transform() const;
    [[nodiscard]] glm::mat4 get_output_transform() const;

private:
    WindowType type;
    miral::Window window;
    WorkspaceContent* workspace;
    std::shared_ptr<LeafContainer> tiling_node;
    std::optional<MirWindowState> restore_state;
    bool is_pinned = false;
    uint32_t animation_handle = 0;
    glm::mat4 transform = glm::mat4(1.f);
};

}

#endif // MIRACLEWM_WINDOW_METADATA_H
