#ifndef OMABAR_IPC_H
#define OMABAR_IPC_H

#include <stdbool.h>
#include <stdint.h>

/* ────────────────────────────────────────────────────────────────
   IPC server (Unix domain socket at /tmp/omabar_$USER.socket)
   ──────────────────────────────────────────────────────────────── */

/* Start the IPC server.  Each accepted client is handled on a dedicated
   GCD block; incoming messages use the length-prefixed JSON protocol:
   [uint32_t big-endian length][payload].  Returns true on success. */
bool socket_daemon_begin_un(void);

/* Shut down the IPC server, close the socket and unlink the path. */
void socket_daemon_end(void);

/* Whether the server is currently listening. */
bool socket_daemon_is_running(void);

/* Legacy wrappers kept for internal callers. */
static inline bool ipc_begin(void) { return socket_daemon_begin_un(); }
static inline void ipc_end(void)   { socket_daemon_end(); }
static inline bool ipc_is_running(void) { return socket_daemon_is_running(); }

/* Send a length-prefixed JSON message on `fd`.  Returns true on success. */
bool ipc_send_json(int fd, const char *json);

/* ────────────────────────────────────────────────────────────────
   Message protocol
   ────────────────────────────────────────────────────────────────
Every message is a single JSON object with a "type" field:

     {"type": "register",  "name": "plugin-name"}
     {"type": "update",    "item": "name", "icon": "...",
                           "label": "...", "background_color": "0xaarrggbb"}
     {"type": "subscribe", "events": ["volume_changed", ...]}
     {"type": "query",     "item": "name"}
     {"type": "trigger",   "event": "custom_name", "data": {...}}

   unknown/malformed messages yield type = IPC_MSG_UNKNOWN.
   ──────────────────────────────────────────────────────────────── */

enum ipc_message_type {
    IPC_MSG_UNKNOWN = 0,
    IPC_MSG_REGISTER,
    IPC_MSG_UPDATE,
    IPC_MSG_SUBSCRIBE,
    IPC_MSG_QUERY,
    IPC_MSG_TRIGGER,
};

struct ipc_message {
    enum ipc_message_type type;
    enum ipc_message_type status;

    char *name;             /* register: plugin name */
    char *item;             /* update / query */
    char *icon;             /* update */
    char *label;            /* update */
    char *background_color; /* update */
    char **events;          /* subscribe */
    int event_count;
    char *event;            /* trigger */
    char *data;             /* trigger: raw JSON object text */

    uint32_t client_fd;     /* connection the message arrived on */
    char error[256];        /* validation error when status is set */
};

/* Parse and validate a JSON message.  Returns a caller-owned struct.
   `client_fd` is attached for routing responses back. */
struct ipc_message *ipc_parse_message(const char *json, uint32_t client_fd);
void ipc_message_free(struct ipc_message *msg);

/* Build a compact JSON reply string: {"ok":true} or {"ok":false,"error":...} */
char *ipc_make_reply(bool ok, const char *error);

#endif