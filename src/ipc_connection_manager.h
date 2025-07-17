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

#include "config_observer.h"
#include "ipc_message_handler.h"
#include "mode_observer.h"
#include "output_listener.h"
#include "window_observer.h"
#include "workspace_observer.h"
#include <mir/fd.h>
#include <mir/main_loop.h>
#include <vector>

namespace miracle
{
class AbstractCommandController;
class BindingEvent;

/// Manages IPC connections and routes requests to the [IpcMessageHandler].
class IpcConnectionManager : public virtual WorkspaceObserver,
                             public virtual ModeObserver,
                             public virtual WindowObserver,
                             public virtual ConfigObserver,
                             public virtual OutputListener
{
public:
    IpcConnectionManager(
        std::shared_ptr<mir::MainLoop> const& main_loop,
        std::shared_ptr<AbstractCommandController> const&,
        std::unique_ptr<IpcCommandExecutor>,
        std::shared_ptr<Config> const&);
    ~IpcConnectionManager() override;
    void on_workspace_created(uint32_t id) override;
    void on_workspace_empty(uint32_t id) override;
    void on_workspace_removed(uint32_t id) override;
    void on_workspace_focused(std::optional<uint32_t>, uint32_t) override;
    void on_workspace_renamed(uint32_t) override;
    void on_config_changed(Config const&) override;
    void on_mode_changed(WindowManagerMode mode) override;
    void on_shutdown();
    void on_window_created(Container const&) override;
    void on_window_closed(Container const&) override;
    void on_window_focused(Container const&) override;
    void on_window_fullscreen(Container const&) override;
    void on_window_move(Container const&) override;
    void on_window_float(Container const&) override;
    void on_window_marked(Container const&);
    void output_created(miral::Output const&) override;
    void output_deleted(miral::Output const&) override;
    void output_updated(miral::Output const& updated, miral::Output const& original) override;
    void on_binding_event(BindingEvent const& binding_event);

private:
    struct IpcClient
    {
        mir::Fd client_fd;
        uint32_t pending_read_length = 0;
        IpcType pending_type;
        std::vector<char> buffer;
        uint32_t write_buffer_len = 0;
        int subscribed_events = 0;
    };

    std::shared_ptr<mir::MainLoop> main_loop;
    std::mutex clients_mutex;
    std::shared_ptr<AbstractCommandController> command_controller;
    std::unique_ptr<IpcMessageHandler> ipc_message_handler;
    mir::Fd ipc_socket;
    sockaddr_un* ipc_sockaddr = nullptr;
    std::vector<std::shared_ptr<IpcClient>> clients;

    /// Disconnects the provided client.
    void disconnect(IpcClient& client);

    void disconnect_internal(IpcClient* client);

    /// Handles a command for the client.
    void handle_command(IpcClient& client, uint32_t payload_length, IpcType payload_type);

    /// Sends a reply to the client.
    void send_reply(IpcClient& client, IpcType command_type, std::string const& payload);

    /// Handles writeable for the client.
    void handle_writeable(IpcClient& client);

    void send_window_event(const char* event, Container const&);
    void send_output_event();
};
}

#endif // IPC_CONNECTION_H
