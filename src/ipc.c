#include "ipc.h"
#include "event.h"
#include "plugin.h"
#include "misc/helpers.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dispatch/dispatch.h>

#define IPC_MAX_MESSAGE (16 * 1024 * 1024) /* 16 MiB per message */
#define IPC_BACKLOG 8

static int g_socket = -1;
static bool g_running = false;

/* Read exactly `len` bytes, returning 0 on EOF/error. */
static int read_full(int fd, void *buf, size_t len) {
    char *p = buf;
    size_t done = 0;
    while (done < len) {
        ssize_t n = read(fd, p + done, len - done);
        if (n <= 0) return 0;
        done += (size_t)n;
    }
    return 1;
}

bool ipc_send_json(int fd, const char *json) {
    if (fd < 0 || !json) return false;
    size_t len = strlen(json);
    if (len > IPC_MAX_MESSAGE) return false;

    uint32_t raw_len = htonl((uint32_t)len);
    if (write(fd, &raw_len, sizeof(raw_len)) != sizeof(raw_len)) return false;
    if (write(fd, json, len) != (ssize_t)len) return false;
    return true;
}

static void handle_client(int client_fd) {
    uint32_t raw_len = 0;

    while (g_running) {
        /* length prefix: big-endian uint32 */
        if (!read_full(client_fd, &raw_len, sizeof(raw_len)))
            break;

        uint32_t len = ntohl(raw_len);
        if (len == 0 || len > IPC_MAX_MESSAGE)
            break;

        char *buffer = malloc(len + 1);
        if (!buffer) break;
        if (!read_full(client_fd, buffer, len)) {
            free(buffer);
            break;
        }
        buffer[len] = '\0';

        struct ipc_message *msg = ipc_parse_message(buffer, (uint32_t)client_fd);
        free(buffer);

        struct event *event = malloc(sizeof(*event));
        if (event) {
            event->type = EVENT_DAEMON_MESSAGE;
            event->arg1 = client_fd;
            event->data = msg; /* ownership transfers to the event */
            event_post(event);
        } else {
            ipc_message_free(msg);
        }
    }

    close(client_fd);
    plugin_unregister_by_fd(client_fd);
}

bool socket_daemon_begin_un(void) {
    if (g_running) return true;

    struct sockaddr_un addr;
    g_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (g_socket < 0) return false;

    char path[256];
    snprintf(path, sizeof(path), "/tmp/omabar_%s.socket", getenv("USER"));
    unlink(path);

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", path);

    if (bind(g_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(g_socket);
        g_socket = -1;
        return false;
    }

    if (listen(g_socket, IPC_BACKLOG) < 0) {
        close(g_socket);
        g_socket = -1;
        unlink(path);
        return false;
    }

    g_running = true;

    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        while (g_running) {
            int client = accept(g_socket, NULL, NULL);
            if (client < 0) continue;

            /* each connection is handled on its own block so concurrent
               plugin connections never block each other */
            dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                handle_client(client);
            });
        }
    });

    return true;
}

void socket_daemon_end(void) {
    g_running = false;
    if (g_socket >= 0) {
        shutdown(g_socket, SHUT_RDWR);
        close(g_socket);
        g_socket = -1;
    }
    char path[256];
    snprintf(path, sizeof(path), "/tmp/omabar_%s.socket", getenv("USER"));
    unlink(path);
}

bool socket_daemon_is_running(void) {
    return g_running;
}

/* ────────────────────────────────────────────────────────────────
   Minimal JSON parser (subset sufficient for the message protocol)
   ──────────────────────────────────────────────────────────────── */

