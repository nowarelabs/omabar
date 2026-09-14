#ifndef OMABAR_IPC_H
#define OMABAR_IPC_H

#include <stdbool.h>
#include <stdint.h>

void ipc_begin(void);
void ipc_end(void);
bool ipc_is_running(void);

#endif