#ifndef OMABAR_VOLUME_H
#define OMABAR_VOLUME_H

#include <stdbool.h>

void forced_volume_event(void);
void volume_begin(void);
void volume_end(void);
int volume_get_percentage(void);

#endif