#include "plugin.h"
#include "misc/helpers.h"
#include <stdlib.h>
#include <unistd.h>

#define PLUGIN_MAX 64

static struct plugin g_plugins[PLUGIN_MAX];
static int g_count = 0;

void plugin_init(void) {
    g_count = 0;
}

void plugin_destroy(void) {
    for (int i = 0; i < g_count; i++) {
        free(g_plugins[i].name);
        if (g_plugins[i].fd >= 0)
            close(g_plugins[i].fd);
    }
    g_count = 0;
}

void plugin_register(const char *name, int fd) {
    if (g_count >= PLUGIN_MAX) return;
    g_plugins[g_count].name = strdup(name);
    g_plugins[g_count].fd = fd;
    g_plugins[g_count].connected = true;
    g_plugins[g_count].subscribed_events = 0;
    g_count++;
}

void plugin_unregister(const char *name) {
    for (int i = 0; i < g_count; i++) {
        if (strcmp(g_plugins[i].name, name) == 0) {
            close(g_plugins[i].fd);
            free(g_plugins[i].name);
            memmove(&g_plugins[i], &g_plugins[i + 1],
                    (g_count - i - 1) * sizeof(struct plugin));
            g_count--;
            return;
        }
    }
}

void plugin_notify(int event_type, const char *data) {
    for (int i = 0; i < g_count; i++) {
        if (!g_plugins[i].connected) continue;
        uint64_t mask = 1ULL << event_type;
        if (!(g_plugins[i].subscribed_events & mask)) continue;

        write(g_plugins[i].fd, data, strlen(data));
        write(g_plugins[i].fd, "\n", 1);
    }
}