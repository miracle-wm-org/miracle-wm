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

using namespace miracle;

WindowMetadata::WindowMetadata(
    miracle::WindowType type,
    miral::Window const& window,
    OutputContent* output) :
    type { type },
    window { window },
    output { output }
{
}

void WindowMetadata::associate_to_node(std::shared_ptr<LeafNode> const& node)
{
    tiling_node = node;
}

void WindowMetadata::set_restore_state(MirWindowState state)
{
    restore_state = state;
}

MirWindowState WindowMetadata::consume_restore_state()
{
    return restore_state;
}

void WindowMetadata::toggle_pin_to_desktop()
{
    if (type == WindowType::floating)
    {
        is_pinned = !is_pinned;
    }
}