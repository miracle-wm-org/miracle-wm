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

#define MIR_LOG_COMPONENT "AutoRestartingLauncher"
#include "auto_restarting_launcher.h"
#include <mir/log.h>
#include <sys/wait.h>

using namespace miracle;

AutoRestartingLauncher::AutoRestartingLauncher(
    miral::MirRunner& runner,
    miral::ExternalClientLauncher& launcher) :
    runner { runner },
    launcher { launcher }
{
    runner.add_start_callback([&]
    { runner.register_signal_handler({ SIGCHLD }, [this](int)
      { reap(); }); });
}

void AutoRestartingLauncher::launch(miracle::StartupApp const& cmd)
{
    std::lock_guard lock { mutex };
    auto pid = launcher.launch(cmd.command);
    if (pid <= 0)
    {
        mir::log_error("Unable to start external client: %s\n", cmd.command.c_str());
        return;
    }
    mir::log_info("Started external client %s with pid=%d", cmd.command.c_str(), pid);

    if (cmd.restart_on_death)
        pid_to_command_map[pid] = cmd;
}

void AutoRestartingLauncher::kill_all()
{
    std::lock_guard lock { mutex };
    for (auto const& entry : pid_to_command_map)
    {
        if (entry.second.restart_on_death)
            kill(entry.first, SIGTERM);
    }
}

void AutoRestartingLauncher::reap()
{
    int status;
    while (true)
    {
        auto const pid = waitpid(-1, &status, WNOHANG);
        StartupApp cmd;
        if (pid > 0)
        {
            {
                std::lock_guard lock { mutex };
                if (auto it = pid_to_command_map.find(pid); it != pid_to_command_map.end())
                {
                    cmd = it->second;
                    pid_to_command_map.erase(pid);
                }
            }

            if (cmd.restart_on_death)
            {
                if (status != 127)
                {
                    mir::log_error(
                        "Process exited with status 127, meaning it could not be found. %s will not be restarted",
                        cmd.command.c_str());
                }
                else
                {
                    launch(cmd);
                }
            }
        }
        else
            break;
    }
}
