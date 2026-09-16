#ifndef OMABAR_HELPERS_H
#define OMABAR_HELPERS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <CoreGraphics/CoreGraphics.h>
#include <CoreFoundation/CoreFoundation.h>

#include "color.h"
#include "env_vars.h"

/* ────────────────────────────────────────────────────────────────
   Base macros
   ──────────────────────────────────────────────────────────────── */

#define array_count(a)  (sizeof((a)) / sizeof(*(a)))
#define max(a, b)       ((a) > (b) ? (a) : (b))
#define min(a, b)       ((a) < (b) ? (a) : (b))
#define clamp(x, l, u)  (min(max((x), (l)), (u)))
#define clampf_range(v, l, u) (clamp((v), (l), (u)))

#define MAXLEN          512
#define FORK_TIMEOUT    60

struct signal_args {
    struct env_vars env_vars;
    void *entity;
    void *param1;
};

/* ────────────────────────────────────────────────────────────────
   rgba_color — ARGB hex parsing (port from spacebar helpers.h)
   ──────────────────────────────────────────────────────────────── */

struct rgba_color {
    bool is_valid;
    uint32_t p;
    float r;
    float g;
    float b;
    float a;
};

static inline struct rgba_color rgba_color_from_hex(uint32_t color) {
    struct rgba_color result;
    result.is_valid = true;
    result.p = color;
    result.r = ((color >> 16) & 0xff) / 255.0f;
    result.g = ((color >> 8) & 0xff) / 255.0f;
    result.b = ((color >> 0) & 0xff) / 255.0f;
    result.a = ((color >> 24) & 0xff) / 255.0f;
    return result;
}

/* ────────────────────────────────────────────────────────────────
   Token & string utilities (port from SketchyBar helpers.h)
   ──────────────────────────────────────────────────────────────── */

struct token {
    char *text;
    unsigned int length;
};

struct key_value_pair {
    char *key;
    char *value;
};

static inline bool string_equals(const char *a, const char *b) {
    return a && b && strcmp(a, b) == 0;
}

static inline bool string_is_empty(const char *s) {
    return !s || !*s;
}

static inline char *string_copy(const char *s) {
    if (!s) return NULL;
    int length = (int)strlen(s);
    char *result = malloc(length + 1);
    if (!result) return NULL;
    memcpy(result, s, length);
    result[length] = '\0';
    return result;
}

static inline char *cfstring_copy(CFStringRef string) {
    CFIndex num_bytes = CFStringGetMaximumSizeForEncoding(
        CFStringGetLength(string), kCFStringEncodingUTF8);
    char *result = malloc(num_bytes + 1);
    if (!result) return NULL;

    if (!CFStringGetCString(string, result, num_bytes + 1, kCFStringEncodingUTF8)) {
        free(result);
        result = NULL;
    }
    return result;
}

static inline char *string_escape_quote(const char *s) {
    if (!s) return NULL;

    const char *cursor = s;
    int num_quotes = 0;
    while (*cursor) {
        if (*cursor == '"') ++num_quotes;
        ++cursor;
    }
    if (!num_quotes) return NULL;

    int size_in_bytes = (int)(cursor - s) + num_quotes;
    char *result = malloc(size_in_bytes + 1);
    if (!result) return NULL;
    result[size_in_bytes] = '\0';

    char *dst = result;
    for (cursor = s; *cursor; ++cursor) {
        if (*cursor == '"') *dst++ = '\\';
        *dst++ = *cursor;
    }
    return result;
}

static inline char *escape_string(const char *string) {
    if (!string) return NULL;
    int len = (int)strlen(string);
    char *buffer = malloc(2 * len + 1);
    if (!buffer) return NULL;
    int cursor = 0;
    for (int i = 0; i < len; i++) {
        if (string[i] == '"') {
            buffer[cursor++] = '\\';
            buffer[cursor++] = '"';
        } else if (string[i] == '\n') {
            buffer[cursor++] = '\\';
            buffer[cursor++] = 'n';
        } else {
            buffer[cursor++] = string[i];
        }
    }
    buffer[cursor] = '\0';
    return buffer;
}

static inline char **token_split(struct token token, char split, uint32_t *count) {
    if (!token.text || token.length == 0) return NULL;
    char **list = NULL;
    *count = 0;

    int prev = -1;
    int len = (int)token.length;
    for (int i = 0; i < len + 1; i++) {
        if (token.text[i] == split || token.text[i] == '\0') {
            list = realloc(list, sizeof(char *) * ++*count);
            if (!list) return NULL;
            token.text[i] = '\0';
            list[*count - 1] = &token.text[prev + 1];
            prev = i;
        }
    }
    return list;
}

