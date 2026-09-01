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

#define MIR_LOG_COMPONENT "ipc_connection_manager"

#include "ipc_connection_manager.h"
#include "binding_event.h"
#include "command_controller.h"
#include "config.h"
#include "ipc_command_executor.h"
#include "window_controller.h"

#include <fcntl.h>
#include <mir/log.h>
#include <mir/main_loop.h>
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
        { "change",       BINDING_MODE_STRINGS[static_cast<size_t>(mode)] },
        { "pango_markup", true                                            }
    };
}

json config_errors_to_json(std::vector<miracle::Error> const& errors)
{
    json result = json::array();
    for (auto const& error : errors)
    {
        result.push_back({
            { "filename", error.filename                                                    },
            { "line",     error.line                                                        },
            { "column",   error.column                                                      },
            { "level",    error.level == miracle::ErrorLevel::warning ? "warning" : "error" },
            { "message",  error.message                                                     },
        });
    }
    return result;
}
}

IpcConnectionManager::IpcConnectionManager(
    std::shared_ptr<mir::MainLoop> const& main_loop_,
    std::shared_ptr<AbstractCommandController> const& command_controller,
    std::shared_ptr<IpcCommandExecutor> const& command_executor,
    std::shared_ptr<Config> const& config,
    std::shared_ptr<WindowController> const& window_controller,
    std::shared_ptr<PluginManager> const& plugin_manager) :
    main_loop(main_loop_),
    command_controller(command_controller),
    config(config),
    ipc_message_handler(std::make_unique<IpcMessageHandler>(command_controller, command_executor, config, window_controller, plugin_manager))
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
    main_loop->register_fd_handler({ ipc_socket }, this, [this](int fd)
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

        auto mir_fd = mir::Fd { client_fd };
        std::lock_guard lock(clients_mutex);
        auto client = std::make_shared<IpcClient>();
        client->client_fd = mir_fd;
        std::weak_ptr<IpcClient> weak_client = client;
        main_loop->register_fd_handler({ mir_fd }, client.get(), [weak_client = weak_client, this](int const)
        {
            auto const client = weak_client.lock();
            if (!client)
                return;

            uint32_t read_available;
            if (ioctl(client->client_fd, FIONREAD, &read_available) == -1)
            {
                mir::log_error("Unable to read IPC socket buffer size");
                disconnect(*client);
                return;
            }

            if (read_available == 0)
            {
                mir::log_warning("Peer has closed connection");
                disconnect(*client);
                return;
            }

            if (client->pending_read_length > 0)
            {
                if (read_available >= client->pending_read_length)
                {
                    // Reset pending values.
                    uint32_t const pending_length = client->pending_read_length;
                    IpcType const pending_type = client->pending_type;
                    client->pending_read_length = 0;
                    handle_command(*client, pending_length, pending_type);
                }
                return;
            }

            if (read_available < IPC_HEADER_SIZE)
            {
                return;
            }

            uint8_t buf[IPC_HEADER_SIZE];
            // Should be fully available, because read_available >= IPC_HEADER_SIZE
            ssize_t const received = recv(client->client_fd, buf, IPC_HEADER_SIZE, 0);
            if (received == -1)
            {
                mir::log_error("Unable to receive header from IPC client");
                disconnect(*client);
                return;
            }

            if (memcmp(buf, ipc_magic, sizeof(ipc_magic)) != 0)
            {
                mir::log_error("IPC header check failed");
                disconnect(*client);
                return;
            }

            memcpy(&client->pending_read_length, buf + sizeof(ipc_magic), sizeof(uint32_t));
            memcpy(&client->pending_type, buf + sizeof(ipc_magic) + sizeof(uint32_t), sizeof(uint32_t));
            mir::log_debug("Received request from IPC client: %d", static_cast<int>(client->pending_type));

            if (read_available - received >= static_cast<long>(client->pending_read_length))
            {
                // Reset pending values.
                uint32_t const pending_length = client->pending_read_length;
                IpcType const pending_type = client->pending_type;
                client->pending_read_length = 0;
                handle_command(*client, pending_length, pending_type);
            }
        });

        clients.push_back(client);
    });
}

IpcConnectionManager::~IpcConnectionManager()
{
    // Expires the weak_ptrs held by any action still sitting on the main loop queue.
    alive.reset();
    main_loop->unregister_fd_handler(this);
}

void IpcConnectionManager::run_on_main_loop(std::function<void()> action)
{
    main_loop->enqueue(this, [weak_alive = std::weak_ptr<bool>(alive), action = std::move(action)]
    {
        // We may have been destroyed between enqueueing the action and draining it.
        if (weak_alive.expired())
            return;

        action();
    });
}

std::vector<std::shared_ptr<IpcConnectionManager::IpcClient>> IpcConnectionManager::snapshot_clients()
{
    std::lock_guard lock(clients_mutex);
    return clients;
}

