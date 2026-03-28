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

#ifndef DYING_SURFACE_MANAGER_H
#define DYING_SURFACE_MANAGER_H

#include <memory>

namespace mir
{
namespace shell
{
    class SurfaceStack;
}
}

namespace miracle
{
class CompositorState;
class WindowContainer;
class WindowController;
class Config;
class Animator;
class PluginManager;

/// When a [Container] is removed, we notify this service which
/// manages any visual interaction on the container as it closes.
/// This most likely means some sort of closing animation.
class DyingSurfaceManager
{
public:
    DyingSurfaceManager(
        std::shared_ptr<mir::shell::SurfaceStack> const& surface_stack,
        std::shared_ptr<CompositorState> const& compositor_state,
        std::shared_ptr<Config> const& config,
        std::shared_ptr<Animator> const& animator,
        std::shared_ptr<PluginManager> const& plugin_manager,
        std::shared_ptr<WindowController> const& window_controller);

    void animate_dying_surface(std::shared_ptr<WindowContainer> const& container);

private:
    std::shared_ptr<mir::shell::SurfaceStack> surface_stack;
    std::shared_ptr<CompositorState> compositor_state;
    std::shared_ptr<Config> config;
    std::shared_ptr<Animator> animator;
    std::shared_ptr<PluginManager> plugin_manager;
    std::shared_ptr<WindowController> window_controller;
};

} // miracle

#endif // DYING_SURFACE_MANAGER_H
