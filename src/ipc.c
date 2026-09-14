#include "ipc.h"
#include "event.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dispatch/dispatch.h>

static int g_socket = -1;
static bool g_running = false;

static void handle_client(int client_fd) {
    char buffer[8192];
    ssize_t n = read(client_fd, buffer, sizeof(buffer) - 1);
    if (n <= 0) {
        close(client_fd);
        return;
    }
    buffer[n] = '\0';

    struct event *event = malloc(sizeof(*event));
    event->type = EVENT_DAEMON_MESSAGE;
    event->data = strdup(buffer);
    event_post(event);

    close(client_fd);
}

void ipc_begin(void) {
    if (g_running) return;

    struct sockaddr_un addr;
    g_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (g_socket < 0) return;

    char path[256];
    snprintf(path, sizeof(path), "/tmp/omabar_%s.socket", getenv("USER"));
    unlink(path);

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", path);

    if (bind(g_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(g_socket);
        g_socket = -1;
        return;
    }

    listen(g_socket, 5);
    g_running = true;

    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        while (g_running) {
            int client = accept(g_socket, NULL, NULL);
            if (client < 0) continue;
            handle_client(client);
        }
    });
}

void ipc_end(void) {
    g_running = false;
    if (g_socket >= 0) {
        close(g_socket);
        g_socket = -1;
    }
    char path[256];
    snprintf(path, sizeof(path), "/tmp/omabar_%s.socket", getenv("USER"));
    unlink(path);
}

bool ipc_is_running(void) {
    return g_running;
}