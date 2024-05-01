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

#include <memory>
#include <miral/window.h>
#include <miral/window_manager_tools.h>

namespace miracle
{
class OutputContent;
class LeafNode;
class TilingWindowTree;

enum class WindowType
{
    none,
    tiled,
    floating,
    other
};

/// Applied to WindowInfo to enable
class WindowMetadata
{
public:
    WindowMetadata(WindowType type, miral::Window const& window, OutputContent* output);
    void associate_to_node(std::shared_ptr<LeafNode> const&);
    miral::Window& get_window() { return window; }
    std::shared_ptr<LeafNode> get_tiling_node() const
    {
        if (type == WindowType::tiled)
            return tiling_node;
        return nullptr;
    }
    WindowType get_type() const { return type; }
    OutputContent* get_output() const { return output; }
    bool get_is_pinned() const { return is_pinned; }
    void set_restore_state(MirWindowState state);
    MirWindowState consume_restore_state();
    void toggle_pin_to_desktop();

private:
    WindowType type;
    miral::Window window;
    OutputContent* output;
    std::shared_ptr<LeafNode> tiling_node;
    MirWindowState restore_state;
    bool is_pinned = false;
};

}

#endif // MIRACLEWM_WINDOW_METADATA_H