static size_t utf8_encode(uint32_t cp, char *buf, size_t len) {
    if (cp < 0x80) {
        buf[len++] = (char)cp;
    } else if (cp < 0x800) {
        buf[len++] = (char)(0xC0 | (cp >> 6));
        buf[len++] = (char)(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        buf[len++] = (char)(0xE0 | (cp >> 12));
        buf[len++] = (char)(0x80 | ((cp >> 6) & 0x3F));
        buf[len++] = (char)(0x80 | (cp & 0x3F));
    } else {
        buf[len++] = (char)(0xF0 | (cp >> 18));
        buf[len++] = (char)(0x80 | ((cp >> 12) & 0x3F));
        buf[len++] = (char)(0x80 | ((cp >> 6) & 0x3F));
        buf[len++] = (char)(0x80 | (cp & 0x3F));
    }
    return len;
}

static int hex_digit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static void json_skip_ws(const char **p) {
    while (**p && isspace((unsigned char)**p)) (*p)++;
}

/* Parse a JSON string literal; returns NUL-terminated decoded copy. */
static char *json_parse_string(const char **p) {
    if (**p != '"') return NULL;
    (*p)++;

    char *out = NULL;
    size_t len = 0, cap = 0;

    while (**p && **p != '"') {
        uint32_t c = (unsigned char)**p;
        (*p)++;
        if (c == '\\') {
            switch (**p) {
                case '"':  c = '"';  (*p)++; break;
                case '\\': c = '\\'; (*p)++; break;
                case '/':  c = '/';  (*p)++; break;
                case 'b':  c = '\b'; (*p)++; break;
                case 'f':  c = '\f'; (*p)++; break;
                case 'n':  c = '\n'; (*p)++; break;
                case 'r':  c = '\r'; (*p)++; break;
                case 't':  c = '\t'; (*p)++; break;
                case 'u': {
                    (*p)++;
                    uint32_t cp = 0;
                    int valid = 1;
                    for (int i = 0; i < 4; i++) {
                        int h = hex_digit(**p);
                        if (h < 0) { valid = 0; break; }
                        cp = (cp << 4) | (uint32_t)h;
                        (*p)++;
                    }
                    if (!valid) { c = '?'; break; }
                    if (cp >= 0xD800 && cp <= 0xDBFF) {
                        /* low surrogate must follow */
                        uint32_t lo = 0;
                        if (**p == '\\' && (*p)[1] == 'u') {
                            const char *q = *p + 2;
                            for (int i = 0; i < 4; i++) {
                                int h = hex_digit(*q);
                                if (h < 0) { lo = 0; break; }
                                lo = (lo << 4) | (uint32_t)h;
                                q++;
                            }
                            if (lo >= 0xDC00 && lo <= 0xDFFF) {
                                cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                                *p = q;
                            }
                        }
                    }
                    if (len + 4 >= cap) {
                        cap = cap ? cap * 2 : 32;
                        out = realloc(out, cap);
                        if (!out) return NULL;
                    }
                    len = utf8_encode(cp, out, len);
                    continue;
                }
                default: c = (unsigned char)**p; if (**p) (*p)++; break;
            }
        }
        if (len + 1 >= cap) {
            cap = cap ? cap * 2 : 32;
            out = realloc(out, cap);
            if (!out) return NULL;
        }
        out[len++] = (char)c;
    }

    if (**p != '"') { free(out); return NULL; }
    (*p)++;

    if (!out) out = strdup("");
    else out[len] = '\0';
    return out;
}

enum jval_kind { JVAL_STRING, JVAL_ARRAY, JVAL_RAW };

struct jval {
    enum jval_kind kind;
    char *string;       /* JVAL_STRING */
    char **array;       /* JVAL_ARRAY */
    int array_count;
    char *raw;          /* JVAL_RAW: serialized value text */
};

static struct jval jval_parse(const char **p) {
    struct jval v = { .kind = JVAL_RAW, .string = NULL, .array = NULL, .array_count = 0 };
    int wellformed = 1;

    json_skip_ws(p);

    if (**p == '"') {
        char *s = json_parse_string(p);
        v.kind = JVAL_STRING;
        v.string = s ? s : strdup("");
        if (!(*p - 1 && *p)) wellformed = 0;
    } else if (**p == '[') {
        (*p)++;
        v.kind = JVAL_ARRAY;
        for (;;) {
            json_skip_ws(p);
            if (**p == ']') { (*p)++; break; }
            char *s = json_parse_string(p);
            if (!s) { wellformed = 0; break; }
            buf_push(v.array, s);
            v.array_count = (int)buf_len(v.array);
            json_skip_ws(p);
            if (**p == ',') (*p)++;
            else if (**p == ']') { (*p)++; break; }
            else { wellformed = 0; break; }
        }
        if (!wellformed) {
            /* leave kind as JVAL_ARRAY if we got at least the containers
               right; validation reports using array_count == 0 */
            if (v.array_count == 0) v.array_count = -1;
        }
    } else {
        /* raw value: scan until top-level , or } respecting nesting */
        const char *start = *p;
        int depth = 0;
        int in_string = 0;
        const char *q = start;
        while (*q) {
            char c = *q;
            if (in_string) {
                if (c == '\\') { q++; }
                else if (c == '"') in_string = 0;
            } else {
                if (c == '"') in_string = 1;
                else if (c == '{' || c == '[') depth++;
                else if (c == '}' || c == ']') { depth--; if (depth <= 0) { q++; break; } }
                else if (c == ',' && depth == 0) break;
            }
            q++;
        }
        size_t n = (size_t)(q - start);
        while (n > 0 && (start[n-1] == ' ' || start[n-1] == '\t'
                         || start[n-1] == '\n' || start[n-1] == '\r')) n--;
        v.raw = malloc(n + 1);
        if (v.raw) { memcpy(v.raw, start, n); v.raw[n] = '\0'; }
        *p = q;
    }
    return v;
}

static void jval_free(struct jval *v) {
    if (!v) return;
    free(v->string);
    free(v->raw);
    if (v->array) {
        for (int i = 0; i < v->array_count; i++) free(v->array[i]);
        buf_free(v->array);
    }
}

struct ipc_message *ipc_parse_message(const char *json, uint32_t client_fd) {
    struct ipc_message *msg = calloc(1, sizeof(*msg));
    if (!msg) return NULL;

    msg->client_fd = client_fd;
    msg->status = IPC_MSG_UNKNOWN;

    const char *p = json;
    json_skip_ws(&p);
    if (*p != '{') {
        snprintf(msg->error, sizeof(msg->error),
                 "expected a JSON object root");
        ipc_message_free(msg);
        return NULL;
    }
    p++;

    for (;;) {
        json_skip_ws(&p);
        if (*p == '}') break;
        if (*p != '"') {
            snprintf(msg->error, sizeof(msg->error), "malformed JSON (expected key)");
            ipc_message_free(msg);
            return NULL;
        }
        char *key = json_parse_string(&p);
        if (!key) {
            snprintf(msg->error, sizeof(msg->error), "malformed JSON (key)");
            ipc_message_free(msg);
            return NULL;
        }
        json_skip_ws(&p);
        if (*p != ':') {
            free(key);
            snprintf(msg->error, sizeof(msg->error), "malformed JSON (expected ':')");
            ipc_message_free(msg);
            return NULL;
        }
        p++;
        struct jval v = jval_parse(&p);
        json_skip_ws(&p);

        if (string_equals(key, "type") && v.kind == JVAL_STRING) {
            if (string_equals(v.string, "register")) msg->type = IPC_MSG_REGISTER;
            else if (string_equals(v.string, "update")) msg->type = IPC_MSG_UPDATE;
            else if (string_equals(v.string, "subscribe")) msg->type = IPC_MSG_SUBSCRIBE;
            else if (string_equals(v.string, "query")) msg->type = IPC_MSG_QUERY;
            else if (string_equals(v.string, "trigger")) msg->type = IPC_MSG_TRIGGER;
        } else if (string_equals(key, "name") && v.kind == JVAL_STRING) {
            msg->name = v.string; v.string = NULL;
        } else if (string_equals(key, "item") && v.kind == JVAL_STRING) {
            msg->item = v.string; v.string = NULL;
        } else if (string_equals(key, "icon") && v.kind == JVAL_STRING) {
            msg->icon = v.string; v.string = NULL;
        } else if (string_equals(key, "label") && v.kind == JVAL_STRING) {
            msg->label = v.string; v.string = NULL;
        } else if (string_equals(key, "background_color") && v.kind == JVAL_STRING) {
            msg->background_color = v.string; v.string = NULL;
        } else if (string_equals(key, "event") && v.kind == JVAL_STRING) {
            msg->event = v.string; v.string = NULL;
        } else if (string_equals(key, "data") && v.kind == JVAL_RAW) {
            msg->data = v.raw; v.raw = NULL;
        } else if (string_equals(key, "events")) {
            if (v.kind == JVAL_ARRAY && v.array_count >= 0) {
                msg->events = v.array;
                msg->event_count = v.array_count;
                v.array = NULL;
            } else {
                snprintf(msg->error, sizeof(msg->error),
                         "'events' must be an array of strings");
            }
        }

        free(key);
        jval_free(&v);

        if (*p == ',') { p++; continue; }
        if (*p == '}') break;
        snprintf(msg->error, sizeof(msg->error), "malformed JSON (unexpected token)");
        ipc_message_free(msg);
        return NULL;
    }

    if (msg->error[0]) {
        ipc_message_free(msg);
        return NULL;
    }

    /* validation per message type */
    switch (msg->type) {
        case IPC_MSG_REGISTER:
            if (!msg->name || !*msg->name) {
                snprintf(msg->error, sizeof(msg->error),
                         "register requires a non-empty 'name' field");
            }
            break;
        case IPC_MSG_UPDATE:
            if (!msg->item) {
                snprintf(msg->error, sizeof(msg->error),
                         "update requires an 'item' field");
            }
            break;
        case IPC_MSG_SUBSCRIBE:
            if (msg->event_count < 1) {
                snprintf(msg->error, sizeof(msg->error),
                         "subscribe requires a non-empty 'events' array");
            }
            break;
        case IPC_MSG_QUERY:
            if (!msg->item) {
                snprintf(msg->error, sizeof(msg->error),
                         "query requires an 'item' field");
            }
            break;
        case IPC_MSG_TRIGGER:
            if (!msg->event) {
                snprintf(msg->error, sizeof(msg->error),
                         "trigger requires an 'event' field");
            }
            break;
        default:
            snprintf(msg->error, sizeof(msg->error),
                     "unknown message type (expected register/update/subscribe/query/trigger)");
            break;
    }

    /* status mirrors validity: non-zero on error */
    if (msg->error[0]) msg->status = IPC_MSG_UNKNOWN;
    else msg->status = msg->type;

    return msg;
}

void ipc_message_free(struct ipc_message *msg) {
    if (!msg) return;
    free(msg->name);
    free(msg->item);
    free(msg->icon);
    free(msg->label);
    free(msg->background_color);
    free(msg->event);
    free(msg->data);
    if (msg->events) {
        for (int i = 0; i < msg->event_count; i++) free(msg->events[i]);
        buf_free(msg->events);
    }
    free(msg);
}

char *ipc_make_reply(bool ok, const char *error) {
    if (ok) return strdup("{\"ok\":true}");
    if (!error) error = "unknown error";
    char *escaped = escape_string(error);
    size_t n = strlen(escaped ? escaped : "[]") + 32;
    char *out = malloc(n);
    if (!out) { free(escaped); return NULL; }
    snprintf(out, n, "{\"ok\":false,\"error\":\"%s\"}",
             escaped ? escaped : "[]");
    free(escaped);
    return out;
}