void IpcConnectionManager::broadcast(IpcType type, std::string payload)
{
    run_on_main_loop([this, type, payload = std::move(payload)]
    {
        for (auto const& client : snapshot_clients())
        {
            if ((client->subscribed_events & ipc_event_mask(type)) == 0)
            {
                continue;
            }

            send_reply(*client, type, payload);
        }
    });
}

void IpcConnectionManager::on_workspace_created(uint32_t id)
{
    json const j = {
        { "change",  "init"                                    },
        { "old",     nullptr                                   },
        { "current", command_controller->workspace_to_json(id) }
    };

    broadcast(IpcType::IPC_EVENT_WORKSPACE, to_string(j));
}

void IpcConnectionManager::on_workspace_empty(uint32_t id)
{
    json const j = {
        { "change",  "empty"                                   },
        { "old",     nullptr                                   },
        { "current", command_controller->workspace_to_json(id) }
    };

    broadcast(IpcType::IPC_EVENT_WORKSPACE, to_string(j));
}

void IpcConnectionManager::on_workspace_removed(uint32_t id)
{
    json const j = {
        { "change",  "empty"                                   },
        { "current", command_controller->workspace_to_json(id) }
    };

    broadcast(IpcType::IPC_EVENT_WORKSPACE, to_string(j));
}

void IpcConnectionManager::on_workspace_focused(
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

    broadcast(IpcType::IPC_EVENT_WORKSPACE, to_string(j));
}

void IpcConnectionManager::on_workspace_renamed(uint32_t id)
{
    json const j = {
        { "change",  "rename"                                  },
        { "current", command_controller->workspace_to_json(id) }
    };

    broadcast(IpcType::IPC_EVENT_WORKSPACE, to_string(j));
}

void IpcConnectionManager::on_config_changed(Config const& changed_config)
{
    json const j = {
        { "change", "reload" }
    };

    broadcast(IpcType::IPC_EVENT_WORKSPACE, to_string(j));
    broadcast(IpcType::IPC_EVENT_CONFIG_ERRORS,
        to_string(config_errors_to_json(changed_config.get_config_errors())));
}

void IpcConnectionManager::on_mode_changed(WindowManagerMode mode)
{
    broadcast(IpcType::IPC_EVENT_MODE, to_string(mode_event_to_json(mode)));
}

void IpcConnectionManager::on_shutdown()
{
    auto const response = to_string(json({
        { "change", "exit" }
    }));
    // Sent synchronously rather than via [broadcast]: the main loop is on its way down
    // and may never drain another action.
    for (auto const& client : snapshot_clients())
    {
        if ((client->subscribed_events & ipc_event_mask(IpcType::IPC_EVENT_SHUTDOWN)) == 0)
        {
            continue;
        }

        send_reply(*client, IpcType::IPC_EVENT_SHUTDOWN, response);
    }

    std::vector<std::shared_ptr<IpcClient>> remaining;
    {
        std::lock_guard lock(clients_mutex);
        remaining.swap(clients);
    }

    for (auto const& client : remaining)
        disconnect_internal(client.get());
}

void IpcConnectionManager::send_window_event(const char* event, Container const& container)
{
    auto const j = json({
        { "change",    event                    },
        { "container", container.to_json(false) }  // TODO: Handle workspace visibility
    });
    // Serialized here, on the caller's thread, because [Container::to_json] reads live
    // window management state. Only the send itself is deferred to the main loop.
    broadcast(IpcType::IPC_EVENT_WINDOW, to_string(j));
}

void IpcConnectionManager::output_created(miral::Output const&)
{
    send_output_event();
}

void IpcConnectionManager::output_deleted(miral::Output const&)
{
    send_output_event();
}

void IpcConnectionManager::output_updated(miral::Output const&, miral::Output const&)
{
    send_output_event();
}

void IpcConnectionManager::send_output_event()
{
    auto const j = json({
        { "change", "unspecified" }
    });
    broadcast(IpcType::IPC_EVENT_OUTPUT, to_string(j));
}

void IpcConnectionManager::on_binding_event(BindingEvent const& binding_event)
{
    broadcast(IpcType::IPC_EVENT_BINDING, to_string(binding_event.to_json()));
}

void IpcConnectionManager::on_plugin_event(std::string const& ns, std::string const& payload_json)
{
    json j;
    j["plugin"] = ns;
    try
    {
        j["payload"] = json::parse(payload_json);
    }
    catch (json::exception const&)
    {
        // Fall back to the raw string if the plugin published a non-JSON payload.
        j["payload"] = payload_json;
    }
    run_on_main_loop([this, ns, str = to_string(j)]
    {
        for (auto const& client : snapshot_clients())
        {
            if ((client->subscribed_events & ipc_event_mask(IpcType::IPC_EVENT_PLUGIN)) == 0)
                continue;

            auto const& namespaces = client->subscribed_plugin_namespaces;
            if (std::ranges::find(namespaces, ns) == namespaces.end())
                continue;

            send_reply(*client, IpcType::IPC_EVENT_PLUGIN, str);
        }
    });
}

