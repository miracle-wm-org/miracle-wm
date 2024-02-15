#define MIR_LOG_COMPONENT "AutoRestartingLauncher"
#include "auto_restarting_launcher.h"
#include <mir/log.h>
#include <sys/wait.h>

using namespace miracle;

AutoRestartingLauncher::AutoRestartingLauncher(
    miral::MirRunner& runner,
    miral::ExternalClientLauncher& launcher)
    : runner{runner},
      launcher{launcher}
{
    runner.add_start_callback([&] { runner.register_signal_handler({SIGCHLD}, [this](int) { reap(); }); });
}

void AutoRestartingLauncher::launch(miracle::StartupApp const& cmd)
{
    std::lock_guard lock{mutex};
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
                std::lock_guard lock{mutex};
                if (auto it = pid_to_command_map.find(pid); it != pid_to_command_map.end())
                {
                    cmd = it->second;
                    pid_to_command_map.erase(pid);
                }
            }

            if (cmd.restart_on_death)
                launch(cmd);
        }
        else
            break;
    }
}