static inline bool token_equals(struct token token, const char *match) {
    const char *at = match;
    for (int i = 0; i < (int)token.length; ++i, ++at) {
        if ((*at == 0) || (token.text[i] != *at)) {
            return false;
        }
    }
    return *at == 0;
}

static inline bool token_is_empty(struct token token) {
    return token.length == 0;
}

static inline struct token get_token(char **message) {
    struct token token;

    token.text = *message;
    while (**message) {
        ++(*message);
    }
    token.length = (unsigned int)(*message - token.text);

    if ((*message)[0] == '\0' && (*message)[1] != '\0') {
        ++(*message);
    }

    return token;
}

static inline char *token_to_string(struct token token) {
    char *result = malloc(token.length + 1);
    if (!result) return NULL;

    memcpy(result, token.text, token.length);
    result[token.length] = '\0';
    return result;
}

static inline uint32_t token_to_uint32t(struct token token) {
    char buffer[token.length + 1];
    memcpy(buffer, token.text, token.length);
    buffer[token.length] = '\0';
    return (uint32_t)strtoul(buffer, NULL, 0);
}

static inline int token_to_int(struct token token) {
    char buffer[token.length + 1];
    memcpy(buffer, token.text, token.length);
    buffer[token.length] = '\0';
    return (int)strtol(buffer, NULL, 0);
}

static inline float token_to_float(struct token token) {
    char buffer[token.length + 1];
    memcpy(buffer, token.text, token.length);
    buffer[token.length] = '\0';
    return strtof(buffer, NULL);
}

static inline struct key_value_pair get_key_value_pair(char *token, char split) {
    struct key_value_pair key_value_pair;
    key_value_pair.key = token;

    while (*token) {
        if (token[0] == split) break;
        ++token;
    }

    if (*token != split) {
        key_value_pair.key = NULL;
        key_value_pair.value = NULL;
    } else if (token[1]) {
        *token = '\0';
        key_value_pair.value = token + 1;
    } else {
        *token = '\0';
        key_value_pair.value = NULL;
    }

    return key_value_pair;
}

static inline bool evaluate_boolean_state(struct token state, bool previous_state) {
    if (token_equals(state, "on") || token_equals(state, "yes")
        || token_equals(state, "true") || token_equals(state, "1")
        || token_equals(state, "show") || token_equals(state, "front"))
        return true;
    else if (token_equals(state, "toggle"))
        return !previous_state;
    else
        return false;
}

static inline uint32_t get_set_bit_position(uint32_t mask) {
    if (mask == 0) return UINT32_MAX;
    uint32_t pos = 0;
    while (!(mask & 1)) {
        mask >>= 1;
        pos++;
    }
    return pos;
}

static inline char *get_type_description(uint32_t type) {
    switch (type) {
        case 0x96:
            return "left";
        case 0x97:
            return "right";
        default:
            return "other";
    }
}

static inline char *get_modifier_description(uint32_t modifier) {
    static char description[64];
    description[0] = '\0';

    if (modifier & (1 << 17)) strcat(description, "shift,");
    if (modifier & (1 << 18)) strcat(description, "ctrl,");
    if (modifier & (1 << 19)) strcat(description, "alt,");
    if (modifier & (1 << 20)) strcat(description, "cmd,");
    if (modifier & (1 << 23)) strcat(description, "fn,");

    int len = (int)strlen(description);
    if (len > 0 && description[len - 1] == ',') {
        description[len - 1] = '\0';
    }
    return description[0] ? description : "none";
}

static inline char *format_bool(bool b) {
    return b ? "on" : "off";
}

/* ────────────────────────────────────────────────────────────────
   Animation easing functions
   ──────────────────────────────────────────────────────────────── */

#define deg_to_rad (2.0 * M_PI / 360.0)

static inline double function_linear(double x) { return x; }
static inline double function_square(double x) { return x * x; }
static inline double function_tanh(double x) {
    double a = 0.52;
    return a * tanh(2. * atanh(1. / (2. * a)) * (x - 0.5)) + 0.5;
}
static inline double function_sin(double x) { return sin(M_PI / 2. * x); }
static inline double function_exp(double x) { return x * exp(x - 1.); }
static inline double function_circ(double x) { return sqrt(1.f - powf(x - 1.f, 2.f)); }

/* ────────────────────────────────────────────────────────────────
   Geometry helpers
   ──────────────────────────────────────────────────────────────── */

