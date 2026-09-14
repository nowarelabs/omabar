#ifndef OMABAR_HELPERS_H
#define OMABAR_HELPERS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/file.h>
#include <CoreGraphics/CoreGraphics.h>

#include "color.h"

/* stretchy buffer (dynamic array) */
#define buf_push(a, v) ((a) = buf_append((a), (v), sizeof(*(a))))
#define buf_count(a) ((a) ? *(int *)((a) - 1) : 0)

static inline void *buf_append(void *a, void *item, size_t elem_size) {
    if (!a) {
        int *p = malloc(sizeof(int) + elem_size);
        if (!p) return a;
        *p = 1;
        memcpy(p + 1, item, elem_size);
        return p + 1;
    }
    int count = *(int *)((char *)a - sizeof(int));
    int *p = realloc((char *)a - sizeof(int), sizeof(int) + elem_size * (count + 1));
    if (!p) return a;
    *p = count + 1;
    memcpy((char *)p + sizeof(int) + elem_size * count, item, elem_size);
    return p + 1;
}

/* string helpers */
static inline int str_estimate_token_count(const char *str, char delimiter) {
    if (!str || !*str) return 0;
    int count = 1;
    for (const char *p = str; *p; p++)
        if (*p == delimiter) count++;
    return count;
}

static inline const char *str_get_token(const char **str, char delimiter) {
    if (!str || !*str || !**str) return NULL;
    const char *start = *str;
    const char *p = strchr(*str, delimiter);
    if (p) {
        *str = p + 1;
        return start;
    }
    *str = start + strlen(start);
    return start;
}

/* fork/exec helper */
static inline pid_t fork_exec(const char *command, char *const envp[]) {
    pid_t pid = fork();
    if (pid == 0) {
        if (envp)
            execle("/usr/bin/env", "env", "-i", command, NULL, envp);
        else
            execl("/bin/sh", "sh", "-c", command, NULL);
        _exit(127);
    }
    return pid;
}

/* CGColor helper */
static inline CGColorRef cgcolor_from_color(struct color c) {
    CGFloat components[] = { c.r, c.g, c.b, c.a };
    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGColorRef cg = CGColorCreate(cs, components);
    CGColorSpaceRelease(cs);
    return cg;
}

#endif