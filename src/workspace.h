#ifndef OMABAR_WORKSPACE_H
#define OMABAR_WORKSPACE_H

#include <stdbool.h>

void workspace_event_handler_init(void);
void workspace_event_handler_begin(void);
void workspace_event_handler_end(void);

void workspace_recheck_space_change(void);
void forced_space_change_event(void);

#endif