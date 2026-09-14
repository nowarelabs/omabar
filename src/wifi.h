#ifndef OMABAR_WIFI_H
#define OMABAR_WIFI_H

#include <stdbool.h>

void wifi_begin(void);
void wifi_end(void);
const char *wifi_get_ssid(void);

#endif