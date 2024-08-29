#include "ipc_client.h"
#include <cstdlib>
#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

static const char ipc_magic[] = { 'i', '3', '-', 'i', 'p', 'c' };

#define IPC_HEADER_SIZE (sizeof(ipc_magic) + 8)

char* get_socketpath(void)
{
    const char* swaysock = getenv("SWAYSOCK");
    if (swaysock)
    {
        return strdup(swaysock);
    }
    char* line = NULL;
    size_t line_size = 0;
    FILE* fp = popen("sway --get-socketpath 2>/dev/null", "r");
    if (fp)
    {
        ssize_t nret = getline(&line, &line_size, fp);
        pclose(fp);
        if (nret > 0)
        {
            // remove trailing newline, if there is one
            if (line[nret - 1] == '\n')
            {
                line[nret - 1] = '\0';
            }
            return line;
        }
    }
    const char* i3sock = getenv("I3SOCK");
    if (i3sock)
    {
        free(line);
        return strdup(i3sock);
    }
    fp = popen("i3 --get-socketpath 2>/dev/null", "r");
    if (fp)
    {
        ssize_t nret = getline(&line, &line_size, fp);
        pclose(fp);
        if (nret > 0)
        {
            // remove trailing newline, if there is one
            if (line[nret - 1] == '\n')
            {
                line[nret - 1] = '\0';
            }
            return line;
        }
    }
    free(line);
    return NULL;
}

int ipc_open_socket(const char* socket_path)
{
    struct sockaddr_un addr;
    int socketfd;
    if ((socketfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        std::cerr << "Unable to open Unix socket" << std::endl;
        std::abort();
    }
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
    addr.sun_path[sizeof(addr.sun_path) - 1] = 0;
    int l = sizeof(struct sockaddr_un);
    if (connect(socketfd, (struct sockaddr*)&addr, l) == -1)
    {
        std::cout << "Unable to connect to " << socket_path << std::endl;
    }
    return socketfd;
}

bool ipc_set_recv_timeout(int socketfd, struct timeval tv)
{
    if (setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1)
    {
        std::cerr << "Failed to set ipc recv timeout" << std::endl;
        return false;
    }
    return true;
}

struct ipc_response* ipc_recv_response(int socketfd)
{
    char data[IPC_HEADER_SIZE];

    size_t total = 0;
    while (total < IPC_HEADER_SIZE)
    {
        ssize_t received = recv(socketfd, data + total, IPC_HEADER_SIZE - total, 0);
        if (received <= 0)
        {
            std::cerr << "Unable to receive IPC response" << std::endl;
            std::abort();
        }
        total += received;
    }

    ipc_response* response = static_cast<ipc_response*>(malloc(sizeof(struct ipc_response)));
    if (!response)
    {
        std::cerr << "Unable to allocate memory for IPC response" << std::endl;
        return NULL;
    }

    memcpy(&response->size, data + sizeof(ipc_magic), sizeof(uint32_t));
    memcpy(&response->type, data + sizeof(ipc_magic) + sizeof(uint32_t), sizeof(uint32_t));

    char* payload = (char*)malloc(response->size + 1);
    if (!payload)
    {
        goto error_2;
    }

    total = 0;
    while (total < response->size)
    {
        ssize_t received = recv(socketfd, payload + total, response->size - total, 0);
        if (received < 0)
        {
            std::cerr << "Unable to receive IPC response" << std::endl;
            std::abort();
        }
        total += received;
    }
    payload[response->size] = '\0';
    response->payload = payload;

    return response;
error_2:
    free(response);
error_1:
    std::cerr << "Unable to allocate memory for IPC response" << std::endl;
    return NULL;
}

void free_ipc_response(struct ipc_response* response)
{
    free(response->payload);
    free(response);
}

char* ipc_single_command(int socketfd, uint32_t type, const char* payload, uint32_t* len)
{
    char data[IPC_HEADER_SIZE];
    memcpy(data, ipc_magic, sizeof(ipc_magic));
    memcpy(data + sizeof(ipc_magic), len, sizeof(*len));
    memcpy(data + sizeof(ipc_magic) + sizeof(*len), &type, sizeof(type));

    if (write(socketfd, data, IPC_HEADER_SIZE) == -1)
    {
        std::cerr << "Unable to send IPC header" << std::endl;
        std::abort();
    }

    if (write(socketfd, payload, *len) == -1)
    {
        std::cerr << "Unable to send IPC payload" << std::endl;
        std::abort();
    }

    struct ipc_response* resp = ipc_recv_response(socketfd);
    char* response = resp->payload;
    *len = resp->size;
    free(resp);

    return response;
}
