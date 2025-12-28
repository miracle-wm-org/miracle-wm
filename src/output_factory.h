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

#ifndef MIRACLE_WM_OUTPUT_FACTORY_H
#define MIRACLE_WM_OUTPUT_FACTORY_H

#include "output_factory_interface.h"

namespace mir
{
class ServerActionQueue;
}

namespace miracle
{
class WorkspaceManager;
class CompositorState;
class Config;
class WindowController;
class Animator;
class DisplayConfig;
class PluginManager;
class ShellApplicationManager;

class MiralOutputFactory : public OutputFactoryInterface
{
public:
    MiralOutputFactory(
        std::shared_ptr<ShellApplicationManager> const& shell_application_manager,
        std::shared_ptr<CompositorState> const& state,
        std::shared_ptr<Config> const& options,
        std::shared_ptr<WindowController> const&,
        std::shared_ptr<Animator> const&,
        std::shared_ptr<DisplayConfig> const& display_config,
        std::shared_ptr<mir::ServerActionQueue> const& server_action_queue,
        std::shared_ptr<PluginManager> const& plugin_manager);
    std::shared_ptr<OutputInterface> create(
        std::string name,
        int id,
        mir::geometry::Rectangle area) override;

private:
    std::shared_ptr<ShellApplicationManager> shell_application_manager;
    std::shared_ptr<CompositorState> state;
    std::shared_ptr<Config> config;
    std::shared_ptr<WindowController> window_controller;
    std::shared_ptr<Animator> animator;
    std::shared_ptr<DisplayConfig> display_config;
    std::shared_ptr<mir::ServerActionQueue> server_action_queue;
    std::shared_ptr<PluginManager> plugin_manager;
};

}

#endif // MIRACLE_WM_OUTPUT_FACTORY_H
