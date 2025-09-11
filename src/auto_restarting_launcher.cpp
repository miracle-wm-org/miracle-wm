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
#include <miral/runner.h>
#include <sys/wait.h>

using namespace miracle;

namespace
{
std::string expand_tilde_getenv(const std::string& path) {
    if (path.empty() || path[0] != '~') {
        return path;
    }

    const char* home_dir = std::getenv("HOME");
    if (home_dir == nullptr) {
        return path;
    }

    std::string expanded_path = home_dir;
    if (path.length() > 1 && path[1] == '/') {
        expanded_path += path.substr(1);
    } else if (path.length() > 1 && path[1] != '/') {
        return path;
    }
    return expanded_path;
}
}

AutoRestartingLauncher::AutoRestartingLauncher(
    mir::Server& server,
    miral::ExternalClientLauncher& launcher) :
    main_loop { server.the_main_loop() },
    launcher { launcher }
{
    server.add_init_callback([&]
    {
        main_loop->register_signal_handler({ SIGCHLD }, [this](int)
        { reap(); });
    });
}

std::vector<std::string_view> split(std::string_view str, char delim)
{
    std::vector<std::string_view> result;
    auto left = str.begin();
    for (auto it = left; it != str.end(); ++it)
    {
        if (*it == delim)
        {
            result.emplace_back(&*left, it - left);
            left = it + 1;
        }
    }
    if (left != str.end())
        result.emplace_back(&*left, str.end() - left);
    return result;
}

void AutoRestartingLauncher::launch(miracle::StartupApp const& cmd)
{
    std::lock_guard lock { mutex };
    pid_t pid;
    if (cmd.in_systemd_scope)
    {
        std::vector<std::string> result = { "systemd-run", "--user" };
        if (cmd.restart_on_death)
        {
            result.push_back("--property");
            result.push_back("Restart=on-failure");
        }

        auto const split_command = miral::ExternalClientLauncher::split_command(cmd.command);
        for (auto const& part : split_command)
            result.push_back(part);

        pid = launcher.launch(result);
    }
    else
    {
        auto split_command = miral::ExternalClientLauncher::split_command(cmd.command);
        split_command[0] = expand_tilde_getenv(split_command[0]);
        pid = launcher.launch(split_command);
    }

    if (pid <= 0)
    {
        mir::log_error("Unable to start external client: %s\n", cmd.command.c_str());
        return;
    }
    mir::log_info("Started external client %s with pid=%d", cmd.command.c_str(), pid);

    if (cmd.restart_on_death || cmd.should_halt_compositor_on_death)
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

            if (cmd.should_halt_compositor_on_death)
            {
                main_loop->stop();
                return;
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
