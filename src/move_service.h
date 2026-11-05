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

#ifndef MIRACLE_WM_MOVE_SERVICE_H
#define MIRACLE_WM_MOVE_SERVICE_H

#include <memory>
#include <mir_toolkit/event.h>
#include <mutex>

namespace miracle
{
class AbstractCommandController;
class AbstractOutput;
class Config;
class CompositorState;
class OutputManager;
class WindowContainer;

class MoveService
{
public:
    MoveService(std::shared_ptr<AbstractCommandController> const&, std::shared_ptr<Config> const&, std::shared_ptr<OutputManager> const& output_manager);
    bool handle_pointer_event(CompositorState& state, float x, float y, MirPointerAction action, unsigned int modifiers, MirPointerButtons buttons);

    /// How far, in pixels, the cursor must travel into another output before a moved
    /// container is transferred to it. Moving back requires the same distance into the
    /// original output, which prevents the container jumping back and forth at the boundary.
    static int constexpr output_switch_buffer = 32;

private:
    void stop_move();
    std::shared_ptr<AbstractOutput> output_at(float x, float y, int inset) const;
    void try_transfer_to_output(std::shared_ptr<WindowContainer> const& container, float x, float y, int inset);

    std::shared_ptr<AbstractCommandController> command_controller;
    std::shared_ptr<Config> config;
    std::shared_ptr<OutputManager> output_manager;

    std::mutex mutex_;
    float cursor_x = 0;
    float cursor_y = 0;
    int start_x = 0;
    int start_y = 0;
    std::weak_ptr<WindowContainer> moving;
};

} // miracle

#endif // MIRACLE_WM_MOVE_SERVICE_H
