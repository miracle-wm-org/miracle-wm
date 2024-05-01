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

#ifndef MIRACLEWM_WINDOW_MANAGER_TOOLS_TILING_INTERFACE_H
#define MIRACLEWM_WINDOW_MANAGER_TOOLS_TILING_INTERFACE_H

#include "tiling_interface.h"
#include <miral/window_manager_tools.h>

namespace miracle
{
class WindowManagerToolsTilingInterface : public TilingInterface
{
public:
    explicit WindowManagerToolsTilingInterface(miral::WindowManagerTools const&);
    bool is_fullscreen(miral::Window const&) override;
    void set_rectangle(miral::Window const&, geom::Rectangle const&) override;
    MirWindowState get_state(miral::Window const&) override;
    void change_state(miral::Window const&, MirWindowState state) override;
    void clip(miral::Window const&, geom::Rectangle const&) override;
    void noclip(miral::Window const&) override;
    void select_active_window(miral::Window const&) override;
    std::shared_ptr<WindowMetadata> get_metadata(miral::Window const&) override;
    std::shared_ptr<WindowMetadata> get_metadata(miral::Window const&, TilingWindowTree const*) override;
    void raise(miral::Window const&) override;
    void send_to_back(miral::Window const&) override;

private:
    miral::WindowManagerTools tools;
};
}

#endif // MIRACLEWM_WINDOW_MANAGER_TOOLS_TILING_INTERFACE_H
