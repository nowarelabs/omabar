#ifndef OMABAR_HOTLOAD_H
#define OMABAR_HOTLOAD_H

#include <stdbool.h>

void hotload_begin(void);
void hotload_end(void);
bool hotload_is_running(void);

#endif