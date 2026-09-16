#ifndef OMABAR_PLUGIN_H
#define OMABAR_PLUGIN_H

#include <stdbool.h>
#include <stdint.h>

/* Plugin lifecycle + notification hub.
 *
 * Plugins connect over the omabar Unix socket (see ipc.c) and announce
 * themselves with {"type":"register","name":"..."} or implicitly with a
 * subscribe message.  Every handler in bar_manager that re-runs items
 * on an event also fans the event out to plugins whose subscribed_events
 * mask intersects the fired event's mask (mask bits match the UPDATE_* /
 * custom_event registration order).
 *
 * Notifications are delivered with the same length-prefixed JSON
 * protocol: {"type":"event","name":"volume_changed","data":...}.  */

struct plugin {
    char *name;
    int fd;
    uint64_t subscribed_events; /* custom-event mask bits */
    bool connected;
    uint64_t last_seen;         /* tick counter, for health checks */
};

void plugin_init(void);
void plugin_destroy(void);

void plugin_register(const char *name, int fd);
void plugin_unregister(const char *name);
void plugin_unregister_by_fd(int fd);

void plugin_subscribe(int fd, uint64_t mask);
void plugin_unsubscribe(int fd, uint64_t mask);

/* Send an event to every plugin subscribed to `mask`.  `name` is the
   event name ("volume_changed", ...); `data` is an optional JSON value
   embedded under the notification's "data" key (NULL => null). */
void plugin_notify(uint64_t mask, const char *name, const char *data);

/* Drop connections that have hit EOF / a broken pipe (called from the
   per-second scroll tick). */
void plugin_health_check(void);

/* Number of currently tracked plugins. */
int plugin_count(void);

#endif