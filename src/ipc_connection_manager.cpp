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

#define MIR_LOG_COMPONENT "ipc_connection_manager"

#include "ipc_connection_manager.h"
#include "command_controller.h"
#include "config.h"
#include "ipc_command_executor.h"

#include <fcntl.h>
#include <mir/log.h>
#include <nlohmann/json.hpp>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

using json = nlohmann::json;
using namespace miracle;

static constexpr char ipc_magic[] = { 'i', '3', '-', 'i', 'p', 'c' };

#define IPC_HEADER_SIZE (sizeof(ipc_magic) + 8)

namespace
{
struct sockaddr_un* ipc_user_sockaddr()
{
    auto const ipc_sockaddr = static_cast<sockaddr_un*>(malloc(sizeof(struct sockaddr_un)));
    if (ipc_sockaddr == nullptr)
    {
        mir::log_error("Can't allocate ipc_sockaddr");
        exit(1);
    }

    ipc_sockaddr->sun_family = AF_UNIX;
    int constexpr path_size = sizeof(ipc_sockaddr->sun_path);

    // Env var typically set by logind, e.g. "/run/user/<user-id>"
    const char* dir = getenv("XDG_RUNTIME_DIR");
    if (!dir)
        dir = "/tmp";

    if (path_size <= snprintf(ipc_sockaddr->sun_path, path_size,
            "%s/miracle-wm-ipc.%u.%i.sock", dir, getuid(), getpid()))
    {
        mir::log_error("Socket path won't fit into ipc_sockaddr->sun_path");
        exit(1);
    }

    return ipc_sockaddr;
}

bool fd_is_valid(int fd)
{
    return fcntl(fd, F_GETFD) != -1 || errno != EBADF;
}

json mode_event_to_json(WindowManagerMode mode)
{
    return {
        { "change",       BINDING_MODE_STRINGS[static_cast<int>(mode)] },
        { "pango_markup", true                                         }
    };
}
}

IpcConnectionManager::IpcConnectionManager(
    miral::MirRunner& runner,
    std::shared_ptr<AbstractCommandController> const& command_controller,
    std::unique_ptr<IpcCommandExecutor> command_executor,
    std::shared_ptr<Config> const& config) :
    command_controller(command_controller),
    ipc_message_handler(std::make_unique<IpcMessageHandler>(command_controller, std::move(command_executor), config))
{
    auto const ipc_socket_raw = socket(AF_UNIX, SOCK_STREAM, 0);
    if (ipc_socket_raw == -1)
    {
        mir::log_error("Unable to create ipc socket");
        exit(1);
    }

    if (fcntl(ipc_socket_raw, F_SETFD, FD_CLOEXEC) == -1)
    {
        mir::log_error("Unable to set CLOEXEC on IPC socket");
        exit(1);
    }
    if (fcntl(ipc_socket_raw, F_SETFL, O_NONBLOCK) == -1)
    {
        mir::log_error("Unable to set NONBLOCK on IPC socket");
        exit(1);
    }

    ipc_sockaddr = ipc_user_sockaddr();
    if (getenv("MIRACLESOCK") != nullptr && access(getenv("MIRACLESOCK"), F_OK) == -1)
    {
        strncpy(ipc_sockaddr->sun_path, getenv("MIRACLESOCK"), sizeof(ipc_sockaddr->sun_path) - 1);
        ipc_sockaddr->sun_path[sizeof(ipc_sockaddr->sun_path) - 1] = 0;
    }

    unlink(ipc_sockaddr->sun_path);
    if (bind(ipc_socket_raw, (struct sockaddr*)ipc_sockaddr, sizeof(*ipc_sockaddr)) == -1)
    {
        mir::log_error("Unable to bind IPC socket");
        exit(1);
    }

    if (listen(ipc_socket_raw, 3) == -1)
    {
        mir::log_error("Unable to listen on IPC socket");
        exit(1);
    }

    setenv("I3SOCK", ipc_sockaddr->sun_path, 1);
    setenv("SWAYSOCK", ipc_sockaddr->sun_path, 1);
    setenv("MIRACLESOCK", ipc_sockaddr->sun_path, 1);

    mir::log_info("Listening to IPC socket on path: %s", ipc_sockaddr->sun_path);

    ipc_socket = mir::Fd { ipc_socket_raw };
    socket_handle = runner.register_fd_handler(ipc_socket, [&](int fd)
    {
        int client_fd = accept(ipc_socket, NULL, NULL);
        if (client_fd == -1)
        {
            mir::log_error("Unable to accept IPC client connection");
            return;
        }

        int flags;
        if ((flags = fcntl(client_fd, F_GETFD)) == -1
            || fcntl(client_fd, F_SETFD, flags | FD_CLOEXEC) == -1)
        {
            mir::log_error("Unable to set CLOEXEC on IPC client socket");
            close(client_fd);
            return;
        }
        if ((flags = fcntl(client_fd, F_GETFL)) == -1
            || fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) == -1)
        {
            mir::log_error("Unable to set NONBLOCK on IPC client socket");
            close(client_fd);
            return;
        }

        auto const mir_fd = mir::Fd { client_fd };
        clients.push_back({ mir_fd,
            runner.register_fd_handler(mir_fd, [this](int const fd)
        {
            auto& client = get_client(fd);

            uint32_t read_available;
            if (ioctl(client.client_fd, FIONREAD, &read_available) == -1)
            {
                mir::log_error("Unable to read IPC socket buffer size");
                disconnect(client);
                return;
            }

            if (client.pending_read_length > 0)
            {
                if (read_available >= client.pending_read_length)
                {
                    // Reset pending values.
                    uint32_t const pending_length = client.pending_read_length;
                    IpcType const pending_type = client.pending_type;
                    client.pending_read_length = 0;
                    handle_command(client, pending_length, pending_type);
                }
                return;
            }

            if (read_available < IPC_HEADER_SIZE)
            {
                return;
            }

            uint8_t buf[IPC_HEADER_SIZE];
            // Should be fully available, because read_available >= IPC_HEADER_SIZE
            ssize_t const received = recv(client.client_fd, buf, IPC_HEADER_SIZE, 0);
            if (received == -1)
            {
                mir::log_error("Unable to receive header from IPC client");
                disconnect(client);
                return;
            }

            if (memcmp(buf, ipc_magic, sizeof(ipc_magic)) != 0)
            {
                mir::log_error("IPC header check failed");
                disconnect(client);
                return;
            }

            memcpy(&client.pending_read_length, buf + sizeof(ipc_magic), sizeof(uint32_t));
            memcpy(&client.pending_type, buf + sizeof(ipc_magic) + sizeof(uint32_t), sizeof(uint32_t));
            mir::log_debug("Received request from IPC client: %d", (int)client.pending_type);

            if (read_available - received >= static_cast<long>(client.pending_read_length))
            {
                // Reset pending values.
                uint32_t const pending_length = client.pending_read_length;
                IpcType const pending_type = client.pending_type;
                client.pending_read_length = 0;
                handle_command(client, pending_length, pending_type);
            }
        }) });
    });
}

