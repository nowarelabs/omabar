#include "ipc_client.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int ipc_client_connect(struct ipc_client *c) {
    if (!c) return -1;

    c->fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (c->fd < 0) return -1;

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;

    const char *user = getenv("USER");
    if (!user) user = "unknown";
    snprintf(addr.sun_path, sizeof(addr.sun_path), "/tmp/omabar_%s.socket", user);

    if (connect(c->fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(c->fd);
        c->fd = -1;
        c->connected = false;
        return -1;
    }

    c->connected = true;
    return 0;
}

void ipc_client_disconnect(struct ipc_client *c) {
    if (!c) return;
    if (c->fd >= 0) close(c->fd);
    c->fd = -1;
    c->connected = false;
}

static int read_full(int fd, void *buf, size_t len) {
    char *p = buf;
    size_t done = 0;
    while (done < len) {
        ssize_t n = read(fd, p + done, len - done);
        if (n <= 0) return -1;
        done += (size_t)n;
    }
    return 0;
}

int ipc_client_send(struct ipc_client *c, const char *json) {
    if (!c || !c->connected || !json) return -1;

    size_t len = strlen(json);
    if (len > IPC_CLIENT_MAX_MESSAGE) return -1;

    uint32_t raw_len = htonl((uint32_t)len);
    if (write(c->fd, &raw_len, sizeof(raw_len)) != (ssize_t)sizeof(raw_len))
        return -1;
    if (write(c->fd, json, len) != (ssize_t)len) return -1;
    return 0;
}

char *ipc_client_request(struct ipc_client *c, const char *json) {
    if (ipc_client_send(c, json) < 0) return NULL;

    uint32_t raw_len = 0;
    if (read_full(c->fd, &raw_len, sizeof(raw_len)) < 0) return NULL;

    uint32_t len = ntohl(raw_len);
    if (len == 0 || len > IPC_CLIENT_MAX_MESSAGE) return NULL;

    char *buf = malloc(len + 1);
    if (!buf) return NULL;
    if (read_full(c->fd, buf, len) < 0) {
        free(buf);
        return NULL;
    }
    buf[len] = '\0';
    return buf;
}