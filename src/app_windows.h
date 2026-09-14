#ifndef OMABAR_APP_WINDOWS_H
#define OMABAR_APP_WINDOWS_H

#include <stdint.h>
#include <stdbool.h>

void app_windows_begin(void);
void app_windows_end(void);
const char *app_windows_front_app_name(void);
int app_windows_is_observing(const char *pid);

#endif