void IpcConnectionManager::on_created(uint32_t id)
{
    json j = {
        { "change",  "init"                                    },
        { "old",     nullptr                                   },
        { "current", command_controller->workspace_to_json(id) }
    };

    auto const serialized_value = to_string(j);
    for (auto& client : clients)
    {
        if ((client.subscribed_events & event_mask(IpcType::IPC_EVENT_WORKSPACE)) == 0)
        {
            continue;
        }

        send_reply(client, IpcType::IPC_EVENT_WORKSPACE, serialized_value);
    }
}

void IpcConnectionManager::on_removed(uint32_t id)
{
    json j = {
        { "change",  "empty"                                   },
        { "current", command_controller->workspace_to_json(id) }
    };

    auto const serialized_value = to_string(j);
    for (auto& client : clients)
    {
        if ((client.subscribed_events & event_mask(IpcType::IPC_EVENT_WORKSPACE)) == 0)
        {
            continue;
        }

        send_reply(client, IpcType::IPC_EVENT_WORKSPACE, serialized_value);
    }
}

void IpcConnectionManager::on_focused(
    std::optional<uint32_t> previous_id,
    uint32_t current_id)
{
    json j = {
        { "change",  "focus"                                           },
        { "current", command_controller->workspace_to_json(current_id) }
    };

    if (previous_id)
        j["old"] = command_controller->workspace_to_json(previous_id.value());
    else
        j["old"] = nullptr;

    auto const serialized_value = to_string(j);
    for (auto& client : clients)
    {
        if ((client.subscribed_events & event_mask(IpcType::IPC_EVENT_WORKSPACE)) == 0)
        {
            continue;
        }

        send_reply(client, IpcType::IPC_EVENT_WORKSPACE, serialized_value);
    }
}

void IpcConnectionManager::on_changed(WindowManagerMode mode)
{
    auto const response = to_string(mode_event_to_json(mode));
    for (auto& client : clients)
    {
        if ((client.subscribed_events & event_mask(IpcType::IPC_EVENT_MODE)) == 0)
        {
            continue;
        }

        send_reply(client, IpcType::IPC_EVENT_MODE, response);
    }
}

void IpcConnectionManager::on_shutdown()
{
    auto const response = to_string(json({
        { "change", "exit" }
    }));
    for (auto& client : clients)
    {
        if ((client.subscribed_events & event_mask(IpcType::IPC_EVENT_SHUTDOWN)) == 0)
        {
            continue;
        }

        send_reply(client, IpcType::IPC_EVENT_SHUTDOWN, response);
    }

    for (auto& client : clients)
        disconnect(client);
}

IpcConnectionManager::IpcClient& IpcConnectionManager::get_client(int fd)
{
    for (auto& client : clients)
    {
        if (client.client_fd == fd)
            return client;
    }

    throw std::runtime_error("Could not find IPC client");
}

