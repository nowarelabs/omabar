#ifndef OMABAR_MAIN_H
#define OMABAR_MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/file.h>
#include <CoreGraphics/CoreGraphics.h>

#include "bar_manager.h"
#include "event_loop.h"
#include "misc/helpers.h"

#define OMABAR_VERSION "0.1.0"

extern struct bar_manager g_bar_manager;
extern pid_t g_pid;

void omabar_init(void);
void omabar_begin(void);
void omabar_cleanup(void);

#endif
