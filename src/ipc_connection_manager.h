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

#ifndef IPC_CONNECTION_H
#define IPC_CONNECTION_H

#include "ipc_message_handler.h"
#include "mode_observer.h"
#include "workspace_manager.h"
#include "workspace_observer.h"
#include <mir/fd.h>
#include <miral/runner.h>
#include <vector>

namespace miracle
{
class AbstractCommandController;

/// Manages IPC connections and routes requests to the [IpcMessageHandler].
class IpcConnectionManager : public virtual WorkspaceObserver, public virtual ModeObserver
{
public:
    IpcConnectionManager(
        miral::MirRunner& runner,
        std::shared_ptr<AbstractCommandController> const&,
        std::unique_ptr<IpcCommandExecutor>,
        std::shared_ptr<Config> const&);
    void on_created(uint32_t id) override;
    void on_removed(uint32_t id) override;
    void on_focused(std::optional<uint32_t>, uint32_t) override;
    void on_changed(WindowManagerMode mode) override;
    void on_shutdown();

private:
    struct IpcClient
    {
        mir::Fd client_fd;
        std::unique_ptr<miral::FdHandle> handle;
        uint32_t pending_read_length = 0;
        IpcType pending_type;
        std::vector<char> buffer;
        uint32_t write_buffer_len = 0;
        int subscribed_events = 0;
    };

    std::shared_ptr<AbstractCommandController> command_controller;
    std::unique_ptr<IpcMessageHandler> ipc_message_handler;
    mir::Fd ipc_socket;
    std::unique_ptr<miral::FdHandle> socket_handle;
    sockaddr_un* ipc_sockaddr = nullptr;
    std::vector<IpcClient> clients;

    void disconnect(IpcClient& client);
    IpcClient& get_client(int fd);

    void handle_command(IpcClient& client, uint32_t payload_length, IpcType payload_type);
    void send_reply(IpcClient& client, IpcType command_type, std::string const& payload);
    void handle_writeable(IpcClient& client);
};
}

#endif // IPC_CONNECTION_H
