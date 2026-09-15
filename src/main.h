#ifndef OMABAR_MAIN_H
#define OMABAR_MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <CoreGraphics/CoreGraphics.h>
#include <CoreFoundation/CoreFoundation.h>

#include "bar_manager.h"
#include "event_loop.h"
#include "window.h"
#include "misc/defines.h"
#include "misc/helpers.h"
#include "misc/env_vars.h"

#define OMABAR_VERSION "0.1.0"

extern struct bar_manager g_bar_manager;
extern struct env_vars g_env_vars;
extern pid_t g_pid;

void omabar_init(void);
void omabar_begin(void);
void omabar_cleanup(void);

#endif
