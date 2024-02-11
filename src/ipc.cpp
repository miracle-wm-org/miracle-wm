#define MIR_LOG_COMPONENT "miracle_ipc"

#include "ipc.h"
#include <linux/input-event-codes.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/un.h>
#include <unistd.h>
#include <mir/log.h>

using namespace miracle;

static const char ipc_magic[] = {'i', '3', '-', 'i', 'p', 'c'};

#define IPC_HEADER_SIZE (sizeof(ipc_magic) + 8)

namespace
{

struct sockaddr_un *ipc_user_sockaddr() {
    auto ipc_sockaddr = (sockaddr_un*)malloc(sizeof(struct sockaddr_un));
    if (ipc_sockaddr == nullptr)
    {
        mir::log_error("Can't allocate ipc_sockaddr");
        exit(1);
    }

    ipc_sockaddr->sun_family = AF_UNIX;
    int path_size = sizeof(ipc_sockaddr->sun_path);

    // Env var typically set by logind, e.g. "/run/user/<user-id>"
    const char *dir = getenv("XDG_RUNTIME_DIR");
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
}

Ipc::Ipc(miral::MirRunner& runner)
{
    auto ipc_socket_raw = socket(AF_UNIX, SOCK_STREAM, 0);
    if (ipc_socket_raw == -1)
    {
        mir::log_error("Unable to create ipc socket");
        exit(1);
    }

    if (fcntl(ipc_socket_raw, F_SETFD, FD_CLOEXEC) == -1) {
        mir::log_error("Unable to set CLOEXEC on IPC socket");
        exit(1);
    }
    if (fcntl(ipc_socket_raw, F_SETFL, O_NONBLOCK) == -1) {
        mir::log_error("Unable to set NONBLOCK on IPC socket");
        exit(1);
    }

    ipc_sockaddr = ipc_user_sockaddr();
    if (getenv("SWAYSOCK") != nullptr && access(getenv("SWAYSOCK"), F_OK) == -1)
    {
        strncpy(ipc_sockaddr->sun_path, getenv("SWAYSOCK"), sizeof(ipc_sockaddr->sun_path) - 1);
        ipc_sockaddr->sun_path[sizeof(ipc_sockaddr->sun_path) - 1] = 0;
    }

    unlink(ipc_sockaddr->sun_path);
    if (bind(ipc_socket_raw, (struct sockaddr *)ipc_sockaddr, sizeof(*ipc_sockaddr)) == -1)
    {
         mir::log_error("Unable to bind IPC socket");
         exit(1);
    }

    if (listen(ipc_socket_raw, 3) == -1) {
        mir::log_error("Unable to listen on IPC socket");
        exit(1);
    }

    // Set i3 IPC socket path so that i3-msg works out of the box
    setenv("I3SOCK", ipc_sockaddr->sun_path, 1);
    setenv("SWAYSOCK", ipc_sockaddr->sun_path, 1);

    ipc_socket = mir::Fd{ipc_socket_raw};
    socket_handle = runner.register_fd_handler(ipc_socket, [&](int fd)
    {
        int client_fd = accept(ipc_socket, NULL, NULL);
        if (client_fd == -1) {
            mir::log_error("Unable to accept IPC client connection");
            return;
        }

        int flags;
        if ((flags = fcntl(client_fd, F_GETFD)) == -1
            || fcntl(client_fd, F_SETFD, flags|FD_CLOEXEC) == -1) {
            mir::log_error("Unable to set CLOEXEC on IPC client socket");
            close(client_fd);
            return;
        }
        if ((flags = fcntl(client_fd, F_GETFL)) == -1
            || fcntl(client_fd, F_SETFL, flags|O_NONBLOCK) == -1) {
            mir::log_error("Unable to set NONBLOCK on IPC client socket");
            close(client_fd);
            return;
        }

        auto mir_fd = mir::Fd{client_fd};
        clients.push_back({
            mir_fd,
            runner.register_fd_handler(mir_fd, [this](int fd)
            {
                auto& client = get_client(fd);
                // TODO: Masking?
//                if (mask & WL_EVENT_ERROR) {
//                    sway_log(SWAY_ERROR, "IPC Client socket error, removing client");
//                    ipc_client_disconnect(client);
//                    return 0;
//                }
//
//                if (mask & WL_EVENT_HANGUP) {
//                    ipc_client_disconnect(client);
//                    return 0;
//                }

                int read_available;
                if (ioctl(client.client_fd, FIONREAD, &read_available) == -1) {
                    mir::log_error("Unable to read IPC socket buffer size");
                    disconnect(client);
                    return;
                }

                if (client.pending_length > 0) {
                    if ((uint32_t)read_available >= client.pending_length) {
                        // Reset pending values.
                        uint32_t pending_length = client.pending_length;
                        IpcCommandType pending_type = client.pending_type;
                        client.pending_length = 0;
                        handle_command(client, pending_length, pending_type);
                    }
                    return;
                }

                if (read_available < (int) IPC_HEADER_SIZE) {
                    return;
                }

                uint8_t buf[IPC_HEADER_SIZE];
                // Should be fully available, because read_available >= IPC_HEADER_SIZE
                ssize_t received = recv(client.client_fd, buf, IPC_HEADER_SIZE, 0);
                if (received == -1)
                {
                    mir::log_error("Unable to receive header from IPC client");
                    disconnect(client);
                    return;
                }

                if (memcmp(buf, ipc_magic, sizeof(ipc_magic)) != 0) {
                    mir::log_error("IPC header check failed");
                    disconnect(client);
                    return;
                }

                memcpy(&client.pending_length, buf + sizeof(ipc_magic), sizeof(uint32_t));
                memcpy(&client.pending_type, buf + sizeof(ipc_magic) + sizeof(uint32_t), sizeof(uint32_t));

                if (read_available - received >= (long)client.pending_length)
                {
                    // Reset pending values.
                    uint32_t pending_length = client.pending_length;
                    IpcCommandType pending_type = client.pending_type;
                    client.pending_length = 0;
                    handle_command(client, pending_length, pending_type);
                }
            })
        });

    });
}

Ipc::IpcClient &Ipc::get_client(int fd)
{
    for (auto& client : clients)
    {
        if (client.client_fd == fd)
            return client;
    }

    throw std::runtime_error("Could not find IPC client");
}

void Ipc::disconnect(Ipc::IpcClient& client)
{
    auto it = std::find_if(clients.begin(), clients.end(), [&](IpcClient const& other)
    {
        return other.client_fd.operator int() == client.client_fd.operator int();
    });
    if (it != clients.end())
    {
        shutdown(client.client_fd, SHUT_RDWR);
        mir::log_info("Disconnected client: %d", (int)client.client_fd);
        clients.erase(it);
    }
    else
    {
        mir::log_error("Unable to disconnect client");
    }
}

void Ipc::handle_command(miracle::Ipc::IpcClient &client, uint32_t payload_length, miracle::IpcCommandType payload_type)
{
    char *buf = (char*)malloc(payload_length + 1);
    if (!buf)
    {
        mir::log_error("Unable to allocate IPC payload");
        disconnect(client);
        return;
    }

    if (payload_length > 0) {
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

    switch (payload_type)
    {
    default:
        mir::log_warning("Unknown payload type: %d", payload_type);
        return;

    }
}