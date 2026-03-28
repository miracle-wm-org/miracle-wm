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

#define MIR_LOG_COMPONENT "output_factory"

#include "output_factory.h"
#include "display_config.h"
#include "output.h"

#include <mir/log.h>

using namespace miracle;

MiralOutputFactory::MiralOutputFactory(
    std::shared_ptr<ShellApplicationManager> const& shell_application_manager,
    std::shared_ptr<CompositorState> const& state,
    std::shared_ptr<Config> const& config,
    std::shared_ptr<WindowController> const& window_controller,
    std::shared_ptr<Animator> const& animator,
    std::shared_ptr<DisplayConfig> const& display_config,
    std::shared_ptr<PluginManager> const& plugin_manager) :
    shell_application_manager { shell_application_manager },
    state { state },
    config { config },
    window_controller { window_controller },
    animator { animator },
    display_config { display_config },
    plugin_manager { plugin_manager }
{
}

std::shared_ptr<AbstractOutput> MiralOutputFactory::create(
    std::string name, int id, mir::geometry::Rectangle area)
{
    OutputConfigDetails raw_output_config;
    for (auto const& output_config : display_config->configuration())
    {
        if (output_config.name == name || output_config.card_id.as_value() == id)
            raw_output_config = output_config;
    }

    return std::make_shared<Output>(
        shell_application_manager,
        std::move(name),
        id,
        area,
        raw_output_config,
        state,
        config,
        window_controller,
        animator,
        plugin_manager);
}
