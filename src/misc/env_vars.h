#ifndef OMABAR_ENV_VARS_H
#define OMABAR_ENV_VARS_H

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define ENV_VAR_MAX 128

struct env_var {
    char *key;
    char *value;
};

struct env_vars {
    struct env_var vars[ENV_VAR_MAX];
    int count;
};

static inline void env_vars_init(struct env_vars *ev) {
    memset(ev, 0, sizeof(*ev));
}

static inline void env_vars_destroy(struct env_vars *ev) {
    for (int i = 0; i < ev->count; i++) {
        free(ev->vars[i].key);
        free(ev->vars[i].value);
    }
    ev->count = 0;
}

static inline void env_vars_set(struct env_vars *ev, const char *key, const char *value) {
    if (!ev || !key) return;

    for (int i = 0; i < ev->count; i++) {
        if (strcmp(ev->vars[i].key, key) == 0) {
            free(ev->vars[i].value);
            ev->vars[i].value = value ? strdup(value) : NULL;
            return;
        }
    }

    if (ev->count >= ENV_VAR_MAX) return;
    ev->vars[ev->count].key = strdup(key);
    ev->vars[ev->count].value = value ? strdup(value) : NULL;
    ev->count++;
}

static inline const char *env_vars_get(struct env_vars *ev, const char *key) {
    if (!ev || !key) return NULL;
    for (int i = 0; i < ev->count; i++) {
        if (strcmp(ev->vars[i].key, key) == 0)
            return ev->vars[i].value;
    }
    return NULL;
}

static inline bool env_vars_has(struct env_vars *ev, const char *key) {
    return env_vars_get(ev, key) != NULL;
}

#endif