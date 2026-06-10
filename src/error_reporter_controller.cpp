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

#define MIR_LOG_COMPONENT "ErrorReporterController"
#include "error_reporter_controller.h"
#include "config.h"
#include <mir/log.h>
#include <miral/external_client.h>
#include <signal.h>

using namespace miracle;

namespace
{
char const* const DEFAULT_ERROR_REPORTER = "miracle-wm-basic-error-reporter";
}

ErrorReporterController::ErrorReporterController(
    miral::ExternalClientLauncher& launcher,
    std::shared_ptr<Config> const& config) :
    launcher { launcher },
    config { config }
{
}

bool ErrorReporterController::is_reporter_running() const
{
    // kill(pid, 0) performs no signal delivery but succeeds only while the process
    // exists. The compositor's SIGCHLD reaper waitpid()s exited children, so a closed
    // reporter no longer satisfies this check and a fresh instance can be launched.
    return reporter_pid && kill(reporter_pid.value(), 0) == 0;
}

void ErrorReporterController::on_config_changed(Config const& changed_config)
{
    auto const setting = changed_config.get_error_reporter_client();
    if (setting == "disabled")
        return;

    if (changed_config.get_config_errors().empty())
        return;

    if (is_reporter_running())
    {
        // The running client receives the new errors via the IPC config_errors event.
        return;
    }

    std::string const command = setting == "default" ? DEFAULT_ERROR_REPORTER : setting;
    auto const split_command = miral::ExternalClientLauncher::split_command(command);
    pid_t const pid = launcher.launch(split_command);
    if (pid <= 0)
    {
        mir::log_error("Unable to launch error reporter client: %s", command.c_str());
        reporter_pid.reset();
        return;
    }

    mir::log_info("Launched error reporter client %s with pid=%d", command.c_str(), pid);
    reporter_pid = pid;
}
