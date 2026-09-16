#include "plugin.h"
#include "ipc.h"
#include "misc/helpers.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <errno.h>

#define PLUGIN_MAX 64

static struct plugin g_plugins[PLUGIN_MAX];
static int g_count = 0;

void plugin_init(void) {
    g_count = 0;
}

int plugin_count(void) {
    return g_count;
}

void plugin_destroy(void) {
    for (int i = 0; i < g_count; i++) {
        free(g_plugins[i].name);
        if (g_plugins[i].fd >= 0)
            close(g_plugins[i].fd);
    }
    g_count = 0;
}

static struct plugin *plugin_find_name(const char *name) {
    for (int i = 0; i < g_count; i++)
        if (g_plugins[i].name && string_equals(g_plugins[i].name, name))
            return &g_plugins[i];
    return NULL;
}

static struct plugin *plugin_find_fd(int fd) {
    for (int i = 0; i < g_count; i++)
        if (g_plugins[i].fd == fd)
            return &g_plugins[i];
    return NULL;
}

void plugin_register(const char *name, int fd) {
    if (!name) return;

    struct plugin *existing = plugin_find_name(name);
    if (existing) {
        /* re-registration on the same socket just updates state */
        if (existing->fd != fd) {
            close(existing->fd);
            existing->fd = fd;
        }
        existing->connected = true;
        existing->subscribed_events = 0;
        return;
    }

    if (g_count >= PLUGIN_MAX) return;

    struct plugin *p = &g_plugins[g_count];
    p->name = strdup(name);
    p->fd = fd;
    p->connected = true;
    p->subscribed_events = 0;
    p->last_seen = 0;
    g_count++;
}

void plugin_unregister(const char *name) {
    if (!name) return;
    for (int i = 0; i < g_count; i++) {
        if (g_plugins[i].name && string_equals(g_plugins[i].name, name)) {
            close(g_plugins[i].fd);
            free(g_plugins[i].name);
            memmove(&g_plugins[i], &g_plugins[i + 1],
                    (g_count - i - 1) * sizeof(struct plugin));
            g_count--;
            return;
        }
    }
}

void plugin_unregister_by_fd(int fd) {
    for (int i = 0; i < g_count; i++) {
        if (g_plugins[i].fd == fd) {
            free(g_plugins[i].name);
            memmove(&g_plugins[i], &g_plugins[i + 1],
                    (g_count - i - 1) * sizeof(struct plugin));
            g_count--;
            return;
        }
    }
}

void plugin_subscribe(int fd, uint64_t mask) {
    struct plugin *p = plugin_find_fd(fd);
    if (!p) {
        if (g_count >= PLUGIN_MAX) return;
        char name[64];
        snprintf(name, sizeof(name), "plugin:%d", fd);
        plugin_register(name, fd);
        p = plugin_find_fd(fd);
        if (!p) return;
    }
    p->subscribed_events |= mask;
    p->connected = true;
}

void plugin_unsubscribe(int fd, uint64_t mask) {
    struct plugin *p = plugin_find_fd(fd);
    if (!p) return;
    p->subscribed_events &= ~mask;
}

void plugin_notify(uint64_t mask, const char *name, const char *data) {
    if (!name) return;

    char buf[1024];
    if (data) {
        /* `data` is already a JSON value (object/array/string) — embed
           verbatim; escaping string data is the caller's job */
        snprintf(buf, sizeof(buf), "{\"type\":\"event\",\"name\":\"%s\",\"data\":%s}",
                 name, data);
    } else {
        snprintf(buf, sizeof(buf), "{\"type\":\"event\",\"name\":\"%s\",\"data\":null}",
                 name);
    }

    for (int i = 0; i < g_count; i++) {
        struct plugin *p = &g_plugins[i];
        if (!p->connected) continue;
        if (!(p->subscribed_events & mask)) continue;

        if (!ipc_send_json(p->fd, buf)) {
            /* connection went away mid-write -> mark for the health check */
            p->connected = false;
        }
    }
}

void plugin_health_check(void) {
    for (int i = 0; i < g_count; i++) {
        struct plugin *p = &g_plugins[i];
        if (!p->connected) {
            plugin_unregister_by_fd(p->fd);
            i--;
            continue;
        }

        /* macOS poll() ignores HUP at events=0, so probe with a
           non-blocking peek: 0 => peer closed, EAGAIN => still alive */
        char probe;
        ssize_t r = recv(p->fd, &probe, 1, MSG_PEEK | MSG_DONTWAIT);
        if (r == 0 || (r < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
            plugin_unregister_by_fd(p->fd);
            i--;
            continue;
        }
    }
}