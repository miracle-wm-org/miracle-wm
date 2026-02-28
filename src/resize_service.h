/**
Copyright (C) 2025  Matthew Kosarek

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

#ifndef MIRACLE_RESIZE_SERVICE_H
#define MIRACLE_RESIZE_SERVICE_H

#include "command_controller.h"
#include "compositor_state.h"
#include "config.h"
#include "output_manager.h"

namespace miracle
{

class ResizeService
{
public:
    ResizeService(
        std::shared_ptr<AbstractCommandController> const& command_controller,
        std::shared_ptr<Config> const& config,
        std::shared_ptr<CompositorState> const& state,
        std::shared_ptr<OutputManager> const& output_manager);

    bool handle_pointer_event(float x, float y, MirPointerAction action);
    void handle_request_resize(std::shared_ptr<WindowContainer> const& container, MirPointerAction action, MirResizeEdge edge);

private:
    void stop();

    std::shared_ptr<AbstractCommandController> command_controller;
    std::shared_ptr<Config> config;
    std::shared_ptr<CompositorState> state;
    std::shared_ptr<OutputManager> output_manager;

    std::weak_ptr<WindowContainer> resizing_container;
    MirResizeEdge resize_edge = mir_resize_edge_none;
    bool is_resizing = false;
};

} // namespace miracle

#endif // MIRACLE_RESIZE_SERVICE_H
