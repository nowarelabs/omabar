#ifndef OMABAR_IPC_CLIENT_H
#define OMABAR_IPC_CLIENT_H

#include <stdbool.h>
#include <stdint.h>

/* Connection to the omabar IPC server
   (/tmp/omabar_$USER.socket) using the length-prefixed JSON protocol:
   [uint32_t big-endian length][payload]. */

#define IPC_CLIENT_MAX_MESSAGE (16 * 1024 * 1024)

struct ipc_client {
    int fd;
    bool connected;
};

/* Connect to the daemon's Unix socket.  Returns 0 on success. */
int ipc_client_connect(struct ipc_client *c);
/* Close the connection. */
void ipc_client_disconnect(struct ipc_client *c);

/* Send a single length-prefixed JSON message (no response read).
   Returns 0 on success. */
int ipc_client_send(struct ipc_client *c, const char *json);

/* Send a message and read one response.  The returned string is
   caller-owned (NUL-terminated) or NULL on failure. */
char *ipc_client_request(struct ipc_client *c, const char *json);

#endif