static inline void clip_rect(CGContextRef context, CGRect region,
                             float clip, uint32_t corner_radius) {
    CGMutablePathRef path = CGPathCreateMutable();
    if (corner_radius > region.size.height / 2.f
        || corner_radius > region.size.width / 2.f) {
        corner_radius = region.size.height > region.size.width
            ? region.size.width / 2.f : region.size.height / 2.f;
    }
    CGPathAddRoundedRect(path, NULL, region, corner_radius, corner_radius);
    CGContextSetBlendMode(context, kCGBlendModeDestinationOut);
    CGContextSetRGBFillColor(context, 0.f, 0.f, 0.f, clip);
    CGContextAddPath(context, path);
    CGContextDrawPath(context, kCGPathFillStroke);
    CGContextSetBlendMode(context, kCGBlendModeNormal);
    CFRelease(path);
}

static inline CGRect cgrect_mirror_y(CGRect rect, float y) {
    CGRect mirrored_rect = rect;
    mirrored_rect.origin.y = 2 * y - rect.origin.y;
    return mirrored_rect;
}

static inline bool cgrect_contains_point(CGRect *r, CGPoint *p) {
    return p->x >= r->origin.x && p->x <= r->origin.x + r->size.width
        && p->y >= r->origin.y && p->y <= r->origin.y + r->size.height;
}

static inline CGColorRef cgcolor_from_color(struct color c) {
    CGFloat components[] = { c.r, c.g, c.b, c.a };
    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    if (!cs) return NULL;
    CGColorRef cg = CGColorCreate(cs, components);
    CGColorSpaceRelease(cs);
    return cg;
}

static inline CFArrayRef cfarray_of_cfnumbers(void *values, size_t size,
                                              int count, CFNumberType type) {
    CFNumberRef temp[count];
    for (int i = 0; i < count; ++i) {
        temp[i] = CFNumberCreate(NULL, type, ((char *)values) + (size * i));
    }

    CFArrayRef result = CFArrayCreate(NULL, (const void **)temp, count,
                                      &kCFTypeArrayCallBacks);
    for (int i = 0; i < count; ++i) {
        CFRelease(temp[i]);
    }
    return result;
}

/* ────────────────────────────────────────────────────────────────
   File / path resolution helpers
   ──────────────────────────────────────────────────────────────── */

static inline bool file_exists(const char *filename) {
    struct stat buffer;
    if (stat(filename, &buffer) != 0) return false;
    if (buffer.st_mode & S_IFDIR) return false;
    return true;
}

static inline bool ensure_executable_permission(const char *filename) {
    struct stat buffer;
    if (stat(filename, &buffer) != 0) return false;

    bool is_executable = buffer.st_mode & S_IXUSR;
    if (!is_executable && chmod(filename, S_IXUSR | buffer.st_mode) != 0)
        return false;
    return true;
}

static inline char *file_read(const char *path) {
    if (!path) return NULL;
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (len < 0) { fclose(f); return NULL; }

    char *buf = malloc(len + 1);
    if (!buf) { fclose(f); return NULL; }

    size_t read = fread(buf, 1, len, f);
    fclose(f);
    buf[read] = '\0';
    return buf;
}

static inline char *resolve_path(char *path) {
    if (!path) return NULL;

    if (path[0] == '~') {
        const char *home = getenv("HOME");
        if (home) {
            char buf[MAXLEN];
            snprintf(buf, sizeof(buf), "%s%s", home, &path[1]);
            free(path);
            return string_copy(buf);
        }
    }
    return path;
}

/* ────────────────────────────────────────────────────────────────
   Memory pool allocator (port from spacebar memory_pool.h)
   ──────────────────────────────────────────────────────────────── */

#define KILOBYTES(value) ((value) * 1024ULL)
#define MEGABYTES(value) (KILOBYTES(value) * 1024ULL)
#define GIGABYTES(value) (MEGABYTES(value) * 1024ULL)

struct memory_pool {
    void *memory;
    uint64_t size;
    volatile uint64_t used;
};

static inline bool memory_pool_init(struct memory_pool *pool, uint64_t size) {
    pool->used = 0;
    pool->size = size;
    pool->memory = mmap(0, size, PROT_READ | PROT_WRITE,
                        MAP_ANON | MAP_PRIVATE, -1, 0);
    return pool->memory != NULL;
}

static inline void *memory_pool_push_size(struct memory_pool *pool, uint64_t size) {
    for (;;) {
        uint64_t used = pool->used;
        uint64_t new_used = used + size;

        if (new_used < pool->size) {
            if (__sync_bool_compare_and_swap(&pool->used, used, new_used)) {
                return pool->memory + used;
            }
        } else {
            if (__sync_bool_compare_and_swap(&pool->used, used, size)) {
                return pool->memory;
            }
        }
    }
}

