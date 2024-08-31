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


#ifndef _SWAY_IPC_CLIENT_H
#define _SWAY_IPC_CLIENT_H

// arbitrary number, it's probably sufficient, higher number = more memory usage
#define JSON_MAX_DEPTH 512

#include <stdbool.h>
#include <stdint.h>
#include <sys/time.h>

#include "ipc.h"

/**
 * IPC response including type of IPC response, size of payload and the json
 * encoded payload string.
 */
struct ipc_response
{
    uint32_t size;
    uint32_t type;
    char* payload;
};

/**
 * Gets the path to the IPC socket from sway.
 */
char* get_socketpath(void);
/**
 * Opens the sway socket.
 */
int ipc_open_socket(const char* socket_path);
/**
 * Issues a single IPC command and returns the buffer. len will be updated with
 * the length of the buffer returned from sway.
 */
char* ipc_single_command(int socketfd, uint32_t type, const char* payload, uint32_t* len);
/**
 * Receives a single IPC response and returns an ipc_response.
 */
struct ipc_response* ipc_recv_response(int socketfd);
/**
 * Free ipc_response struct
 */
void free_ipc_response(struct ipc_response* response);
/**
 * Sets the receive timeout for the IPC socket
 */
bool ipc_set_recv_timeout(int socketfd, struct timeval tv);

#endif
