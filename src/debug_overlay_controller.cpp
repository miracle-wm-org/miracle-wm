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

#define MIR_LOG_COMPONENT "DebugOverlayController"
#include "debug_overlay_controller.h"
#include "config.h"
#include <mir/log.h>
#include <miral/external_client.h>
#include <signal.h>

using namespace miracle;

namespace
{
char const* const DEFAULT_DEBUG_OVERLAY = "miracle-wm-debug-overlay";
}

DebugOverlayController::DebugOverlayController(
    miral::ExternalClientLauncher& launcher,
    std::shared_ptr<Config> const& config) :
    launcher { launcher },
    config { config }
{
}

bool DebugOverlayController::is_running() const
{
    // kill(pid, 0) delivers no signal but succeeds only while the process
    // exists. The compositor's SIGCHLD reaper waitpid()s exited children, so a
    // closed overlay no longer satisfies this check.
    return overlay_pid && kill(overlay_pid.value(), 0) == 0;
}

void DebugOverlayController::on_config_changed(Config const& changed_config)
{
    // Honor a live switch to "disabled" by tearing down a running overlay.
    if (changed_config.get_debug_overlay_client() == "disabled" && is_running())
        stop();
}

void DebugOverlayController::launch()
{
    auto const setting = config->get_debug_overlay_client();
    if (setting == "disabled")
    {
        mir::log_info("Debug overlay is disabled in the configuration; not launching");
        return;
    }

    std::string const command = setting == "default" ? DEFAULT_DEBUG_OVERLAY : setting;
    auto const split_command = miral::ExternalClientLauncher::split_command(command);
    pid_t const pid = launcher.launch(split_command);
    if (pid <= 0)
    {
        mir::log_error("Unable to launch debug overlay client: %s", command.c_str());
        overlay_pid.reset();
        return;
    }

    mir::log_info("Launched debug overlay client %s with pid=%d", command.c_str(), pid);
    overlay_pid = pid;
}

void DebugOverlayController::stop()
{
    if (!overlay_pid)
        return;

    kill(overlay_pid.value(), SIGTERM);
    overlay_pid.reset();
}

void DebugOverlayController::toggle()
{
    if (is_running())
        stop();
    else
        launch();
}

void DebugOverlayController::set_enabled(bool enabled)
{
    if (enabled && !is_running())
        launch();
    else if (!enabled && is_running())
        stop();
}