static inline void memory_pool_destroy(struct memory_pool *pool) {
    if (pool->memory) {
        munmap(pool->memory, pool->size);
        pool->memory = NULL;
    }
    pool->used = 0;
    pool->size = 0;
}

#define memory_pool_push(p, t) memory_pool_push_size(p, sizeof(t))

/* ────────────────────────────────────────────────────────────────
   Stretchy buffer (port from spacebar sbuffer.h)
   ──────────────────────────────────────────────────────────────── */

struct buf_hdr {
    size_t len;
    size_t cap;
    char buf[0];
};

#define buf__hdr(b) ((struct buf_hdr *)((char *)(b) - offsetof(struct buf_hdr, buf)))
#define buf__should_grow(b, n) (buf_len(b) + (n) >= buf_cap(b))
#define buf__fit(b, n) (buf__should_grow(b, n) ? ((b) = buf__grow_f(b, buf_len(b) + (n), sizeof(*(b)))) : 0)

#define buf_len(b)  ((b) ? buf__hdr(b)->len : 0)
#define buf_cap(b)  ((b) ? buf__hdr(b)->cap : 0)
#define buf_last(b) ((b)[buf_len(b) - 1])
#define buf_push(b, x) (buf__fit(b, 1), (b)[buf_len(b)] = (x), buf__hdr(b)->len++)
#define buf_del(b, x)  ((b) ? (b)[x] = (b)[buf_len(b) - 1], buf__hdr(b)->len-- : 0)
#define buf_free(b)    ((b) ? free(buf__hdr(b)) : 0)
#define buf_clear(b)   ((b) ? (buf__hdr(b)->len = 0) : 0)

static inline void *buf__grow_f(const void *buf, size_t new_len, size_t elem_size) {
    size_t new_cap = max(1 + 2 * buf_cap(buf), new_len);
    size_t new_size = offsetof(struct buf_hdr, buf) + new_cap * elem_size;
    struct buf_hdr *new_hdr = realloc(buf ? buf__hdr(buf) : 0, new_size);
    if (!new_hdr) return (void *)buf;
    new_hdr->cap = new_cap;
    if (!buf) new_hdr->len = 0;
    return new_hdr->buf;
}

/* ────────────────────────────────────────────────────────────────
   Hash table (port from spacebar hashtable.h)
   ──────────────────────────────────────────────────────────────── */

#define TABLE_HASH_FUNC(name)     unsigned long name(void *key)
typedef TABLE_HASH_FUNC(table_hash_func);

#define TABLE_COMPARE_FUNC(name)  int name(void *key_a, void *key_b)
typedef TABLE_COMPARE_FUNC(table_compare_func);

struct bucket {
    void *key;
    void *value;
    struct bucket *next;
};

struct table {
    int count;
    int capacity;
    float max_load;
    table_hash_func *hash;
    table_compare_func *cmp;
    struct bucket **buckets;
};

static inline void table_init(struct table *table, int capacity,
                              table_hash_func hash, table_compare_func cmp) {
    table->count = 0;
    table->capacity = capacity;
    table->max_load = 0.75f;
    table->hash = hash;
    table->cmp = cmp;
    table->buckets = calloc(capacity, sizeof(struct bucket *));
}

static inline void table_free(struct table *table) {
    for (int i = 0; i < table->capacity; ++i) {
        struct bucket *next, *bucket = table->buckets[i];
        while (bucket) {
            next = bucket->next;
            free(bucket->key);
            free(bucket);
            bucket = next;
        }
    }

    if (table->buckets) {
        free(table->buckets);
        table->buckets = NULL;
    }
    table->count = 0;
    table->capacity = 0;
}

static inline struct bucket **table_get_bucket(struct table *table, void *key) {
    if (!table->buckets || table->capacity == 0) return NULL;
    struct bucket **bucket = table->buckets + (table->hash(key) % table->capacity);
    while (*bucket) {
        if (table->cmp((*bucket)->key, key)) {
            break;
        }
        bucket = &(*bucket)->next;
    }
    return bucket;
}

