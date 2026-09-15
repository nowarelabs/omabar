#ifndef OMABAR_ENV_VARS_H
#define OMABAR_ENV_VARS_H

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

#define ENV_VAR_MAX 128

struct env_var {
    char *key;
    char *value;
};

struct env_vars {
    struct env_var vars[ENV_VAR_MAX];
    int count;
};

/* Global store — plugins/scripts read from this via the IPC layer.
   Initialised in main.c at startup. */
extern struct env_vars g_env_vars;

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

static inline void env_vars_unset(struct env_vars *ev, const char *key) {
    if (!ev || !key) return;
    for (int i = 0; i < ev->count; i++) {
        if (strcmp(ev->vars[i].key, key) == 0) {
            free(ev->vars[i].key);
            free(ev->vars[i].value);
            ev->vars[i] = ev->vars[ev->count - 1];
            ev->count--;
            return;
        }
    }
}

static inline void env_vars_set(struct env_vars *ev, const char *key,
                                const char *value) {
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

/* SketchyBar-compatible lookup name. */
static inline const char *env_vars_get_value_for_key(struct env_vars *ev,
                                                     const char *key) {
    if (!ev || !key) return NULL;
    for (int i = 0; i < ev->count; i++) {
        if (strcmp(ev->vars[i].key, key) == 0)
            return ev->vars[i].value;
    }
    return NULL;
}

static inline const char *env_vars_get(struct env_vars *ev, const char *key) {
    return env_vars_get_value_for_key(ev, key);
}

static inline bool env_vars_has(struct env_vars *ev, const char *key) {
    return env_vars_get(ev, key) != NULL;
}

/* Serialize as a packed stream of NUL-terminated keys and values
   (k0\0v0\0k1\0v1\0...\0). Returns caller-owned memory. */
static inline char *env_vars_copy_serialized_representation(struct env_vars *ev,
                                                            uint32_t *len) {
    uint32_t length = 0;
    for (int i = 0; i < ev->count; i++) {
        length += ev->vars[i].key ? strlen(ev->vars[i].key) + 1 : 1;
        length += ev->vars[i].value ? strlen(ev->vars[i].value) + 1 : 1;
    }

    uint32_t caret = 0;
    char *seri = malloc(++length);
    if (!seri) return NULL;

    for (int i = 0; i < ev->count; i++) {
        if (ev->vars[i].key) {
            uint32_t n = (uint32_t)strlen(ev->vars[i].key) + 1;
            memcpy(seri + caret, ev->vars[i].key, n);
            caret += n;
        } else {
            seri[caret++] = '\0';
        }

        if (ev->vars[i].value) {
            uint32_t n = (uint32_t)strlen(ev->vars[i].value) + 1;
            memcpy(seri + caret, ev->vars[i].value, n);
            caret += n;
        } else {
            seri[caret++] = '\0';
        }
    }
    seri[caret++] = '\0';
    assert(caret == length);
    *len = length;
    return seri;
}

#endif