void IpcConnectionManager::on_window_created(Container const& container)
{
    send_window_event("new", container);
}

void IpcConnectionManager::on_window_closed(Container const& container)
{
    send_window_event("close", container);
}

void IpcConnectionManager::on_window_focused(Container const& container)
{
    send_window_event("focus", container);
}

void IpcConnectionManager::on_window_fullscreen(Container const& container)
{
    send_window_event("fullscreen_mode", container);
}

void IpcConnectionManager::on_window_move(Container const& container)
{
    send_window_event("move", container);
}

void IpcConnectionManager::on_window_float(Container const& container)
{
    send_window_event("floating", container);
}

void IpcConnectionManager::on_window_marked(Container const& container)
{
    send_window_event("mark", container);
}

void IpcConnectionManager::on_urgency_changed(Container const& container)
{
    send_window_event("urgent", container);
}

void IpcConnectionManager::disconnect(IpcClient& client)
{
    std::shared_ptr<IpcClient> removed;

    {
        std::lock_guard lock(clients_mutex);
        auto const it = std::ranges::find_if(clients, [&](std::shared_ptr<IpcClient> const& other)
        {
            return other->client_fd.operator int() == client.client_fd.operator int();
        });
        if (it == clients.end())
        {
            mir::log_error("Unable to disconnect client");
            return;
        }

        // [clients] holds the only reference to the client, so we keep it alive until this
        // call returns: our callers still refer to it after we have erased it.
        removed = *it;
        clients.erase(it);
    }

    // Called with the lock released: it blocks until the client's fd handler is idle.
    disconnect_internal(removed.get());
}

void IpcConnectionManager::disconnect_internal(IpcClient* client)
{
    main_loop->unregister_fd_handler(client);
    if (fd_is_valid(client->client_fd))
        shutdown(client->client_fd, SHUT_WR);
    mir::log_info("Disconnected client: %d", (int)client->client_fd);
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
    {
        disconnect(client);
        free(buf);
        return;
    }

    send_reply(client, result.type, result.payload);

    client.subscribed_events |= result.subscribed_events;
    client.subscribed_plugin_namespaces.insert(
        client.subscribed_plugin_namespaces.end(),
        result.subscribed_plugin_namespaces.begin(),
        result.subscribed_plugin_namespaces.end());
    if (result.subscribed_events & ipc_event_mask(IpcType::IPC_EVENT_TICK))
    {
        json const response = {
            { "first",   true },
            { "payload", ""   }
        };
        send_reply(client, IpcType::IPC_EVENT_TICK, to_string(response));
    }

    if (result.subscribed_events & ipc_event_mask(IpcType::IPC_EVENT_CONFIG_ERRORS))
    {
        // A freshly-subscribed client immediately receives all errors from the most
        // recent configuration load.
        auto const serialized_errors = to_string(config_errors_to_json(config->get_config_errors()));
        send_reply(client, IpcType::IPC_EVENT_CONFIG_ERRORS, serialized_errors);
    }

    if (result.send_tick_event)
    {
        for (auto const& other_client : snapshot_clients())
        {
            if ((other_client->subscribed_events & ipc_event_mask(IpcType::IPC_EVENT_TICK)) == 0)
            {
                continue;
            }

            json response = {
                { "first",   false            },
                { "payload", std::string(buf) }
            };
            send_reply(*other_client, IpcType::IPC_EVENT_TICK, to_string(response));
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

    const uint32_t payload_length = static_cast<uint32_t>(payload.size());
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
    struct timespec ts = { 0, 0 };

    sigemptyset(&newset);
    sigaddset(&newset, SIGPIPE);
    pthread_sigmask(SIG_BLOCK, &newset, &oldset);

    result = write(fd, buf, len);

    // Draining SIGPIPE below overwrites errno (sigtimedwait sets EAGAIN when it times out),
    // so preserve the write's own errno for the caller.
    int const write_errno = errno;

    while (sigtimedwait(&newset, &si, &ts) >= 0 || errno != EAGAIN)
        ;
    pthread_sigmask(SIG_SETMASK, &oldset, 0);

    errno = write_errno;
    return result;
}
}

void IpcConnectionManager::handle_writeable(IpcClient& client)
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

        auto const size = static_cast<size_t>(client.write_buffer_len - written);
        memmove(client.buffer.data(), client.buffer.data() + written, size);
        client.write_buffer_len -= static_cast<uint32_t>(written);
    }

    client.write_buffer_len = 0;
}