static inline void table_rehash(struct table *table) {
    struct bucket **old_buckets = table->buckets;
    int old_capacity = table->capacity;

    table->count = 0;
    table->capacity = 2 * table->capacity;
    table->buckets = calloc(table->capacity, sizeof(struct bucket *));

    for (int i = 0; i < old_capacity; ++i) {
        struct bucket *next_bucket, *old_bucket = old_buckets[i];
        while (old_bucket) {
            struct bucket **new_bucket = table_get_bucket(table, old_bucket->key);
            *new_bucket = malloc(sizeof(struct bucket));
            if (!*new_bucket) return;
            (*new_bucket)->key = old_bucket->key;
            (*new_bucket)->value = old_bucket->value;
            (*new_bucket)->next = NULL;
            ++table->count;
            next_bucket = old_bucket->next;
            free(old_bucket);
            old_bucket = next_bucket;
        }
    }

    free(old_buckets);
}

#define table_add(table, key, value) _table_add(table, key, sizeof(*key), value)

static inline void _table_add(struct table *table, void *key, int key_size,
                              void *value) {
    struct bucket **bucket = table_get_bucket(table, key);
    if (!bucket) return;
    if (*bucket) {
        if (!(*bucket)->value) {
            (*bucket)->value = value;
        }
    } else {
        *bucket = malloc(sizeof(struct bucket));
        if (!*bucket) return;
        (*bucket)->key = malloc(key_size);
        if (!(*bucket)->key) { free(*bucket); *bucket = NULL; return; }
        (*bucket)->value = value;
        memcpy((*bucket)->key, key, key_size);
        (*bucket)->next = NULL;
        ++table->count;

        float load = (1.0f * table->count) / table->capacity;
        if (load > table->max_load) {
            table_rehash(table);
        }
    }
}

static inline void table_remove(struct table *table, void *key) {
    struct bucket **bucket = table_get_bucket(table, key);
    if (!bucket) return;
    struct bucket *next, *to_free = *bucket;
    if (to_free) {
        free(to_free->key);
        next = to_free->next;
        free(to_free);
        *bucket = next;
        --table->count;
    }
}

static inline void *table_find(struct table *table, void *key) {
    struct bucket **bucket = table_get_bucket(table, key);
    return (bucket && *bucket) ? (*bucket)->value : NULL;
}

/* ────────────────────────────────────────────────────────────────
   Process / exec helpers
   ──────────────────────────────────────────────────────────────── */

static inline bool is_root(void) {
    return getuid() == 0 || geteuid() == 0;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
static inline bool sync_exec(char *command, struct env_vars *env_vars) {
    if (env_vars) {
        for (int i = 0; i < env_vars->count; i++) {
            setenv(env_vars->vars[i].key, env_vars->vars[i].value, 1);
        }
    }

    char *exec[] = { "/usr/bin/env", "sh", "-c", command, NULL };
    return execvp(exec[0], exec) == 0;
}

static inline bool fork_exec(char *command, struct env_vars *env_vars) {
    int pid = vfork();
    if (pid == -1) return false;
    if (pid != 0) return true;

    alarm(FORK_TIMEOUT);
    exit(sync_exec(command, env_vars));
}

/* Same as fork_exec but captures the child's stdout. `out` receives a
   caller-owned NUL-terminated buffer ("" on empty output). Returns false
   only when the pipe/fork setup fails. */
static inline bool fork_exec_output(char *command, struct env_vars *env_vars,
                                    char **out) {
    if (!out) return false;
    *out = strdup("");

    int pipe_out[2];
    if (pipe(pipe_out) == -1) return false;

    int pid = vfork();
    if (pid == -1) {
        close(pipe_out[0]);
        close(pipe_out[1]);
        return false;
    }
    if (pid == 0) {
        close(pipe_out[0]);
        dup2(pipe_out[1], STDOUT_FILENO);
        close(pipe_out[1]);
        alarm(FORK_TIMEOUT);
        exit(sync_exec(command, env_vars) ? 0 : 1);
        /* NOTREACHED */
    }
    close(pipe_out[1]);

    size_t capacity = 4096, length = 0;
    char *buffer = malloc(capacity);
    if (!buffer) return false;

    ssize_t n;
    while ((n = read(pipe_out[0], buffer + length, capacity - length - 1)) > 0) {
        length += (size_t)n;
        if (length >= capacity - 1) {
            capacity *= 2;
            buffer = realloc(buffer, capacity);
            if (!buffer) return false;
        }
    }
    close(pipe_out[0]);
    buffer[length] = '\0';

    int status;
    while (waitpid(pid, &status, 0) == -1) { /* EINTR retry */ }

    free(*out);
    *out = buffer;
    return true;
}
#pragma clang diagnostic pop

static inline void error(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    exit(EXIT_FAILURE);
}

static inline int get_wid_from_cg_event(CGEventRef event) {
    return (int)CGEventGetIntegerValueField(event, 0x33);
}

#endif