void IpcConnectionManager::disconnect(IpcClient& client)
{
    auto const it = std::ranges::find_if(clients, [&](IpcClient const& other)
    {
        return other.client_fd.operator int() == client.client_fd.operator int();
    });
    if (it != clients.end())
    {
        if (fd_is_valid(client.client_fd))
            shutdown(client.client_fd, SHUT_RDWR);
        mir::log_info("Disconnected client: %d", (int)client.client_fd);
        clients.erase(it);
    }
    else
    {
        mir::log_error("Unable to disconnect client");
    }
}

void IpcConnectionManager::handle_command(IpcClient& client, uint32_t payload_length, IpcType payload_type)
{
    auto const buf = static_cast<char*>(malloc(payload_length + 1));
    if (!buf)
    {
        mir::log_error("Unable to allocate IPC payload");
        disconnect(client);
        return;
    }

    if (payload_length > 0)
    {
        // Payload should be fully available
        ssize_t received = recv(client.client_fd, buf, payload_length, 0);
        if (received == -1)
        {
            mir::log_error("Unable to receive payload from IPC client");
            disconnect(client);
            free(buf);
            return;
        }
    }
    buf[payload_length] = '\0';
    auto const result = ipc_message_handler->handle_msg(payload_type, buf, payload_length);
    if (result.fatal)
        disconnect(client);
    else
        send_reply(client, result.type, result.payload);

    client.subscribed_events |= result.subscribed_events;
    if (result.subscribed_events & event_mask(IpcType::IPC_EVENT_TICK))
    {
        json const response = {
            { "first",   true },
            { "payload", ""   }
        };
        send_reply(client, IpcType::IPC_EVENT_TICK, to_string(response));
    }

    if (result.send_tick_event)
    {
        for (auto& other_client : clients)
        {
            if ((other_client.subscribed_events & event_mask(IpcType::IPC_EVENT_TICK)) == 0)
            {
                continue;
            }

            json response = {
                { "first",   false            },
                { "payload", std::string(buf) }
            };
            send_reply(other_client, IpcType::IPC_EVENT_TICK, to_string(response));
        }
    }

    free(buf);
}

void IpcConnectionManager::send_reply(IpcClient& client, IpcType command_type, const std::string& payload)
{
    if (!fd_is_valid(client.client_fd.operator int()))
    {
        mir::log_warning("Unable to send reply to client: file descriptor is invalid");
        disconnect(client);
        return;
    }

    const uint32_t payload_length = payload.size();
    char data[IPC_HEADER_SIZE];

    const auto casted_command = static_cast<uint32_t>(command_type);
    memcpy(data, ipc_magic, sizeof(ipc_magic));
    memcpy(data + sizeof(ipc_magic), &payload_length, sizeof(payload_length));
    memcpy(data + sizeof(ipc_magic) + sizeof(payload_length), &casted_command, sizeof(casted_command));

    auto new_buffer_size = client.buffer.size();
    while (client.write_buffer_len + IPC_HEADER_SIZE + payload_length >= new_buffer_size)
    {
        if (new_buffer_size == 0)
            new_buffer_size = 1;
        new_buffer_size *= 2;
    }

    size_t constexpr MAX_BUFFER_SIZE = 4e6; // 4 MB
    if (new_buffer_size > MAX_BUFFER_SIZE)
    {
        mir::log_error("Client write buffer too big (%zu), disconnecting client", client.buffer.size());
        disconnect(client);
        return;
    }

    client.buffer.resize(new_buffer_size);

    memcpy(client.buffer.data() + client.write_buffer_len, data, IPC_HEADER_SIZE);
    client.write_buffer_len += IPC_HEADER_SIZE;
    memcpy(client.buffer.data() + client.write_buffer_len, payload.c_str(), payload_length);
    client.write_buffer_len += payload_length;
    handle_writeable(client);
}

namespace
{
// https://stackoverflow.com/questions/24920748/how-to-handle-a-sigpipe-error-inside-the-object-that-generated-it
ssize_t write_nosigpipe(int fd, void* buf, size_t len)
{
    sigset_t oldset, newset;
    ssize_t result;
    siginfo_t si;
    struct timespec ts = { 0 };

    sigemptyset(&newset);
    sigaddset(&newset, SIGPIPE);
    pthread_sigmask(SIG_BLOCK, &newset, &oldset);

    result = write(fd, buf, len);

    while (sigtimedwait(&newset, &si, &ts) >= 0 || errno != EAGAIN)
        ;
    pthread_sigmask(SIG_SETMASK, &oldset, 0);

    return result;
}
}

void IpcConnectionManager::handle_writeable(miracle::IpcConnectionManager::IpcClient& client)
{
    while (client.write_buffer_len > 0)
    {
        ssize_t const written = write_nosigpipe(client.client_fd, client.buffer.data(), client.write_buffer_len);
        if (written == -1 && errno == EAGAIN)
        {
            return;
        }
        else if (written == -1)
        {
            mir::log_error("Unable to send data from queue to IPC client");
            disconnect(client);
            return;
        }

        memmove(client.buffer.data(), client.buffer.data() + written, client.write_buffer_len - written);
        client.write_buffer_len -= written;
    }

    client.write_buffer_len = 0;
}
