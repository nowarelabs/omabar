#ifndef OMABAR_APP_WINDOWS_H
#define OMABAR_APP_WINDOWS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

void app_windows_begin(void);
void app_windows_end(void);
const char *app_windows_front_app_name(void);
int app_windows_is_observing(const char *pid);
pid_t app_windows_pid_for_bundle_id(const char *bundle_id);
const char *app_windows_name_for_pid(pid_t pid, char *buf, size_t len);

#endif