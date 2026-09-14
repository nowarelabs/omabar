#ifndef OMABAR_PLUGIN_H
#define OMABAR_PLUGIN_H

#include <stdbool.h>
#include <stdint.h>

struct plugin {
    char *name;
    int fd;
    uint64_t subscribed_events;
    bool connected;
};

void plugin_init(void);
void plugin_destroy(void);
void plugin_register(const char *name, int fd);
void plugin_unregister(const char *name);
void plugin_notify(int event_type, const char *data);

#endif